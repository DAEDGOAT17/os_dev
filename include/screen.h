#ifndef SCREEN_H
#define SCREEN_H

#include <stdint.h>

void clear_screen();
void print_char(char c);
void print_string(char* str);
void update_cursor();
void kprint_hex(uint32_t n);
void kprint_dec(uint32_t num);
extern int cursor; 
void scroll();     

#endif