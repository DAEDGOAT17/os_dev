#include "tokenizer.h"
#include "string.h"

void tokenizer_init(void) {
    // Basic initialization for byte-level tokenizer/BPE merges
    // Could eventually load tokenizer.bin using fat32_read here
}

int tokenizer_encode(const char* text, int* tokens, int max_tokens) {
    int count = 0;
    while (*text && count < max_tokens) {
        tokens[count++] = (uint8_t)(*text++);
    }
    return count;
}

void tokenizer_decode(int token, char* text) {
    // For a byte-level tokenizer, token directly maps to standard ASCII char
    text[0] = (char)(token & 0xFF);
    text[1] = '\0';
}
