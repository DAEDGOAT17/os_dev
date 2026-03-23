#include "ata.h"
#include "fat32.h"
#include "gdt.h"
#include "idt.h"
#include "kmalloc.h"
#include "pmm.h"
#include "screen.h"
#include "shell.h"
#include "task.h"
#include "vfs.h"
#include "vmm.h"

void kmain(multiboot_info_t *mbd, uint32_t magic) {
  asm volatile("cli");
  clear_screen();
  set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
  print_string("JARVIS Kernel: Booting...\n");

  // Initialize core systems
  print_string("  [ ] GDT...");
  init_gdt();
  print_string("OK\n");

  print_string("  [ ] IDT...");
  init_idt();
  print_string("OK\n");

  print_string("  [ ] PMM...");
  pmm_init(mbd);
  print_string("OK\n");

  print_string("  [ ] VMM...");
  vmm_init();
  print_string("OK\n");

  print_string("  [ ] Heap...");
  kmalloc_init();
  print_string("OK\n");

  print_string("  [ ] Tasks...");
  task_init();
  print_string("OK\n");

  // Initialize I/O and File System
  print_string("  [ ] VFS...");
  vfs_init();
  print_string("OK\n");

  print_string("  [ ] ATA...");
  ata_init();
  print_string("OK\n");

  print_string("  [ ] FAT32...");
  if (fat32_init(0)) { // Initialize Primary Master (0)
    vfs_mount("/", fat32_get_root());
    print_string("Mounted /\n");
  } else {
    print_string("None Found\n");
  }

  print_string("  [ ] Finishing Boot...\n");
  task_create("shell", shell_task, 1);
  print_string("  [ ] Task Created\n");
  asm volatile("sti");
  print_string("  [ ] Interrupts Enabled\n");

  print_string("\nBoot Complete. Loading Shell in 2 seconds...\n");
  for (volatile uint32_t j = 0; j < 200000000; j++)
    asm volatile("nop");

  // Clear screen and show welcome message
  clear_screen();
  set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
  print_string("================================================\n");
  print_string("       __  ___   ____  _    _  _____  ____      \n");
  print_string("      /  |/  /  / __ \\| |  | |/ ____|/ __ \\     \n");
  print_string("     / /|_/ /  | |  | | |  | | (___ | |  | |    \n");
  print_string("    / /  / /   | |  | | |  | |\\___ \\| |  | |    \n");
  print_string("   / /  / /    | |__| | |__| |____) | |__| |    \n");
  print_string("  /_/  /_/      \\____/ \\____/|_____/ \\____/     \n");
  print_string("================================================\n");

  set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
  print_string("\n  [+] Memory Manager: ");
  set_color(VGA_COLOR_GREEN, VGA_COLOR_BLACK);
  print_string("ACTIVE\n");
  set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
  print_string("  [+] Virtual Memory: ");
  set_color(VGA_COLOR_GREEN, VGA_COLOR_BLACK);
  print_string("RECURSIVE\n");
  set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
  print_string("  [+] Multitasking:   ");
  set_color(VGA_COLOR_GREEN, VGA_COLOR_BLACK);
  print_string("ENABLED\n");
  set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
  print_string("  [+] Shell:          ");
  set_color(VGA_COLOR_GREEN, VGA_COLOR_BLACK);
  print_string("READY\n\n");

  set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
  print_string("  Type 'help' for available commands\n\n");
  set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);

  // Main kernel loop
  while (1) {
    task_yield();
  }
}