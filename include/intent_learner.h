#ifndef INTENT_LEARNER_H
#define INTENT_LEARNER_H

#include "word_embeddings.h"

typedef struct {
    char intent[32];
    float confidence;
} IntentPrediction;

// Train: teach the AI about an intent
void train_intent(const char* intent_name, const char* example);

// Predict: what intent does this input belong to?
IntentPrediction predict_intent_learned(const char* input);

// Debug: show what was learned
void show_learned_intents(void);

// Get current intent count
int get_intent_count(void);

#endif