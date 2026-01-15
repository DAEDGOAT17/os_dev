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
        print_string("Available Commands:\n");
        print_string("  help       - Show this message\n");
        print_string("  clear      - Clear screen\n");
        print_string("  mem        - Show system memory status\n");
        print_string("  heap       - Show kernel heap usage\n");
        print_string("  echo <msg> - Print message\n");
        print_string("  version    - Show OS version\n");
        print_string("  ps         - Show process list\n");
        print_string("  uptime     - Show uptime\n");
        print_string("  reboot     - Restart system\n");
        
    } else if (strcmp(cmd, "clear") == 0) {
        clear_screen();
        
    } else if (strcmp(cmd, "mem") == 0) {
        print_string("Memory Status\n");
        print_string("-------------\n");
        print_string("Physical Memory : 128 MB\n");      // from multiboot later
        print_string("Paging          : Enabled\n");
        print_string("Page Size       : 4096 bytes\n");
        print_string("Kernel Heap     : Active\n");
        
    } else if (cmd_starts_with(cmd, "echo ")) {
        // Print everything after "echo "
        print_string(cmd + 5);
        print_char('\n');
        
    } else if (strcmp(cmd, "version") == 0) {
        print_string("JARVIS OS v0.1 Alpha\n");
        print_string("Build: January 2026\n");
        print_string("Architecture: i386\n");
        
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

        print_string("Kernel Heap\n");
        print_string("-----------\n");

        print_string("Total : ");
        kprint_dec(stats.total_size);
        print_string(" bytes\n");

        print_string("Used  : ");
        kprint_dec(stats.used_size);
        print_string(" bytes\n");

        print_string("Free  : ");
        kprint_dec(stats.free_size);
        print_string(" bytes\n");
    } else if (strcmp(cmd, "ps") == 0) {
        task_list();
    } else if (strcmp(cmd, "uptime") == 0) {
        uint32_t h, m, s;
        timer_get_uptime(&h, &m, &s);
        print_string("Uptime: ");
        kprint_dec(h);
        print_string("h ");
        kprint_dec(m);
        print_string("m ");
        kprint_dec(s);
        print_string("s\n");
    } else if (cmd[0] != '\0') {
        print_string("Unknown command: ");
        print_string(cmd);
        print_string("\nType 'help' for available commands.\n");
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

//shell task function
void shell_task(void) {
    print_string("> ");

    while (1) {
        // Shell runs passively
        // Keyboard IRQ feeds shell_input()
        task_sleep(1);
    }
}
