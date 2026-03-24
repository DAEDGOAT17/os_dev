#ifndef NEURAL_NET_H
#define NEURAL_NET_H

#include <stdint.h>

// ===== NEURAL NETWORK ARCHITECTURE =====

#define EMBEDDING_DIM 32        // Word embedding dimension
#define HIDDEN_DIM 64           // Hidden layer size
#define MAX_SEQUENCE_LEN 20     // Max words in a sentence
#define NUM_INTENTS 10          // Number of intent classes
#define VOCAB_SIZE 200          // Vocabulary size

// ===== STRUCTURES =====

// Word embedding (learned representation)
typedef struct {
    float embedding[EMBEDDING_DIM];
} WordEmbedding;

// Neural network weights
typedef struct {
    // Embedding layer weights
    WordEmbedding word_embeddings[VOCAB_SIZE];
    
    // Attention weights (simplified self-attention)
    float attention_weights[EMBEDDING_DIM][EMBEDDING_DIM];
    
    // Dense layer 1: embedding → hidden
    float dense1_weights[EMBEDDING_DIM][HIDDEN_DIM];
    float dense1_bias[HIDDEN_DIM];
    
    // Dense layer 2: hidden → intent
    float dense2_weights[HIDDEN_DIM][NUM_INTENTS];
    float dense2_bias[NUM_INTENTS];
} NeuralNetwork;

// ===== FORWARD PASS =====

// Tokenize text to word indices
int tokenize_to_indices(const char* text, int* indices, int max_len);

// Forward pass: text → intent prediction
typedef struct {
    char intent[32];
    float confidence;
    float logits[NUM_INTENTS];  // Raw neural network outputs
} NeuralPrediction;

NeuralPrediction neural_predict(const char* input);

// ===== TRAINING =====

// Train on examples
void neural_train(const char* text, const char* correct_intent, float learning_rate);

// Initialize network with random weights
void neural_init(void);

// Save/load weights
void neural_save_weights(void);
void neural_load_weights(void);

#endif