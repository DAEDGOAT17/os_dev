#include "shell.h"
#include "ata.h"
#include "io.h"
#include "kmalloc.h"
#include "screen.h"
#include "string.h"
#include "task.h"
#include "timer.h"
#include "vfs.h"
#include <stdbool.h>

char shell_buffer[256];
int buffer_idx = 0;
int tab_press_count = 0;

// Helper function to compare command with argument for shell
bool cmd_starts_with(const char *cmd, const char *prefix) {
  int i = 0;
  while (prefix[i] && cmd[i] == prefix[i])
    i++;
  return prefix[i] == '\0';
}

// shell execute function
void shell_execute(char *cmd) {
  if (strcmp(cmd, "help") == 0) {
    set_color(VGA_COLOR_CYAN, VGA_COLOR_BLACK);
    print_string("\n=== JARVIS OS COMMANDS ===\n\n");
    set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    print_string("  help       - Show this message\n");
    print_string("  ls [path]  - List directory contents\n");
    print_string("  cat <file> - Display file contents\n");
    print_string("  disk       - Show disk information\n");
    print_string("  clear      - Clear screen\n");
    print_string("  sysinfo    - Show system info\n");
    print_string("  mem        - Memory status\n");
    print_string("  ps         - Process list\n");
    print_string("  reboot     - Restart machine\n");
    print_string("\n");

  } else if (strcmp(cmd, "ls") == 0 || cmd_starts_with(cmd, "ls ")) {
    char *path = (strcmp(cmd, "ls") == 0) ? "/" : cmd + 3;
    vfs_node_t *node = vfs_opendir(path);
    if (!node) {
      print_string("Directory not found.\n");
      return;
    }

    dirent_t *de;
    int i = 0;
    while ((de = vfs_readdir(node, i++))) {
      print_string(de->name);
      print_string("\n");
    }

  } else if (cmd_starts_with(cmd, "cat ")) {
    char *path = cmd + 4;
    int fd = vfs_open(path, 0);
    if (fd < 0) {
      print_string("File not found.\n");
      return;
    }

    char buf[512];
    int bytes;
    while ((bytes = vfs_read(fd, buf, 511)) > 0) {
      buf[bytes] = '\0';
      print_string(buf);
    }
    print_char('\n');
    vfs_close(fd);

  } else if (strcmp(cmd, "disk") == 0) {
    set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    print_string("\n=== Disk Information ===\n\n");
    set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    for (int i = 0; i < 4; i++) {
      ata_device_t *dev = ata_get_device(i);
      if (dev) {
        print_string("  Drive ");
        kprint_dec(i);
        print_string(": ");
        print_string(dev->model);
        print_string(" (");
        kprint_dec(dev->size / 2048);
        print_string(" MB)\n");
      }
    }
    print_string("\n");

  } else if (strcmp(cmd, "sysinfo") == 0) {
    set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    print_string("\n      ___          \n");
    print_string("     /  /\\        ");
    set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    print_string("OS: JARVIS OS v0.1 Alpha\n");
    set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    print_string("    /  /::\\       ");
    set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    print_string("Kernel: 32-bit monolithic\n");
    set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    print_string("   /  /:/\\:\\      ");
    set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    print_string("Uptime: ");
    uint32_t h, m, s;
    timer_get_uptime(&h, &m, &s);
    kprint_dec(h);
    print_string("h ");
    kprint_dec(m);
    print_string("m\n");
    set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    print_string("  /  /::\\ \\:\\     ");
    set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    print_string("Shell: JARVIS Shell v1.0\n");
    set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    print_string(" /__/:/\\:\\ \\:\\    ");
    set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    print_string("Resolution: 80x25 (VGA)\n");
    set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    print_string(" \\__\\/  \\:\\_\\/    ");
    set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    print_string("Memory: 256 MB\n");
    set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    print_string("      \\__\\/       \n\n");
    set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);

  } else if (strcmp(cmd, "crash") == 0) {
    set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
    print_string("Triggering page fault...\n");
    uint32_t *ptr = (uint32_t *)0xDEADBEEF;
    *ptr = 0x12345678;

  } else if (strcmp(cmd, "clear") == 0) {
    clear_screen();

  } else if (strcmp(cmd, "mem") == 0) {
    set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    print_string("\n=== Memory Status ===\n\n");
    set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    print_string("  Physical Memory : 256 MB\n");
    print_string("  Paging          : Enabled (Recursive Mapping)\n");
    print_string("  Page Size       : 4096 bytes\n");
    print_string("\n");

  } else if (cmd_starts_with(cmd, "echo ")) {
    // Print everything after "echo "
    print_string(cmd + 5);
    print_char('\n');

  } else if (strcmp(cmd, "version") == 0) {
    set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    print_string("\n=== JARVIS OS ===\n\n");
    set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    print_string("  Version      : v0.1 Alpha\n");
    print_string("  Build        : Feb 2026\n");
    print_string("  Architecture : i386\n");
    print_string("\n");

  } else if (strcmp(cmd, "reboot") == 0) {
    print_string("Rebooting...\n");
    sys_reboot();

  } else if (strcmp(cmd, "heap") == 0) {
    heap_stats_t stats;
    kmalloc_get_stats(&stats);
    set_color(VGA_COLOR_LIGHT_MAGENTA, VGA_COLOR_BLACK);
    print_string("\n=== Kernel Heap ===\n\n");
    set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    print_string("  Total : ");
    kprint_dec(stats.total_size / (1024 * 1024));
    print_string(" MB\n");
    print_string("  Used  : ");
    kprint_dec(stats.used_size / (1024 * 1024));
    print_string(" MB\n");
    print_string("  Free  : ");
    kprint_dec(stats.free_size / (1024 * 1024));
    print_string(" MB\n\n");
  } else if (strcmp(cmd, "ps") == 0) {
    print_string("\n");
    task_list();
    print_char('\n');
  } else if (strcmp(cmd, "uptime") == 0) {
    uint32_t h, m, s;
    timer_get_uptime(&h, &m, &s);
    print_string("\n  Uptime: ");
    kprint_dec(h);
    print_string("h ");
    kprint_dec(m);
    print_string("m ");
    kprint_dec(s);
    print_string("s\n\n");
  } else if (cmd[0] != '\0') {
    set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
    print_string("\n  Error: Unknown command '");
    print_string(cmd);
    print_string("'\n  Type 'help' for commands.\n\n");
    set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
  }
}

static void show_prompt() {
  set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
  print_string("JARVIS");
  set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
  print_string(" @ ");
  set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
  print_string("AOS");
  set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
  print_string(" $ ");
}

// shell input function
void shell_input(char c) {
  if (c == '\n') {
    shell_buffer[buffer_idx] = '\0';
    print_char('\n');

    // Execute command
    shell_execute(shell_buffer);

    // Show prompt
    show_prompt();
    buffer_idx = 0;

  } else if (c == '\b' && buffer_idx > 0) {
    buffer_idx--;
    print_char('\b');

  } else if (buffer_idx < 255 && c >= ' ') {
    shell_buffer[buffer_idx++] = c;
    print_char(c);
  }
}

// shell task function
void shell_task(void) {
  set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
  print_string("\n  Welcome to JARVIS OS!\n");
  set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
  print_string("  Type 'help' to begin.\n\n");
  show_prompt();

  while (1) {
    // Shell runs passively
    // Keyboard IRQ feeds shell_input()
    task_sleep(1);
  }
}
