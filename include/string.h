#ifndef STRING_H
#define STRING_H

#include <stddef.h>

int strcmp(const char *s1, const char *s2);
size_t strlen(const char *s);
char *strcpy(char *dest, const char *src);
void *memcpy(void *dest, const void *src, size_t n);

#endif