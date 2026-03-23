#include "vmm.h"
#include "io.h"
#include "pmm.h"
#include "screen.h"
#include <stdint.h>

// The Page Directory (1024 entries, 4KB aligned)
uint32_t page_directory[1024] __attribute__((aligned(4096)));

// Static page tables for identity mapping the first 64MB (16 tables * 4MB)
uint32_t boot_page_tables[16][1024] __attribute__((aligned(4096)));

// We will define this in assembly later
extern void vmm_load_page_directory(uint32_t *directory);
extern void vmm_enable_paging();

void vmm_init() {
  // 1. Clear the Directory (Set all to "Not Present")
  for (int i = 0; i < 1024; i++) {
    page_directory[i] = 0x00000002; // Not present, but Read/Write
  }

  // 2. Identity Map the first 64MB (0 to 0x4000000)
  for (int table = 0; table < 16; table++) {
    for (int i = 0; i < 1024; i++) {
      uint32_t addr = (table * 4096 * 1024) + (i * 4096);
      boot_page_tables[table][i] = addr | (VMM_PRESENT | VMM_WRITABLE);
    }
    page_directory[table] =
        ((uint32_t)boot_page_tables[table]) | (VMM_PRESENT | VMM_WRITABLE);
  }

  // 3. Set up Recursive Paging
  page_directory[1023] =
      ((uint32_t)page_directory) | (VMM_PRESENT | VMM_WRITABLE);

  // 4. Hand the Directory to the CPU and Enable Paging
  vmm_load_page_directory(page_directory);
  vmm_enable_paging();
}

void vmm_map_page(uint32_t virtual_addr, uint32_t physical_addr) {
  uint32_t pd_index = virtual_addr >> 22;
  uint32_t pt_index = (virtual_addr >> 12) & 0x3FF;

  uint32_t *pd = (uint32_t *)0xFFFFF000;
  uint32_t *pt = (uint32_t *)(0xFFC00000 + (pd_index * 4096));

  if (!(page_directory[pd_index] & VMM_PRESENT)) {
    uint32_t phys_table = (uint32_t)pmm_alloc_block();
    if (!phys_table)
      return;

    page_directory[pd_index] = phys_table | (VMM_PRESENT | VMM_WRITABLE);
    pd[pd_index] = phys_table | (VMM_PRESENT | VMM_WRITABLE);
    asm volatile("invlpg (%0)" : : "r"(pt) : "memory");

    for (int i = 0; i < 1024; i++)
      pt[i] = 0;
  }

  pt[pt_index] = physical_addr | (VMM_PRESENT | VMM_WRITABLE);
  asm volatile("invlpg (%0)" : : "r"(virtual_addr) : "memory");
}

char *exception_messages[] = {"Division By Zero",
                              "Debug",
                              "Non Maskable Interrupt",
                              "Breakpoint",
                              "Into Detected Overflow",
                              "Out of Bounds",
                              "Invalid Opcode",
                              "No Coprocessor",
                              "Double Fault",
                              "Coprocessor Segment Overrun",
                              "Bad TSS",
                              "Segment Not Present",
                              "Stack Fault",
                              "General Protection Fault",
                              "Page Fault",
                              "Unknown Interrupt",
                              "Coprocessor Fault",
                              "Alignment Check",
                              "Machine Check",
                              "Reserved",
                              "Reserved",
                              "Reserved",
                              "Reserved",
                              "Reserved",
                              "Reserved",
                              "Reserved",
                              "Reserved",
                              "Reserved",
                              "Reserved",
                              "Reserved",
                              "Reserved",
                              "Reserved"};

void exception_handler(registers_t *regs) {
  asm volatile("cli");
  clear_screen();
  set_color(VGA_COLOR_WHITE, VGA_COLOR_RED);
  print_string("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
  print_string("           KERNEL PANIC: CPU EXCEPTION          \n");
  print_string("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n\n");

  if (regs->int_no < 32) {
    print_string("Condition: ");
    print_string(exception_messages[regs->int_no]);
    print_string("\n");
  } else {
    print_string("Interrupt: ");
    kprint_hex(regs->int_no);
    print_string("\n");
  }

  print_string("Error Code: ");
  kprint_hex(regs->err_code);
  print_string("\nEIP: ");
  kprint_hex(regs->eip);
  print_string("\nCS:  ");
  kprint_hex(regs->cs);
  print_string("\nEFLAGS: ");
  kprint_hex(regs->eflags);
  print_string("\n\nSystem Halted.");

  while (1)
    asm volatile("hlt");
}

void page_fault_handler(uint32_t error_code, uint32_t faulting_addr) {
  // This is now handled by the generic exception_handler via isr14 stub
  // but we can add back specialized logic if needed.
}