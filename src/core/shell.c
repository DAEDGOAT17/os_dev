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
#include "pmm.h"


char shell_buffer[256];
int buffer_idx = 0;
const char* commands[] = {
    "ls", "cd", "cat", "touch", "write", "rm", "mkdir", "rmdir", "clear", "help", "ps", "mem", "reboot", "sysinfo", "cpuid", "arch", NULL
};

// Static variables for filename completion state
static char tc_best_match[64];
static int tc_match_count = 0;
static char tc_prefix[64];
static int tc_prefix_len = 0;
static bool tc_is_dir = false;

void tc_callback(const char* name, uint8_t attr, uint32_t size, uint32_t cluster) {
    (void)size;
    (void)cluster;
    if (strncasecmp(name, tc_prefix, tc_prefix_len) == 0) {
        if (tc_match_count == 0) {
            strcpy(tc_best_match, name);
            tc_is_dir = (attr & FAT_ATTR_DIRECTORY) != 0;
        } else {
            // Find common prefix (case insensitive check, but keep case from tc_best_match or input)
            // Simplified: we'll follow the case of the first match
            int j = 0;
            while (tc_best_match[j] && name[j]) {
                char c1 = tc_best_match[j];
                char c2 = name[j];
                if (c1 >= 'A' && c1 <= 'Z') c1 += 32;
                if (c2 >= 'A' && c2 <= 'Z') c2 += 32;
                if (c1 != c2) break;
                j++;
            }
            tc_best_match[j] = '\0';
            if (tc_is_dir && !(attr & FAT_ATTR_DIRECTORY)) tc_is_dir = false;
        }
        tc_match_count++;
    }
}

static void cmd_cpuid() {
    uint32_t eax, ebx, ecx, edx;

    // Leaf 0: Vendor string
    asm volatile("cpuid"
        : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
        : "a"(0));
    char vendor[13];
    vendor[0]  = ebx & 0xFF; vendor[1]  = (ebx >> 8) & 0xFF;
    vendor[2]  = (ebx >> 16) & 0xFF; vendor[3]  = (ebx >> 24) & 0xFF;
    vendor[4]  = edx & 0xFF; vendor[5]  = (edx >> 8) & 0xFF;
    vendor[6]  = (edx >> 16) & 0xFF; vendor[7]  = (edx >> 24) & 0xFF;
    vendor[8]  = ecx & 0xFF; vendor[9]  = (ecx >> 8) & 0xFF;
    vendor[10] = (ecx >> 16) & 0xFF; vendor[11] = (ecx >> 24) & 0xFF;
    vendor[12] = '\0';

    set_text_color(MAKE_COLOR(COLOR_LIGHT_CYAN, COLOR_BLACK));
    print_string("CPUID Information\n");
    print_string("-----------------\n");
    reset_text_color();
    print_string("CPU Vendor    : "); print_string(vendor); print_char('\n');

    // Leaf 1: Feature flags
    asm volatile("cpuid"
        : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
        : "a"(1));
    uint32_t family  = ((eax >> 8) & 0xF) + ((eax >> 20) & 0xFF);
    uint32_t model   = ((eax >> 4) & 0xF) | (((eax >> 16) & 0xF) << 4);
    uint32_t stepping = eax & 0xF;
    print_string("Family/Model  : "); kprint_dec(family);
    print_string("/"); kprint_dec(model);
    print_string("  Stepping: "); kprint_dec(stepping); print_char('\n');

    // Check key 64-bit feature bits
    int has_sse2   = (edx >> 26) & 1;
    int has_fpu    = (edx >>  0) & 1;
    int has_apic   = (edx >>  9) & 1;
    int has_avx    = (ecx >> 28) & 1;
    int has_sse4_2 = (ecx >> 20) & 1;

    print_string("Features      : ");
    if (has_fpu)    print_string("FPU ");
    if (has_apic)   print_string("APIC ");
    if (has_sse2)   print_string("SSE2 ");
    if (has_sse4_2) print_string("SSE4.2 ");
    if (has_avx)    print_string("AVX ");
    print_char('\n');

    // Leaf 0x80000001: Check Long Mode (bit 29 of EDX = LM)
    asm volatile("cpuid"
        : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
        : "a"(0x80000001));
    int has_lm = (edx >> 29) & 1;
    print_string("64-bit (LM)   : ");
    if (has_lm) { set_text_color(MAKE_COLOR(COLOR_LIGHT_GREEN, COLOR_BLACK)); print_string("YES - CPU supports x86_64"); }
    else        { set_text_color(MAKE_COLOR(COLOR_RED, COLOR_BLACK));         print_string("NO  - 32-bit only"); }
    reset_text_color();
    print_char('\n');

    // Leaf 0x80000002–0x80000004: Brand String
    char brand[49];
    uint32_t* b = (uint32_t*)brand;
    uint32_t dummy_b, dummy_c, dummy_d;
    asm volatile("cpuid" : "=a"(b[0]),  "=b"(b[1]),  "=c"(b[2]),  "=d"(b[3])  : "a"(0x80000002));
    asm volatile("cpuid" : "=a"(b[4]),  "=b"(b[5]),  "=c"(b[6]),  "=d"(b[7])  : "a"(0x80000003));
    asm volatile("cpuid" : "=a"(b[8]),  "=b"(b[9]),  "=c"(b[10]), "=d"(b[11]) : "a"(0x80000004));
    (void)dummy_b; (void)dummy_c; (void)dummy_d;
    brand[48] = '\0';
    // Trim leading spaces
    char* bp = brand;
    while (*bp == ' ') bp++;
    print_string("CPU Brand     : "); print_string(bp); print_char('\n');
}

