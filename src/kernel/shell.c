#include "screen.h"
#include "string.h"
#include "io.h"
#include <stdbool.h>
#include "shell.h"
#include "kmalloc.h"
#include "task.h"
#include "timer.h"
#include "pci.h"
#include "math.h"
#include "fat32.h"
#include "ata.h"


char shell_buffer[256];
int buffer_idx = 0;
int tab_press_count = 0;

// Helper function to compare command with argument for shell
bool cmd_starts_with(const char* cmd, const char* prefix) {
    int i = 0;
    while (prefix[i] && cmd[i] == prefix[i]) i++;
    return prefix[i] == '\0';
}

void shell_execute(char* cmd) {
    if (cmd[0] == '\0') return;

    // 1. First check for EXACT command matches (File system and common utils)
    if (strcmp(cmd, "ls") == 0) {
        fat32_ls(0);
        return;
    }
    else if (cmd_starts_with(cmd, "ls ")) {
        uint32_t cluster = fat32_resolve_path(cmd + 3);
        if (cluster != 0) fat32_ls(cluster);
        else print_string("Directory not found!\n");
        return;
    }
    else if (cmd_starts_with(cmd, "cd ")) {
        if (fat32_chdir(cmd + 3) != 0) {
            print_string("Directory not found!\n");
        }
        return;
    }
    else if (cmd_starts_with(cmd, "cat ")) {
        int fd = fat32_open(cmd + 4, 'r');
        if (fd >= 0) {
            char buf[513];
            int bytes;
            while ((bytes = fat32_read(fd, buf, 512)) > 0) {
                buf[bytes] = '\0';
                print_string(buf);
            }
            fat32_close(fd);
            print_char('\n');
        } else {
            print_string("File not found!\n");
        }
        return;
    }
    else if (cmd_starts_with(cmd, "touch ")) {
        int fd = fat32_open(cmd + 6, 'w');
        if (fd >= 0) {
            fat32_close(fd);
            print_string("File created.\n");
        } else {
            print_string("Failed to create file.\n");
        }
        return;
    }
    else if (cmd_starts_with(cmd, "write ")) {
        char* space = strstr(cmd + 6, " ");
        if (space) {
            *space = '\0';
            char* file = cmd + 6;
            char* text = space + 1;
            int fd = fat32_open(file, 'w');
            if (fd >= 0) {
                fat32_write(fd, text, strlen(text));
                fat32_close(fd);
                print_string("File written.\n");
            } else {
                print_string("Failed to write to file.\n");
            }
        } else {
            print_string("Usage: write <file> <text>\n");
        }
        return;
    }
    else if (cmd_starts_with(cmd, "rm ")) {
        if (fat32_unlink(cmd + 3) == 0) {
            print_string("File removed.\n");
        } else {
            print_string("Could not remove file.\n");
        }
        return;
    }
    else if (strcmp(cmd, "clear") == 0) {
        clear_screen();
        return;
    }
    else if (strcmp(cmd, "help") == 0) {
        print_string("Available commands:\n");
        print_string("- ls [path]    (List files)\n");
        print_string("- cd <path>    (Change directory)\n");
        print_string("- cat <file>   (Show file contents)\n");
        print_string("- touch <file> (Create empty file)\n");
        print_string("- write <f> <t>(Write text to file)\n");
        print_string("- rm <file>    (Remove file)\n");
        print_string("- clear        (Clear screen)\n");
        print_string("- ps           (List processes)\n");
        print_string("- mem          (Show memory statistics)\n");
        print_string("- reboot       (Restart system)\n\n");
        return;
    }

    else if (strcmp(cmd, "ps") == 0) {
        task_list();
        return;
    }
    else if (strcmp(cmd, "mem") == 0) {
        print_string("Physical Memory: 256 MB\n");
        heap_stats_t stats;
        kmalloc_get_stats(&stats);
        print_string("Heap Used: ");
        kprint_dec(stats.used_size / 1024);
        print_string(" KB / ");
        kprint_dec(stats.total_size / 1024);
        print_string(" KB\n\n");
        return;
    }
    else if (strcmp(cmd, "reboot") == 0) {
        print_string("Rebooting...\n");
        sys_reboot();
        return;
    }
    else {
        print_string("Unknown command. Type 'help' for assistance.\n");
    }
}

//shell input function
void shell_input(char c) {
    if (c == '\n') {
        shell_buffer[buffer_idx] = '\0';
        print_char('\n');
        shell_execute(shell_buffer);
        print_string("JARVIS $ ");
        buffer_idx = 0;
        return;
    }

    if (c == '\b' && buffer_idx > 0) {
        buffer_idx--;
        print_char('\b');
        print_char(' ');
        print_char('\b');
        return;
    }

    if (buffer_idx < 255 && c >= ' ') {
        shell_buffer[buffer_idx++] = c;
        print_char(c);
    }
}

void shell_task(void) {
    static bool welcomed = false;
    if (!welcomed) {
        print_string("\nJARVIS OS - Standard Mode\n");
        print_string("Type 'help' for a list of commands.\n\n");
        print_string("JARVIS $ ");
        welcomed = true;
    }
}
