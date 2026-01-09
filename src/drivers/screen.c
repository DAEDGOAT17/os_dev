#include "screen.h"
#include "io.h"

// Define the cursor globally so the linker can find it
int cursor = 0;

void update_cursor() {
    uint16_t pos = cursor;
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)(pos & 0xFF));
}

void clear_screen() {
    char *vga = (char*)0xB8000;
    for (int i = 0; i < 80 * 25; i++) {
        vga[i * 2] = ' ';
        vga[i * 2 + 1] = 0x07;
    }
    cursor = 0;
    update_cursor();
}

void print_char(char c) {
    char *vga = (char*)0xB8000;

    if (c == '\b') { // Backspace
        if (cursor > 0) {
            cursor--;
            vga[cursor * 2] = ' ';
            vga[cursor * 2 + 1] = 0x07;
        }
    } else if (c == '\n') { // Newline
        cursor = ((cursor / 80) + 1) * 80;
    } else {
        vga[cursor * 2] = c;
        vga[cursor * 2 + 1] = 0x07;
        cursor++;
    }
    update_cursor();
}

void print_string(char* str) {
    for (int i = 0; str[i] != '\0'; i++) {
        print_char(str[i]);
    }
}