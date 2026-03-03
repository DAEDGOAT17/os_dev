#include "embeddings.h"
#include "string.h"

static const char* vocab[VOCAB_SIZE] = {
    "memory","ram","mem","show","check","how","much",
    "task","process","list","ps","running","active",
    "clear","screen","clean","reset",
    "reboot","restart","shutdown","power","off",
    "help","what","can","do","available",
    "predict","next","suggest","recommend",
    "uptime","long","time","system","status"
};

TextEmbedding text_to_embedding(const char* text)
{
    TextEmbedding emb;

    for (int i = 0; i < VOCAB_SIZE; i++)
        emb.embedding[i] = 0;

    for (int i = 0; i < VOCAB_SIZE; i++) {
        if (strstr(text, vocab[i])) {
            emb.embedding[i] = 1;
        }
    }

    return emb;
}

float embedding_similarity(TextEmbedding a, TextEmbedding b)
{
    int dot = 0;
    int sum_a = 0;
    int sum_b = 0;

    for (int i = 0; i < VOCAB_SIZE; i++) {
        dot += a.embedding[i] * b.embedding[i];
        sum_a += a.embedding[i];
        sum_b += b.embedding[i];
    }

    if (sum_a == 0 || sum_b == 0)
        return 0.0f;

    return (float)dot / (float)sum_a;
}