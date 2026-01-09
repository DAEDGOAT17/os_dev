#include "screen.h"
#include "io.h"

int cursor_pos = 0;

void update_cursor() {
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)((cursor_pos >> 8) & 0xFF));
    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)(cursor_pos & 0xFF));
}

void clear_screen() {
    char *vga = (char*)0xB8000;
    for (int i = 0; i < 80 * 25; i++) {
        vga[i * 2] = ' ';
        vga[i * 2 + 1] = 0x07;
    }
    cursor_pos = 0;
    update_cursor();
}

void print_char(char c) {
    char *vga = (char*)0xB8000;
    if (c == '\n') {
        cursor_pos = ((cursor_pos / 80) + 1) * 80;
    } else {
        vga[cursor_pos * 2] = c;
        vga[cursor_pos * 2 + 1] = 0x07;
        cursor_pos++;
    }
    update_cursor();
}

void print_string(char* str) {
    for (int i = 0; str[i] != '\0'; i++) print_char(str[i]);
}

void itoa(unsigned int n, char* s) {
    int i = 0;
    if (n == 0) s[i++] = '0';
    else {
        while (n > 0) { s[i++] = (n % 10) + '0'; n /= 10; }
    }
    s[i] = '\0';
    for (int j = 0; j < i / 2; j++) {
        char t = s[j]; s[j] = s[i-j-1]; s[i-j-1] = t;
    }
}