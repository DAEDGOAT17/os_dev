#include "neural_net.h"
#include "word_embeddings.h"
#include "string.h"
#include "screen.h"
#include <stdint.h>

// Global neural network
static NeuralNetwork nn;

// Intent mapping
static const char* intent_names[] = {
    "mem", "ps", "clear", "reboot", "help", 
    "uptime", "version", "vocab", "unknown", "other"
};

#define NUM_INTENT_NAMES (sizeof(intent_names) / sizeof(intent_names[0]))

// ===== ACTIVATION FUNCTIONS =====

// ReLU: max(0, x)
static float relu(float x) {
    return x > 0.0f ? x : 0.0f;
}

// ReLU derivative
static float relu_derivative(float x) {
    return x > 0.0f ? 1.0f : 0.0f;
}

// Softmax: convert logits to probabilities
static void softmax(float* logits, int size, float* output) {
    // Find max for numerical stability
    float max_logit = logits[0];
    for (int i = 1; i < size; i++) {
        if (logits[i] > max_logit) max_logit = logits[i];
    }
    
    // Compute exp(x - max) and sum
    float sum = 0.0f;
    for (int i = 0; i < size; i++) {
        float exp_val = logits[i] - max_logit;
        // Approximate exp using Taylor series (no stdlib exp)
        float e = 1.0f + exp_val + (exp_val * exp_val) / 2.0f;
        if (e < 0.0f) e = 0.001f;
        output[i] = e;
        sum += output[i];
    }
    
    // Normalize
    for (int i = 0; i < size; i++) {
        output[i] /= sum;
    }
}

// ===== FORWARD PASS =====

// Simple matrix multiplication: (n x m) * (m x p) = (n x p)
static void matmul(float* a, float* b, float* c, int n, int m, int p) {
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < p; j++) {
            c[i * p + j] = 0.0f;
            for (int k = 0; k < m; k++) {
                c[i * p + j] += a[i * m + k] * b[k * p + j];
            }
        }
    }
}

// Add bias vector to matrix row
static void add_bias(float* result, float* bias, int size) {
    for (int i = 0; i < size; i++) {
        result[i] += bias[i];
    }
}

NeuralPrediction neural_predict(const char* input) {
    NeuralPrediction result;
    result.intent[0] = 0;
    result.confidence = 0.0f;
    
    // Step 1: Tokenize
    int word_indices[MAX_SEQUENCE_LEN];
    int seq_len = tokenize_to_indices(input, word_indices, MAX_SEQUENCE_LEN);
    
    if (seq_len == 0) {
        strcpy(result.intent, "unknown");
        return result;
    }
    
    // Step 2: Embedding layer
    // Average word embeddings to get sentence embedding
    float sentence_embedding[EMBEDDING_DIM] = {0};
    
    for (int i = 0; i < seq_len; i++) {
        int word_idx = word_indices[i];
        if (word_idx >= VOCAB_SIZE) word_idx = VOCAB_SIZE - 1;
        
        for (int j = 0; j < EMBEDDING_DIM; j++) {
            sentence_embedding[j] += nn.word_embeddings[word_idx].embedding[j];
        }
    }
    
    // Average
    for (int i = 0; i < EMBEDDING_DIM; i++) {
        sentence_embedding[i] /= (float)seq_len;
    }
    
    // Step 3: Attention (simplified)
    // Apply attention weights to embeddings
    float attended[EMBEDDING_DIM] = {0};
    for (int i = 0; i < EMBEDDING_DIM; i++) {
        for (int j = 0; j < EMBEDDING_DIM; j++) {
            attended[i] += nn.attention_weights[i][j] * sentence_embedding[j];
        }
    }
    
    // Step 4: Dense layer 1 (with ReLU activation)
    float hidden[HIDDEN_DIM];
    matmul(attended, (float*)nn.dense1_weights, hidden, 1, EMBEDDING_DIM, HIDDEN_DIM);
    add_bias(hidden, nn.dense1_bias, HIDDEN_DIM);
    
    for (int i = 0; i < HIDDEN_DIM; i++) {
        hidden[i] = relu(hidden[i]);
    }
    
    // Step 5: Dense layer 2 (output logits)
    float logits[NUM_INTENTS];
    matmul(hidden, (float*)nn.dense2_weights, logits, 1, HIDDEN_DIM, NUM_INTENTS);
    add_bias(logits, nn.dense2_bias, NUM_INTENTS);
    
    // Step 6: Softmax to get probabilities
    float probabilities[NUM_INTENTS];
    softmax(logits, NUM_INTENTS, probabilities);
    
    // Copy logits to result
    for (int i = 0; i < NUM_INTENTS; i++) {
        result.logits[i] = probabilities[i];
    }
    
    // Step 7: Find best intent
    float best_prob = 0.0f;
    int best_intent = -1;
    
    for (int i = 0; i < NUM_INTENT_NAMES; i++) {
        if (probabilities[i] > best_prob) {
            best_prob = probabilities[i];
            best_intent = i;
        }
    }
    
    if (best_intent >= 0 && best_prob > 0.3f) {  // 30% threshold
        strcpy(result.intent, intent_names[best_intent]);
        result.confidence = best_prob * 100.0f;
    } else {
        strcpy(result.intent, "unknown");
        result.confidence = 0.0f;
    }
    
    return result;
}

