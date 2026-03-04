#ifndef WORD_EMBEDDINGS_H
#define WORD_EMBEDDINGS_H

#include <stdint.h>

#define EMBEDDING_DIM 32

// Get embedding for a word (creates it if doesn't exist)
float* get_word_embedding(const char* word);

// Tokenize input into word indices
int tokenize(const char* input, int* word_indices, int max_words);

// Get averaged embedding for entire sentence
void get_sentence_embedding(const int* word_indices, int count, float* out_embedding);

// Learn from user corrections
void learn_intent_association(const char* input, const char* correct_intent);

// Debug: show what the AI learned
void show_learned_vocabulary(void);
int get_or_create_word(const char *word);

// Get total word count
int get_word_count(void);

#endif