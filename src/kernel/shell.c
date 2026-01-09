#include "screen.h"
#include "string.h"
#include <stdbool.h>
#include "shell.h"

char shell_buffer[256];
int buffer_idx = 0;

void shell_input(char c) {
    if (c == '\n') {
        shell_buffer[buffer_idx] = '\0';
        print_char('\n');
        
        // Execute command
        if (strcmp(shell_buffer, "help") == 0) {
            print_string("Commands: help, clear, mem\n");
        } else if (strcmp(shell_buffer, "clear") == 0) {
            clear_screen();
        } else if (strcmp(shell_buffer, "mem") == 0) {
            print_string("PMM/VMM Active. 128MB Managed.\n");
        } else {
            print_string("Unknown command.\n");
        }

        print_string("> ");
        buffer_idx = 0;
    } else if (c == '\b' && buffer_idx > 0) {
        buffer_idx--;
        print_char('\b');
    } else if (buffer_idx < 255 && c >= ' ') {
        shell_buffer[buffer_idx++] = c;
        print_char(c);
    }
}