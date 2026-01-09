// include/screen.h
#ifndef SCREEN_H
#define SCREEN_H
#include <stdint.h>

void clear_screen();
void print_string(char* str);
void print_char(char c);
void update_cursor(); // Add this if missing
void itoa(unsigned int n, char* s); // <--- Add this line!

#endif