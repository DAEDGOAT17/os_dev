#ifndef EMBEDDINGS_H
#define EMBEDDINGS_H

#define VOCAB_SIZE 36

typedef struct {
    int embedding[VOCAB_SIZE];
} TextEmbedding;

TextEmbedding text_to_embedding(const char* text);
float embedding_similarity(TextEmbedding a, TextEmbedding b);

#endif