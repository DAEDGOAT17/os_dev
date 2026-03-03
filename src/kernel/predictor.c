#include "predictor.h"
#include "embeddings.h"
#include "string.h"

typedef struct {
    const char* name;
    const char* phrase;
} IntentDef;

static IntentDef intents[] = {

    { "mem",   "show memory status ram usage free" },
    { "ps",    "list running processes active tasks" },
    { "clear", "clear clean screen reset display" },
    { "reboot","restart reboot shutdown system power off" },
    { "help",  "help what can you do commands available" }

};

#define INTENT_COUNT (sizeof(intents)/sizeof(intents[0]))

Prediction predict_intent(const char* input)
{
    Prediction best;
    best.intent[0] = 0;
    best.confidence = 0.0f;

    TextEmbedding input_emb = text_to_embedding(input);
    float best_score = 0.0f;

    for (int i = 0; i < INTENT_COUNT; i++) {

        TextEmbedding intent_emb =
            text_to_embedding(intents[i].phrase);

        float score =
            embedding_similarity(input_emb, intent_emb);

        if (score > best_score) {
            best_score = score;
            strcpy(best.intent, intents[i].name);
        }
    }

    if (best_score == 0.0f) {
        strcpy(best.intent, "unknown");
        best.confidence = 0.0f;
        return best;
    }

    best.confidence = best_score * 100.0f;
    return best;
}