static void cmd_arch() {
    uint32_t eax, ebx, ecx, edx;
    set_text_color(MAKE_COLOR(COLOR_LIGHT_GREEN, COLOR_BLACK));
    print_string("  ___  __  ___  __      _____  _  _   \n");
    print_string(" \\ \\ \\/ _ \\/ __\\/_ \\    / ___/ | || |  \n");
    print_string("  \\ \\/ // / (_ / __/   / /__  / _  |  \n");
    print_string("  /_/\\___/\\___/\\___/   \\___/ /_/ |_|  \n");
    reset_text_color();
    print_char('\n');
    print_string("Architecture : "); set_text_color(MAKE_COLOR(COLOR_LIGHT_CYAN, COLOR_BLACK));
    print_string("x86_64 (64-bit Long Mode)"); reset_text_color(); print_char('\n');
    print_string("Pointer Size : 64-bit (8 bytes)\n");
    print_string("Page Tables  : 4-Level (PML4 → PDPT → PD → PT)\n");
    print_string("Address Space: 48-bit Virtual (256 TB range)\n");
    print_string("Endianness   : Little-Endian\n");
    print_string("Registers    : RAX RBX RCX RDX RSI RDI R8-R15\n");
    print_string("SIMD         : SSE2+\n");

    asm volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(0x80000001));
    print_string("Long Mode Bit: ");
    if ((edx >> 29) & 1) { set_text_color(MAKE_COLOR(COLOR_LIGHT_GREEN, COLOR_BLACK)); print_string("SET (Active)"); }
    else                 { set_text_color(MAKE_COLOR(COLOR_RED, COLOR_BLACK));          print_string("NOT SET (ERROR)"); }
    reset_text_color(); print_char('\n');
}

