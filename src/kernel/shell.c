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
#include "predictor.h"
#include "nlp.h"
#include <stdint.h>

char shell_buffer[256];
int buffer_idx = 0;
int tab_press_count = 0;

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
        print_string("  crash      - Trigger a page fault\n");
        print_string("  pci-scan   - Scan PCI bus for multimedia audio controllers\n");
        print_string("  mat-test   - Run a small fixed-point matrix multiply test\n");
        print_string("  nlp-test   - Run embedded NLP intent rule based classifier (incomplete) \n");
        print_string("\n");
        
    } else if (strcmp(cmd, "crash") == 0) {
        print_string("Triggering page fault...\n");
        uint32_t *ptr = (uint32_t*)0xFFFF0000; // Address not mapped
        *ptr = 0xDEADBEEF;
        
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
        sys_reboot();

    } else if (strcmp(cmd, "pci-scan") == 0) {
        // Run PCI scan and print multimedia audio controllers
        pci_scan_multimedia();

    } else if (strcmp(cmd, "mat-test") == 0) {
        // Run a small fixed-point 2x2 matrix multiply test
        const uint8_t FRAC = 16;
        const uint32_t n = 2;
        int32_t A[4] = { (1<<FRAC), (32768), (-1<<FRAC), (2<<FRAC) };
        int32_t B[4] = { (2<<FRAC), ( (uint32_t)(1<<FRAC) + (uint32_t)(32768) ), (16384), (-32768) };
        int32_t C[4] = {0,0,0,0};
        mat_mul_fixed(A, B, C, n, FRAC);
        print_string("Matrix multiply test (fixed 16.16):\n");
        for (uint32_t i = 0; i < n; ++i) {
            for (uint32_t j = 0; j < n; ++j) {
                print_string("C["); kprint_dec(i); print_string("]["); kprint_dec(j); print_string("] = ");
                // print fixed-point value
                int64_t t = C[i*n + j];
                int neg = 0;
                if (t < 0) { neg = 1; t = -t; }
                uint32_t intpart = (uint32_t)(t >> FRAC);
                uint32_t fracmask = ((uint32_t)1 << FRAC) - 1;
                uint32_t frac = (uint32_t)(t & fracmask);
                uint32_t frac3 = (uint32_t)(((uint64_t)frac * 1000 + (1ULL << (FRAC-1))) >> FRAC);
                if (neg) print_string("-");
                kprint_dec(intpart);
                print_string(".");
                if (frac3 < 100) { if (frac3 < 10) print_string("00"); else print_string("0"); }
                kprint_dec(frac3);
                print_string("\n");
            }
        }
        print_string("End matrix test.\n");
        
    } else if (strcmp(cmd, "heap") == 0) {
        heap_stats_t stats;
        kmalloc_get_stats(&stats);

        print_string("\n=== Kernel Heap ===\n\n");

        print_string("  Total : ");
        kprint_dec(stats.total_size / (1024 * 1024));
        print_string(" MB\n");

        print_string("  Used  : ");
        kprint_dec(stats.used_size / (1024 * 1024));
        print_string(" MB (roughly)\n");

        print_string("  Free  : ");
        kprint_dec(stats.free_size / (1024 * 1024));
        print_string(" MB\n");
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
    }else if (cmd_starts_with(cmd, "nlp ")) {

        nlp_result_t res = nlp_process(cmd + 4);
        print_string("\n  intent ---> shell command  : ");
        print_string(res.mapped_command);
        print_string("\n\n");
    }else if (cmd[0] != '\0') {
        print_string("\n  Error: Unknown command '");
        print_string(cmd);
        print_string("'\n  Type 'help' for available commands.\n\n");
    }
}

//shell input function
void shell_input(char c) {

    /* ================= TAB ================= */
    if (c == '\t') {

        shell_buffer[buffer_idx] = '\0';

        const char* matches[16];
        int match_count = predictor_get_matches(shell_buffer, matches, 16);

        if (match_count == 0)
            return;

        /* ---- SINGLE TAB ---- */
        if (tab_press_count == 0) {

            char prefix[64];
            int len = predictor_longest_common_prefix(shell_buffer, prefix);

            if (len > buffer_idx) {

                for (int i = buffer_idx; i < len && i < 255; i++) {
                    shell_buffer[i] = prefix[i];
                    print_char(prefix[i]);
                }

                buffer_idx = len;
                shell_buffer[len] = '\0';
            }

            tab_press_count = 1;
        }

        /* ---- DOUBLE TAB ---- */
        else {

            print_string("\n");

            for (int i = 0; i < match_count; i++) {
                print_string(matches[i]);
                print_string("  ");
            }

            print_string("\nJARVIS $ ");
            print_string(shell_buffer);

            tab_press_count = 0;
        }

        return;
    }

    /* ================= ENTER ================= */
    if (c == '\n') {

        shell_buffer[buffer_idx] = '\0';
        print_char('\n');

        const char* suggestion = predictor_suggest(shell_buffer);

        if (suggestion && strcmp(shell_buffer, suggestion) != 0) {
            print_string("  Did you mean ----> ");
            print_string(suggestion);
            print_string("\n");
        }

        shell_execute(shell_buffer);
        predictor_learn(shell_buffer);

        print_string("JARVIS $ ");

        buffer_idx = 0;
        tab_press_count = 0;
        return;
    }

    /* ================= BACKSPACE ================= */
    if (c == '\b' && buffer_idx > 0) {

        buffer_idx--;

        /* Proper erase */
        print_char('\b');
        print_char(' ');
        print_char('\b');

        tab_press_count = 0;
        return;
    }

    /* ================= NORMAL CHAR ================= */
    if (buffer_idx < 255 && c >= ' ') {

        shell_buffer[buffer_idx++] = c;
        print_char(c);

        tab_press_count = 0;
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
