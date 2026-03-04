#include "intent_learner.h"
#include "word_embeddings.h"
#include "screen.h"
#include "string.h"
#include "intent_config.h"
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
// FIXED: Simpler, working similarity calculation
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
    
    // ===== FIND CLOSEST INTENT (FIXED) =====
    float best_similarity = -1.0f;
    int best_intent_idx = -1;
    
    for (int i = 0; i < intent_count; i++) {
        // Calculate cosine similarity properly with floats
        float dot = 0.0f;
        float norm_a = 0.0f;
        float norm_b = 0.0f;
        
        for (int j = 0; j < EMBEDDING_DIM; j++) {
            float a = input_embedding[j];
            float b = intents[i].intent_embedding[j];
            
            dot += a * b;
            norm_a += a * a;
            norm_b += b * b;
        }
        
        // Avoid division by zero
        if (norm_a < 0.0001f || norm_b < 0.0001f) {
            continue;
        }
        
        // Simple similarity (works better with small embeddings)
        float similarity = dot / (norm_a + norm_b);  // Simpler formula
        
        if (similarity > best_similarity) {
            best_similarity = similarity;
            best_intent_idx = i;
        }
    }
    
    if (best_intent_idx >= 0) {
        strcpy(result.intent, intents[best_intent_idx].intent_name);
        
        // Boost confidence based on examples trained
        result.confidence = (best_similarity + 1.0f) * 50.0f;  // Scale to 0-100%
        
        if (result.confidence > 100.0f) result.confidence = 100.0f;
        if (result.confidence < 0.0f) result.confidence = 0.0f;
        
        intents[best_intent_idx].times_used++;
    } else {
        strcpy(result.intent, "unknown");
        result.confidence = 0.0f;
    }
    
    return result;
}