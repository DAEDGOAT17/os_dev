#ifndef SCREEN_H
#define SCREEN_H

#include <stdint.h>

void clear_screen();
void print_char(char c);
void print_string(char* str);
void update_cursor();

#endif