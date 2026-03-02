#include <stdint.h>
#include "string.h"


int strcmp(const char *s1, const char *s2)
{
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char *)s1 - *(unsigned char *)s2;
}


int strlen(const char *s)
{
    int len = 0;
    while (s[len])
        len++;
    return len;
}



char* strstr(const char* haystack, const char* needle)
{
    if (!*needle)
        return (char*)haystack;

    while (*haystack) {

        const char* h = haystack;
        const char* n = needle;

        while (*h && *n && (*h == *n)) {
            h++;
            n++;
        }

        if (!*n)
            return (char*)haystack;

        haystack++;
    }

    return 0;
}