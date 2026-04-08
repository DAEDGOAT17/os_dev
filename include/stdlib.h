#ifndef STDLIB_H
#define STDLIB_H

#include <stdint.h>
#include <stddef.h>

long strtol(const char *nptr, char **endptr, int base);
int atoi(const char *nptr);

#endif