static void cmd_sysinfo() {
    set_text_color(MAKE_COLOR(COLOR_LIGHT_CYAN, COLOR_BLACK));
    print_string("  +--------------------------------------+\n");
    print_string("  |    JARVIS OS  --  System Report      |\n");
    print_string("  +--------------------------------------+\n");
    reset_text_color();

    // --- OS Info ---
    set_text_color(MAKE_COLOR(COLOR_LIGHT_BLUE, COLOR_BLACK));
    print_string("  =[ OS ]=================================\n");
    reset_text_color();
    print_string("  Name         : Jarvis OS\n");
    print_string("  Arch         : x86_64 (64-bit)\n");
    print_string("  Boot Method  : GRUB Multiboot2 / UEFI\n");
    print_string("  Paging       : 4-Level (PML4)\n");

    // --- CPU Info ---
    set_text_color(MAKE_COLOR(COLOR_LIGHT_BLUE, COLOR_BLACK));
    print_string("  =[ CPU ]================================\n");
    reset_text_color();
    uint32_t eax, ebx, ecx, edx;
    asm volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(0));
    char vendor[13];
    vendor[0]=(ebx)&0xFF; vendor[1]=(ebx>>8)&0xFF; vendor[2]=(ebx>>16)&0xFF; vendor[3]=(ebx>>24)&0xFF;
    vendor[4]=(edx)&0xFF; vendor[5]=(edx>>8)&0xFF; vendor[6]=(edx>>16)&0xFF; vendor[7]=(edx>>24)&0xFF;
    vendor[8]=(ecx)&0xFF; vendor[9]=(ecx>>8)&0xFF; vendor[10]=(ecx>>16)&0xFF; vendor[11]=(ecx>>24)&0xFF;
    vendor[12]='\0';
    print_string("  Vendor       : "); print_string(vendor); print_char('\n');

    char brand[49];
    uint32_t* b = (uint32_t*)brand;
    asm volatile("cpuid" : "=a"(b[0]), "=b"(b[1]), "=c"(b[2]), "=d"(b[3])  : "a"(0x80000002));
    asm volatile("cpuid" : "=a"(b[4]), "=b"(b[5]), "=c"(b[6]), "=d"(b[7])  : "a"(0x80000003));
    asm volatile("cpuid" : "=a"(b[8]), "=b"(b[9]), "=c"(b[10]),"=d"(b[11]) : "a"(0x80000004));
    brand[48]='\0';
    char* bp = brand; while (*bp == ' ') bp++;
    print_string("  Model        : "); print_string(bp); print_char('\n');

    asm volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(1));
    uint32_t family = ((eax>>8)&0xF) + ((eax>>20)&0xFF);
    uint32_t model_num = ((eax>>4)&0xF) | (((eax>>16)&0xF)<<4);
    print_string("  Family       : "); kprint_dec(family); print_string("  Model: "); kprint_dec(model_num); print_char('\n');
    int has_apic = (edx >> 9) & 1;
    print_string("  APIC         : ");
    if (has_apic) { set_text_color(MAKE_COLOR(COLOR_LIGHT_GREEN, COLOR_BLACK)); print_string("Present"); }
    else          { set_text_color(MAKE_COLOR(COLOR_RED, COLOR_BLACK));         print_string("Not Found"); }
    reset_text_color(); print_char('\n');

    asm volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(0x80000001));
    int has_lm = (edx >> 29) & 1;
    print_string("  x86_64 LM    : ");
    if (has_lm) { set_text_color(MAKE_COLOR(COLOR_LIGHT_GREEN, COLOR_BLACK)); print_string("Active"); }
    else        { set_text_color(MAKE_COLOR(COLOR_RED, COLOR_BLACK));         print_string("INACTIVE"); }
    reset_text_color(); print_char('\n');

    // --- Memory ---
    set_text_color(MAKE_COLOR(COLOR_LIGHT_BLUE, COLOR_BLACK));
    print_string("  =[ MEMORY ]=============================\n");
    reset_text_color();
    uint32_t total_mb = pmm_get_total_memory_kb() / 1024;
    uint32_t used_kb  = pmm_get_used_blocks() * 4;
    uint32_t free_kb  = pmm_get_free_blocks() * 4;
    print_string("  Total RAM    : "); kprint_dec(total_mb); print_string(" MB\n");
    print_string("  Used         : "); kprint_dec(used_kb / 1024); print_string(" MB\n");
    print_string("  Free         : "); kprint_dec(free_kb / 1024); print_string(" MB\n");

    uint32_t uptime_h, uptime_m, uptime_s;
    timer_get_uptime(&uptime_h, &uptime_m, &uptime_s);
    set_text_color(MAKE_COLOR(COLOR_LIGHT_BLUE, COLOR_BLACK));
    print_string("  =[ UPTIME ]=============================\n");
    reset_text_color();
    print_string("  ");
    kprint_dec(uptime_h); print_string("h ");
    kprint_dec(uptime_m); print_string("m ");
    kprint_dec(uptime_s); print_string("s\n");
}