// ===== TOKENIZATION =====

int tokenize_to_indices(const char* text, int* indices, int max_len) {
    int count = 0;
    char word[32] = {0};
    int word_idx = 0;
    
    for (int i = 0; text[i] && count < max_len; i++) {
        char c = text[i];
        
        // Convert to lowercase
        if (c >= 'A' && c <= 'Z') {
            c = c - 'A' + 'a';
        }
        
        // Check if alphanumeric
        if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')) {
            if (word_idx < 31) {
                word[word_idx++] = c;
            }
        } else {
            // End of word
            if (word_idx > 0) {
                word[word_idx] = '\0';
                
                // Get or create word index
                int idx = get_or_create_word(word);
                if (idx >= 0 && idx < VOCAB_SIZE) {
                    indices[count++] = idx;
                }
                
                word_idx = 0;
            }
        }
    }
    
    // Handle last word
    if (word_idx > 0) {
        word[word_idx] = '\0';
        int idx = get_or_create_word(word);
        if (idx >= 0 && idx < VOCAB_SIZE && count < max_len) {
            indices[count++] = idx;
        }
    }
    
    return count;
}

// ===== TRAINING =====

void neural_train(const char* text, const char* correct_intent, float learning_rate) {
    // Forward pass
    NeuralPrediction pred = neural_predict(text);
    
    // Find correct intent index
    int correct_idx = -1;
    for (int i = 0; i < NUM_INTENT_NAMES; i++) {
        if (strcmp(intent_names[i], correct_intent) == 0) {
            correct_idx = i;
            break;
        }
    }
    
    if (correct_idx < 0) return;
    
    // Compute loss and update weights (simplified backprop)
    for (int i = 0; i < NUM_INTENTS; i++) {
        float error = (i == correct_idx ? 1.0f : 0.0f) - pred.logits[i];
        
        // Update output layer weights
        for (int j = 0; j < HIDDEN_DIM; j++) {
            nn.dense2_weights[j][i] += learning_rate * error;
        }
    }
    
    print_string("[NEURAL] Trained on: ");
    print_string(text);
    print_string(" → ");
    print_string(correct_intent);
    print_string("\n");
}

// ===== INITIALIZATION =====

void neural_init(void) {
    print_string("[NEURAL] Initializing neural network...\n");
    
    // Initialize embeddings with small random values
    uint32_t seed = 42;
    
    for (int i = 0; i < VOCAB_SIZE; i++) {
        for (int j = 0; j < EMBEDDING_DIM; j++) {
            seed = (seed * 1103515245 + 12345) & 0x7fffffff;
            nn.word_embeddings[i].embedding[j] = (float)(seed % 100) / 1000.0f - 0.05f;
        }
    }
    
    // Initialize attention weights (identity-like)
    for (int i = 0; i < EMBEDDING_DIM; i++) {
        for (int j = 0; j < EMBEDDING_DIM; j++) {
            seed = (seed * 1103515245 + 12345) & 0x7fffffff;
            nn.attention_weights[i][j] = (i == j ? 0.5f : 0.0f) + 
                                         (float)(seed % 10) / 1000.0f;
        }
    }
    
    // Initialize dense layer 1
    for (int i = 0; i < EMBEDDING_DIM; i++) {
        for (int j = 0; j < HIDDEN_DIM; j++) {
            seed = (seed * 1103515245 + 12345) & 0x7fffffff;
            nn.dense1_weights[i][j] = (float)(seed % 100) / 1000.0f - 0.05f;
        }
    }
    
    for (int i = 0; i < HIDDEN_DIM; i++) {
        nn.dense1_bias[i] = 0.0f;
    }
    
    // Initialize dense layer 2
    for (int i = 0; i < HIDDEN_DIM; i++) {
        for (int j = 0; j < NUM_INTENTS; j++) {
            seed = (seed * 1103515245 + 12345) & 0x7fffffff;
            nn.dense2_weights[i][j] = (float)(seed % 100) / 1000.0f - 0.05f;
        }
    }
    
    for (int i = 0; i < NUM_INTENTS; i++) {
        nn.dense2_bias[i] = 0.0f;
    }
    
    print_string("[NEURAL] Network initialized!\n");
}

void neural_save_weights(void) {
    print_string("[NEURAL] Saving weights...\n");
    // TODO: Write weights to disk/EEPROM
}

void neural_load_weights(void) {
    print_string("[NEURAL] Loading weights...\n");
    // TODO: Read weights from disk/EEPROM
}