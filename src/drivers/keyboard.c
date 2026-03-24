#include "io.h"
#include "shell.h"
#include "screen.h"
#include <stdint.h>
#include <stdbool.h>

static bool shift_pressed = false;
static bool caps_lock = false;

unsigned char kbd_us[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0, '\\',
    'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' '
};

unsigned char kbd_us_shift[128] = {
    0,  27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '\"', '~', 0, '|',
    'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0, '*', 0, ' '
};

void keyboard_handler() {
    uint8_t scancode = inb(0x60);

    if (scancode & 0x80) {
        uint8_t released = scancode & 0x7F;
        if (released == 0x2A || released == 0x36) shift_pressed = false;
    } else {
        if (scancode == 0x2A || scancode == 0x36) {
            shift_pressed = true;
        } else if (scancode == 0x3A) {
            caps_lock = !caps_lock;
        } else if (scancode == 0x49) { // Page Up
            scroll_up();
        } else if (scancode == 0x51) { // Page Down
            scroll_down();
        } else {
            char c = shift_pressed ? kbd_us_shift[scancode] : kbd_us[scancode];
            if (caps_lock && c >= 'a' && c <= 'z') c -= 32;
            if (c > 0) shell_input(c);
        }
    }
    // Inside keyboard_handler()

    
    outb(0x20, 0x20);
}

