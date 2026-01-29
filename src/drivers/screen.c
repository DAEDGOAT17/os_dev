#include "screen.h"
#include "io.h"

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

void scroll() {
    char *vga = (char*)0xB8000;
    
    // Move all lines up by one (copy line 1 to line 0, line 2 to line 1, etc.)
    // We scroll at line 22 to keep lines 23-24 as bottom margin (2 lines of space)
    for (int i = 0; i < 80 * 22; i++) {
        vga[i * 2] = vga[(i + 80) * 2];
        vga[i * 2 + 1] = vga[(i + 80) * 2 + 1];
    }
    
    // Clear line 22 (where new content will appear)
    for (int i = 80 * 22; i < 80 * 23; i++) {
        vga[i * 2] = ' ';
        vga[i * 2 + 1] = 0x07;
    }
    
    // Move cursor to the beginning of line 22 (keeping 2 lines at bottom as margin)
    cursor = 80 * 22;
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
    
    // Check if we need to scroll - trigger at line 23 to maintain bottom space
    if (cursor >= 80 * 23) {
        scroll();
    }
    
    update_cursor();
}

void print_string(char* str) {
    for (int i = 0; str[i] != '\0'; i++) {
        print_char(str[i]);
    }
}

void kprint_hex(uint32_t n) {
    char* chars = "0123456789ABCDEF";
    char buffer[11];
    buffer[0] = '0';
    buffer[1] = 'x';
    buffer[10] = '\0';

    for (int i = 0; i < 8; i++) {
        buffer[9 - i] = chars[n & 0xF];
        n >>= 4;
    }
    print_string(buffer);
}
void kprint_dec(uint32_t num) {
    char buf[12];
    int i = 0;

    if (num == 0) {
        print_char('0');
        return;
    }

    while (num > 0) {
        buf[i++] = '0' + (num % 10);
        num /= 10;
    }

    while (i--)
        print_char(buf[i]);
}
