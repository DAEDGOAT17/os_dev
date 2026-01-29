#include "screen.h"
#include "io.h"
#include <stdint.h>

#define MAX_HISTORY 1000
#define SCREEN_ROWS 25
#define SCREEN_COLS 80
#define MARGIN_ROWS 2
#define VISIBLE_ROWS (SCREEN_ROWS - MARGIN_ROWS)

static uint16_t history_buffer[MAX_HISTORY][SCREEN_COLS];
static int total_lines_stored = 0; // Cumulative count of lines ever written
static int current_line_idx = 0;   // Circular index in buffer where we are currently writing
static int current_col = 0;
static int scroll_offset = 0;      // How many lines scrolled UP from the current line

int cursor = 0;

static void clear_history_line(int idx) {
    for (int i = 0; i < SCREEN_COLS; i++) {
        history_buffer[idx][i] = (0x07 << 8) | ' ';
    }
}

void refresh_screen() {
    uint16_t *vga = (uint16_t*)0xB8000;
    
    // The last line to show is total_lines_stored - scroll_offset
    // We want to show VISIBLE_ROWS lines ending at that line
    int end_history_total = total_lines_stored - scroll_offset;
    int start_history_total = end_history_total - (VISIBLE_ROWS - 1);
    if (start_history_total < 0) start_history_total = 0;

    // 1. Render visible history
    for (int r = 0; r < VISIBLE_ROWS; r++) {
        int history_total_idx = start_history_total + r;
        
        if (history_total_idx <= total_lines_stored) {
            // Map total index to circular buffer index
            int circular_idx = history_total_idx % MAX_HISTORY;
            
            for (int c = 0; c < SCREEN_COLS; c++) {
                int vga_idx = r * SCREEN_COLS + c;
                if (c == SCREEN_COLS - 1) continue; // Skip scrollbar col
                vga[vga_idx] = history_buffer[circular_idx][c];
            }
        } else {
            // Fill with empty if we haven't reached this total line yet
            for (int c = 0; c < SCREEN_COLS - 1; c++) {
                vga[r * SCREEN_COLS + c] = (0x07 << 8) | ' ';
            }
        }
    }

    // 2. Clear Margin Rows
    for (int r = VISIBLE_ROWS; r < SCREEN_ROWS; r++) {
        for (int c = 0; c < SCREEN_COLS; c++) {
            vga[r * SCREEN_COLS + c] = (0x07 << 8) | ' ';
        }
    }

    // 3. Render Scrollbar (Col 79)
    int max_scroll = total_lines_stored - (VISIBLE_ROWS - 1);
    if (max_scroll < 0) max_scroll = 0;

    for (int r = 0; r < VISIBLE_ROWS; r++) {
        int vga_idx = r * SCREEN_COLS + (SCREEN_COLS - 1);
        if (max_scroll > 0) {
            int handle_r = (VISIBLE_ROWS - 1) - (scroll_offset * (VISIBLE_ROWS - 1) / max_scroll);
            if (r == handle_r) {
                vga[vga_idx] = (0x0F << 8) | 219; // White block
            } else {
                vga[vga_idx] = (0x08 << 8) | 179; // Dark track
            }
        } else {
            vga[vga_idx] = (0x08 << 8) | 179;
        }
    }
}

void update_cursor() {
    if (scroll_offset > 0) {
        // Hide cursor via hardware registers
        outb(0x3D4, 0x0A);
        outb(0x3D5, 0x20); // Bit 5 hides cursor
        return;
    }

    // Show cursor (set scanline 14-15)
    outb(0x3D4, 0x0A); outb(0x3D5, 0x0E);
    outb(0x3D4, 0x0B); outb(0x3D5, 0x0F);

    // Position calculation
    int end_history_total = total_lines_stored;
    int start_history_total = end_history_total - (VISIBLE_ROWS - 1);
    if (start_history_total < 0) start_history_total = 0;

    int cursor_row = total_lines_stored - start_history_total;
    int pos = cursor_row * SCREEN_COLS + current_col;

    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)(pos & 0xFF));
    
    cursor = pos; // Sync global
}

void clear_screen() {
    for (int i = 0; i < MAX_HISTORY; i++) clear_history_line(i);
    total_lines_stored = 0;
    current_line_idx = 0;
    current_col = 0;
    scroll_offset = 0;
    refresh_screen();
    update_cursor();
}

void scroll_up() {
    int max_scroll = total_lines_stored - (VISIBLE_ROWS - 1);
    if (max_scroll < 0) max_scroll = 0;
    
    if (scroll_offset < max_scroll && scroll_offset < MAX_HISTORY - VISIBLE_ROWS) {
        scroll_offset++;
        refresh_screen();
        update_cursor();
    }
}

void scroll_down() {
    if (scroll_offset > 0) {
        scroll_offset--;
        refresh_screen();
        update_cursor();
    }
}

void print_char(char c) {
    if (c == '\b') {
        if (current_col > 0) {
            current_col--;
            history_buffer[current_line_idx][current_col] = (0x07 << 8) | ' ';
        }
    } else if (c == '\n') {
        current_col = 0;
        total_lines_stored++;
        current_line_idx = total_lines_stored % MAX_HISTORY;
        clear_history_line(current_line_idx);
    } else {
        if (current_col < SCREEN_COLS - 1) {
            history_buffer[current_line_idx][current_col] = (0x07 << 8) | c;
            current_col++;
        } else {
            // Auto wrap
            print_char('\n');
            print_char(c);
            return;
        }
    }

    // Always follow output to the bottom
    scroll_offset = 0;
    refresh_screen();
    update_cursor();
}

void print_string(char* str) {
    while (*str) print_char(*str++);
}

void kprint_hex(uint32_t n) {
    char* hex = "0123456789ABCDEF";
    print_string("0x");
    for (int i = 28; i >= 0; i -= 4) {
        print_char(hex[(n >> i) & 0xF]);
    }
}

void kprint_dec(uint32_t n) {
    if (n == 0) {
        print_char('0');
        return;
    }
    char buf[11];
    int i = 0;
    while (n > 0) {
        buf[i++] = (n % 10) + '0';
        n /= 10;
    }
    while (--i >= 0) print_char(buf[i]);
}
