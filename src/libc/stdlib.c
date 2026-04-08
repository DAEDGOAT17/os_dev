#include "stdlib.h"

// Minimal strtol implementation for lwIP
long strtol(const char *nptr, char **endptr, int base) {
    long result = 0;
    while (*nptr >= '0' && *nptr <= '9') {
        result = (result * (base ? base : 10)) + (*nptr - '0');
        nptr++;
    }
    if (endptr) {
        *endptr = (char*)nptr;
    }
    return result;
}

int atoi(const char *nptr) {
    return (int)strtol(nptr, NULL, 10);
}
