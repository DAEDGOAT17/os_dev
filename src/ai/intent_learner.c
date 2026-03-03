#include "intent_learner.h"
#include "word_embeddings.h"
#include "screen.h"
#include "string.h"
#include <stdint.h>

#define MAX_INTENTS 20
#define MAX_EXAMPLES_PER_INTENT 10

// Integer square root helper
static int int_sqrt(int n) {
    if (n == 0) return 0;
    if (n == 1) return 1;
    
    int x = n;
    int y = (x + 1) / 2;
    
    while (y < x) {
        x = y;
        y = (x + n / x) / 2;
    }
    return x;
}

// Store training examples for each intent
typedef struct {
    char intent_name[32];
    float intent_embedding[EMBEDDING_DIM];  // Centroid of all examples
    
    // Training examples
    char examples[MAX_EXAMPLES_PER_INTENT][256];
    int example_count;
    
    uint32_t times_used;
} IntentProfile;

static IntentProfile intents[MAX_INTENTS];
static int intent_count = 0;

// ========== CORE LEARNING FUNCTIONS ==========

// Add a training example for an intent
void train_intent(const char* intent_name, const char* example) {
    // Find or create intent
    int intent_idx = -1;
    
    for (int i = 0; i < intent_count; i++) {
        if (strcmp(intents[i].intent_name, intent_name) == 0) {
            intent_idx = i;
            break;
        }
    }
    
    // Create new intent if doesn't exist
    if (intent_idx < 0 && intent_count < MAX_INTENTS) {
        intent_idx = intent_count;
        strcpy(intents[intent_idx].intent_name, intent_name);
        intents[intent_idx].example_count = 0;
        
        // Initialize intent embedding to zeros
        for (int i = 0; i < EMBEDDING_DIM; i++) {
            intents[intent_idx].intent_embedding[i] = 0.0f;
        }
        
        intent_count++;
    }
    
    if (intent_idx < 0) return;  // No space
    
    // Add training example
    IntentProfile* profile = &intents[intent_idx];
    
    if (profile->example_count >= MAX_EXAMPLES_PER_INTENT) {
        return;  // Too many examples
    }
    
    // Store example
    strcpy(profile->examples[profile->example_count], example);
    profile->example_count++;
    
    // ===== UPDATE INTENT EMBEDDING =====
    // Get embedding of this example
    int word_indices[50];
    int word_count = tokenize(example, word_indices, 50);
    
    float example_embedding[EMBEDDING_DIM];
    get_sentence_embedding(word_indices, word_count, example_embedding);
    
    // Add to intent's centroid (average of all examples)
    for (int i = 0; i < EMBEDDING_DIM; i++) {
        profile->intent_embedding[i] += example_embedding[i];
    }
    
    // Normalize centroid
    for (int i = 0; i < EMBEDDING_DIM; i++) {
        profile->intent_embedding[i] /= (float)profile->example_count;
    }
}

// Predict intent from new input
IntentPrediction predict_intent_learned(const char* input) {
    IntentPrediction result;
    result.intent[0] = 0;
    result.confidence = 0.0f;
    
    // Get input embedding
    int word_indices[50];
    int word_count = tokenize(input, word_indices, 50);
    
    if (word_count == 0) {
        strcpy(result.intent, "unknown");
        return result;
    }
    
    float input_embedding[EMBEDDING_DIM];
    get_sentence_embedding(word_indices, word_count, input_embedding);
    
    // ===== FIND CLOSEST INTENT =====
    float best_similarity = 0.0f;
    int best_intent_idx = -1;
    
    for (int i = 0; i < intent_count; i++) {
        // Calculate cosine similarity
        int dot = 0;
        int norm_a = 0;
        int norm_b = 0;
        
        for (int j = 0; j < EMBEDDING_DIM; j++) {
            // Use fixed-point math to avoid floating point
            int ia = (int)(input_embedding[j] * 1000);
            int ib = (int)(intents[i].intent_embedding[j] * 1000);
            
            dot += ia * ib;
            norm_a += ia * ia;
            norm_b += ib * ib;
        }
        
        if (norm_a == 0 || norm_b == 0) continue;
        
        int sqrt_a = int_sqrt(norm_a);
        int sqrt_b = int_sqrt(norm_b);
        
        if (sqrt_a == 0 || sqrt_b == 0) continue;
        
        float similarity = (float)dot / (float)(sqrt_a * sqrt_b);
        
        if (similarity > best_similarity) {
            best_similarity = similarity;
            best_intent_idx = i;
        }
    }
    
    if (best_intent_idx >= 0) {
        strcpy(result.intent, intents[best_intent_idx].intent_name);
        result.confidence = best_similarity * 100.0f;
        if (result.confidence > 100.0f) result.confidence = 100.0f;
        intents[best_intent_idx].times_used++;
    } else {
        strcpy(result.intent, "unknown");
    }
    
    return result;
}

// Show what the AI has learned about intents
void show_learned_intents(void) {
    print_string("\n=== Learned Intents ===\n");
    print_string("Total intents: ");
    kprint_dec(intent_count);
    print_string("\n");
    
    for (int i = 0; i < intent_count; i++) {
        print_string("\nIntent: ");
        print_string(intents[i].intent_name);
        print_string(" (used ");
        kprint_dec(intents[i].times_used);
        print_string(" times)\n");
        
        print_string("  Training examples:\n");
        for (int j = 0; j < intents[i].example_count; j++) {
            print_string("    - \"");
            print_string(intents[i].examples[j]);
            print_string("\"\n");
        }
    }
    print_string("\n");
}

// Get current intent count
int get_intent_count(void) {
    return intent_count;
}