#include "screen.h"
#include "string.h"
#include "io.h"
#include <stdbool.h>
#include "shell.h"
#include "kmalloc.h"
#include "task.h"
#include "timer.h"

char shell_buffer[256];
int buffer_idx = 0;

// Helper function to compare command with argument for shell
bool cmd_starts_with(const char* cmd, const char* prefix) {
    int i = 0;
    while (prefix[i] && cmd[i] == prefix[i]) i++;
    return prefix[i] == '\0';
}
//shell execute function
void shell_execute(char* cmd) {
    if (strcmp(cmd, "help") == 0) {
        print_string("\n=== Available Commands ===\n\n");
        print_string("  help       - Show this message\n");
        print_string("  clear      - Clear screen\n");
        print_string("  mem        - Show system memory status\n");
        print_string("  heap       - Show kernel heap usage\n");
        print_string("  echo <msg> - Print message\n");
        print_string("  version    - Show OS version\n");
        print_string("  ps         - Show process list\n");
        print_string("  uptime     - Show uptime\n");
        print_string("  reboot     - Restart system\n");
        print_string("\n");
        
    } else if (strcmp(cmd, "clear") == 0) {
        clear_screen();
        
    } else if (strcmp(cmd, "mem") == 0) {
        print_string("\n=== Memory Status ===\n\n");
        print_string("  Physical Memory : 256 MB\n");
        print_string("  Paging          : Enabled\n");
        print_string("  Page Size       : 4096 bytes\n");
        print_string("  Kernel Heap     : Active\n");
        print_string("\n");
        
    } else if (cmd_starts_with(cmd, "echo ")) {
        // Print everything after "echo "
        print_string(cmd + 5);
        print_char('\n');
        
    } else if (strcmp(cmd, "version") == 0) {
        print_string("\n=== JARVIS OS ===\n\n");
        print_string("  Version      : v0.1 Alpha\n");
        print_string("  Build        : January 2026\n");
        print_string("  Architecture : i386\n");
        print_string("\n");
        
    } else if (strcmp(cmd, "reboot") == 0) {
        print_string("Rebooting...\n");
        // Pulse the CPU reset line
        uint8_t temp;
        asm volatile("cli");
        do {
            temp = inb(0x64);
            if (temp & 0x01) inb(0x60);
        } while (temp & 0x02);
        outb(0x64, 0xFE);
        asm volatile("hlt");
        
    } else if (strcmp(cmd, "heap") == 0) {
        heap_stats_t stats;
        kmalloc_get_stats(&stats);

        print_string("\n=== Kernel Heap ===\n\n");

        print_string("  Total : ");
        kprint_dec(stats.total_size);
        print_string(" bytes\n");

        print_string("  Used  : ");
        kprint_dec(stats.used_size);
        print_string(" bytes\n");

        print_string("  Free  : ");
        kprint_dec(stats.free_size);
        print_string(" bytes\n");
        print_string("\n");
    } else if (strcmp(cmd, "ps") == 0) {
        print_string("\n");
        task_list();
        print_string("\n");
    } else if (strcmp(cmd, "uptime") == 0) {
        uint32_t h, m, s;
        timer_get_uptime(&h, &m, &s);
        print_string("\n  System Uptime: ");
        kprint_dec(h);
        print_string("h ");
        kprint_dec(m);
        print_string("m ");
        kprint_dec(s);
        print_string("s\n\n");
    } else if (cmd[0] != '\0') {
        print_string("\n  Error: Unknown command '");
        print_string(cmd);
        print_string("'\n  Type 'help' for available commands.\n\n");
    }
}

//shell input function
void shell_input(char c) {
    if (c == '\n') {
        shell_buffer[buffer_idx] = '\0';
        print_char('\n');
        
        // Execute command
        shell_execute(shell_buffer);
        
        // Show prompt
        print_string("JARVIS $ ");
        buffer_idx = 0;
        
    } else if (c == '\b' && buffer_idx > 0) {
        buffer_idx--;
        print_char('\b');
        
    } else if (buffer_idx < 255 && c >= ' ') {
        shell_buffer[buffer_idx++] = c;
        print_char(c);
    }
}

//shell task function
void shell_task(void) {
    print_string("\n  Welcome to JARVIS OS!\n  Type 'help' for available commands.\n\n");
    print_string("JARVIS $ ");

    while (1) {
        // Shell runs passively
        // Keyboard IRQ feeds shell_input()
        task_sleep(1);
    }
}
