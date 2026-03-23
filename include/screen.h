#ifndef SCREEN_H
#define SCREEN_H

#include <stdint.h>

typedef enum {
  VGA_COLOR_BLACK = 0,
  VGA_COLOR_BLUE = 1,
  VGA_COLOR_GREEN = 2,
  VGA_COLOR_CYAN = 3,
  VGA_COLOR_RED = 4,
  VGA_COLOR_MAGENTA = 5,
  VGA_COLOR_BROWN = 6,
  VGA_COLOR_LIGHT_GREY = 7,
  VGA_COLOR_DARK_GREY = 8,
  VGA_COLOR_LIGHT_BLUE = 9,
  VGA_COLOR_LIGHT_GREEN = 10,
  VGA_COLOR_LIGHT_CYAN = 11,
  VGA_COLOR_LIGHT_RED = 12,
  VGA_COLOR_LIGHT_MAGENTA = 13,
  VGA_COLOR_LIGHT_BROWN = 14,
  VGA_COLOR_WHITE = 15,
} vga_color_t;

void clear_screen();
void print_char(char c);
void print_string(char *str);
void print_string_color(char *str, vga_color_t fg, vga_color_t bg);
void set_color(vga_color_t fg, vga_color_t bg);
void update_cursor();
void kprint_hex(uint32_t n);
void kprint_dec(uint32_t num);
extern int cursor;
void scroll_up();
void scroll_down();

#endif