#ifndef TOKENIZER_H
#define TOKENIZER_H

#include <stdint.h>

void tokenizer_init(void);
int tokenizer_encode(const char* text, int* tokens, int max_tokens);
void tokenizer_decode(int token, char* text);

#endif
