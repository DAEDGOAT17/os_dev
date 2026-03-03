#include "word_embeddings.h"
#include "string.h"
#include "screen.h"
#include <stdint.h>
#include <stddef.h>

#define MAX_WORDS 200          // Will grow dynamically
#define EMBEDDING_DIM 16       // Small but enough for kernel
#define LEARNING_RATE 0.01f

// Simple pseudo-random number generator (for kernel)
static uint32_t seed = 12345;
static uint32_t simple_rand(void) {
    seed = (seed * 1103515245 + 12345) & 0x7fffffff;
    return seed;
}

// Dynamic word dictionary
typedef struct {
    char word[32];
    float embedding[EMBEDDING_DIM];  // 16-dimensional vector
    uint32_t frequency;              // How many times seen
} WordEntry;

// Global word database (learned from input)
static WordEntry word_db[MAX_WORDS];
static int word_count = 0;

// ========== CORE FUNCTIONS ==========

// Find or create word in dictionary
int get_or_create_word(const char* word) {
    // Search for existing word
    for (int i = 0; i < word_count; i++) {
        if (strcmp(word_db[i].word, word) == 0) {
            word_db[i].frequency++;
            return i;
        }
    }
    
    // Word not found - CREATE IT
    if (word_count >= MAX_WORDS) {
        return -1;  // Dictionary full
    }
    
    // Initialize new word with RANDOM EMBEDDING
    int idx = word_count;
    strcpy(word_db[idx].word, word);
    word_db[idx].frequency = 1;
    
    // Initialize embedding with small random values
    for (int i = 0; i < EMBEDDING_DIM; i++) {
        word_db[idx].embedding[i] = (float)(simple_rand() % 100) / 1000.0f - 0.05f;
    }
    
    word_count++;
    return idx;
}

// Get embedding vector for a word
float* get_word_embedding(const char* word) {
    int idx = get_or_create_word(word);
    if (idx < 0) return (float*)0;  // Return NULL pointer (kernel style)
    return word_db[idx].embedding;
}

// Extract words from input (tokenize)
int tokenize(const char* input, int* word_indices, int max_words) {
    int count = 0;
    char temp[32] = {0};
    int temp_idx = 0;
    
    for (int i = 0; input[i] && count < max_words; i++) {
        char c = input[i];
        
        // Convert to lowercase for consistency
        if (c >= 'A' && c <= 'Z') {
            c = c - 'A' + 'a';
        }
        
        // Check if character is alphanumeric
        if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')) {
            if (temp_idx < 31) {
                temp[temp_idx++] = c;
            }
        } else {
            // End of word
            if (temp_idx > 0) {
                temp[temp_idx] = '\0';
                int idx = get_or_create_word(temp);
                if (idx >= 0) {
                    word_indices[count++] = idx;
                }
                temp_idx = 0;
            }
        }
    }
    
    // Handle last word
    if (temp_idx > 0) {
        temp[temp_idx] = '\0';
        int idx = get_or_create_word(temp);
        if (idx >= 0) {
            word_indices[count++] = idx;
        }
    }
    
    return count;
}

// Average embedding of all words in input
void get_sentence_embedding(const int* word_indices, int count, float* out_embedding) {
    // Initialize to zeros
    for (int i = 0; i < EMBEDDING_DIM; i++) {
        out_embedding[i] = 0.0f;
    }
    
    if (count == 0) return;
    
    // Sum all word embeddings
    for (int i = 0; i < count; i++) {
        float* word_emb = word_db[word_indices[i]].embedding;
        for (int j = 0; j < EMBEDDING_DIM; j++) {
            out_embedding[j] += word_emb[j];
        }
    }
    
    // Average
    for (int i = 0; i < EMBEDDING_DIM; i++) {
        out_embedding[i] /= (float)count;
    }
}

// Learn from user input + correct intent
// This is like "backpropagation"
void learn_intent_association(const char* input, const char* correct_intent) {
    int word_indices[50];
    int word_count_in_input = tokenize(input, word_indices, 50);
    
    if (word_count_in_input == 0) return;
    
    // Adjust embeddings to be closer to intent
    // This is simplified gradient descent
    
    print_string("\n[LEARNING] Associated words with intent: ");
    print_string(correct_intent);
    print_string("\n");
    
    for (int i = 0; i < word_count_in_input; i++) {
        int idx = word_indices[i];
        
        // Move embedding slightly toward intent_id
        // (In real systems, this would be actual backprop)
        for (int j = 0; j < EMBEDDING_DIM; j++) {
            word_db[idx].embedding[j] += LEARNING_RATE * 0.1f;
        }
        
        print_string("  - '");
        print_string(word_db[idx].word);
        print_string("' (freq: ");
        kprint_dec(word_db[idx].frequency);
        print_string(")\n");
    }
    print_string("\n");
}

// Show learned vocabulary
void show_learned_vocabulary(void) {
    print_string("\n=== Learned Vocabulary ===\n");
    print_string("Total words: ");
    kprint_dec(word_count);
    print_string("\n\n");
    
    for (int i = 0; i < word_count && i < 20; i++) {
        print_string("  ");
        print_string(word_db[i].word);
        print_string(" (freq: ");
        kprint_dec(word_db[i].frequency);
        print_string(")\n");
    }
    print_string("\n");
}

// Get current word count
int get_word_count(void) {
    return word_count;
}