void shell_execute(char* cmd) {
    // Trim leading whitespace
    while (*cmd == ' ') cmd++;
    if (*cmd == '\0') return;

    // Trim trailing whitespace
    int len = strlen(cmd);
    while (len > 0 && cmd[len - 1] == ' ') {
        cmd[len - 1] = '\0';
        len--;
    }

    // Split command and argument
    char* arg = NULL;
    for (int i = 0; cmd[i] != '\0'; i++) {
        if (cmd[i] == ' ') {
            cmd[i] = '\0';
            arg = &cmd[i + 1];
            while (*arg == ' ') arg++;
            if (*arg == '\0') arg = NULL;
            break;
        }
    }

    // Remove trailing slashes from arg if it's a path (except root "/")
    if (arg) {
        int arg_len = strlen(arg);
        while (arg_len > 1 && arg[arg_len - 1] == '/') {
            arg[arg_len - 1] = '\0';
            arg_len--;
        }
    }

    // Execute commands
    if (strcmp(cmd, "ls") == 0) {
        if (!arg) {
            fat32_ls(0);
        } else {
            uint32_t cluster = fat32_resolve_path(arg);
            if (cluster != 0) fat32_ls(cluster);
            else print_string("Directory not found!\n");
        }
        return;
    }
    else if (strcmp(cmd, "cd") == 0) {
        if (!arg) { print_string("Usage: cd <path>\n"); return; }
        if (fat32_chdir(arg) != 0) {
            print_string("Directory not found!\n");
        }
        return;
    }
    else if (strcmp(cmd, "cat") == 0) {
        if (!arg) { print_string("Usage: cat <file>\n"); return; }
        int fd = fat32_open(arg, 'r');
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
    else if (strcmp(cmd, "touch") == 0) {
        if (!arg) { print_string("Usage: touch <file>\n"); return; }
        int fd = fat32_open(arg, 'w');
        if (fd >= 0) {
            fat32_close(fd);
            print_string("File created.\n");
        } else {
            print_string("Failed to create file.\n");
        }
        return;
    }
    else if (strcmp(cmd, "write") == 0) {
        if (!arg) { print_string("Usage: write <file> <text>\n"); return; }
        char* space = strstr(arg, " ");
        if (space) {
            *space = '\0';
            char* file = arg;
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
    else if (strcmp(cmd, "rm") == 0) {
        if (!arg) { print_string("Usage: rm <file>\n"); return; }
        if (fat32_unlink(arg) == 0) {
            print_string("File removed.\n");
        } else {
            print_string("Could not remove file.\n");
        }
        return;
    }
    else if (strcmp(cmd, "mkdir") == 0) {
        if (!arg) { print_string("Usage: mkdir <dir>\n"); return; }
        if (fat32_mkdir(arg) == 0) {
            print_string("Directory created.\n");
        } else {
            print_string("Could not create directory.\n");
        }
        return;
    }
    else if (strcmp(cmd, "rmdir") == 0) {
        if (!arg) { print_string("Usage: rmdir <dir>\n"); return; }
        if (fat32_rmdir(arg) == 0) {
            print_string("Directory removed.\n");
        } else {
            print_string("Could not remove directory.\n");
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
        print_string("- mkdir <dir>  (Create directory)\n");
        print_string("- rmdir <dir>  (Remove directory)\n");
        print_string("- clear        (Clear screen)\n");
        print_string("- ps           (List processes)\n");
        print_string("- mem          (Show memory statistics)\n");
        print_string("- sysinfo      (Full system information)\n");
        print_string("- cpuid        (Raw CPU identity report)\n");
        print_string("- arch         (Confirm CPU architecture)\n");
        print_string("- reboot       (Restart system)\n\n");
        return;
    }
    else if (strcmp(cmd, "ps") == 0) {
        task_list();
        return;
    }
    else if (strcmp(cmd, "mem") == 0) {
        uint32_t total_pmem = pmm_get_total_memory_kb();
        print_string("Physical Memory: ");
        kprint_dec(total_pmem / 1024);
        print_string(" MB\n");
        
        uint32_t used_p = pmm_get_used_blocks();
        uint32_t total_p = pmm_get_used_blocks() + pmm_get_free_blocks();
        print_string("Physical Used:   ");
        kprint_dec((used_p * 4096) / 1024);
        print_string(" KB / ");
        kprint_dec((total_p * 4096) / 1024);
        print_string(" KB\n");

        heap_stats_t stats;
        kmalloc_get_stats(&stats);
        print_string("Kernel Heap:    ");
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
    else if (strcmp(cmd, "sysinfo") == 0) {
        cmd_sysinfo();
        return;
    }
    else if (strcmp(cmd, "cpuid") == 0) {
        cmd_cpuid();
        return;
    }
    else if (strcmp(cmd, "arch") == 0) {
        cmd_arch();
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
        set_text_color(MAKE_COLOR(COLOR_LIGHT_GREEN, COLOR_BLACK));
        print_string("JARVIS [");
        set_text_color(MAKE_COLOR(COLOR_LIGHT_BLUE, COLOR_BLACK));
        fat32_print_cwd();
        set_text_color(MAKE_COLOR(COLOR_LIGHT_GREEN, COLOR_BLACK));
        print_string("] $ ");
        reset_text_color();
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

    if (c == '\t') {
        // Tab completion logic
        char* last_word = shell_buffer;
        int last_space_idx = -1;
        for (int i = 0; i < buffer_idx; i++) {
            if (shell_buffer[i] == ' ') {
                last_word = &shell_buffer[i + 1];
                last_space_idx = i;
            }
        }

        char prefix[64];
        int prefix_len = buffer_idx - (last_space_idx + 1);
        if (prefix_len > 63) prefix_len = 63;
        memcpy(prefix, last_word, prefix_len);
        prefix[prefix_len] = '\0';

        if (last_word == shell_buffer) {
            // Complete command
            int match_count = 0;
            char common_prefix[32];
            strcpy(common_prefix, "");

            for (int i = 0; commands[i] != NULL; i++) {
                if (strncmp(commands[i], prefix, prefix_len) == 0) {
                    if (match_count == 0) {
                        strcpy(common_prefix, commands[i]);
                    } else {
                        int j = 0;
                        while (common_prefix[j] && commands[i][j] && common_prefix[j] == commands[i][j]) j++;
                        common_prefix[j] = '\0';
                    }
                    match_count++;
                }
            }

            if (match_count > 0) {
                // One or more matches found
                for (int i = prefix_len; common_prefix[i] != '\0'; i++) {
                    shell_input(common_prefix[i]);
                }
                if (match_count == 1) {
                    shell_input(' ');
                }
            }
        } else {
            // Complete filename/directory
            // For now, only complete if the command is one that takes paths
            // Identify the command (first word)
            int first_space = -1;
            for (int i = 0; i < buffer_idx; i++) {
                if (shell_buffer[i] == ' ') { first_space = i; break; }
            }
            
            char cmd_name[32];
            int cmd_len = 0;
            if (first_space == -1) cmd_len = (buffer_idx < 31) ? buffer_idx : 31;
            else cmd_len = (first_space < 31) ? first_space : 31;
            
            memcpy(cmd_name, shell_buffer, cmd_len);
            cmd_name[cmd_len] = '\0';

            bool takes_path = false;
            if (strcmp(cmd_name, "ls") == 0 || strcmp(cmd_name, "cd") == 0 || 
                strcmp(cmd_name, "cat") == 0 || strcmp(cmd_name, "rm") == 0 ||
                strcmp(cmd_name, "mkdir") == 0 || strcmp(cmd_name, "rmdir") == 0 ||
                strcmp(cmd_name, "touch") == 0 || strcmp(cmd_name, "write") == 0) {
                takes_path = true;
            }

            if (takes_path) {
                tc_match_count = 0;
                tc_prefix_len = prefix_len;
                strcpy(tc_prefix, prefix);
                strcpy(tc_best_match, "");

                fat32_list_dir(fat32_cwd_cluster, tc_callback);

                if (tc_match_count > 0) {
                    for (int i = tc_prefix_len; tc_best_match[i] != '\0'; i++) {
                        shell_input(tc_best_match[i]);
                    }
                    if (tc_match_count == 1) {
                        shell_input(tc_is_dir ? '/' : ' ');
                    }
                }
            }
        }
        return;
    }

    if (buffer_idx < 255 && c >= ' ') {
        shell_buffer[buffer_idx++] = c;
        set_text_color(MAKE_COLOR(COLOR_WHITE, COLOR_BLACK));
        print_char(c);
        reset_text_color();
    }
}

extern char kbd_get(void);

void shell_task(void) {
    static bool welcomed = false;
    if (!welcomed) {
        set_text_color(MAKE_COLOR(COLOR_LIGHT_CYAN, COLOR_BLACK));
        print_string("\nJARVIS OS - Interactive Mode\n");
        reset_text_color();
        print_string("Type 'help' for a list of commands.\n\n");
        set_text_color(MAKE_COLOR(COLOR_LIGHT_GREEN, COLOR_BLACK));
        print_string("JARVIS [");
        set_text_color(MAKE_COLOR(COLOR_LIGHT_BLUE, COLOR_BLACK));
        fat32_print_cwd();
        set_text_color(MAKE_COLOR(COLOR_LIGHT_GREEN, COLOR_BLACK));
        print_string("] $ ");
        reset_text_color();
        welcomed = true;
    }

    // Process input from buffer (prevents running commands in ISR context)
    char c = kbd_get();
    if (c) {
        shell_input(c);
    }
}
