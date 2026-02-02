#include "vmm.h"
#include "pmm.h"
#include "io.h"
#include <stdint.h>

// The Page Directory (1024 entries, 4KB aligned)
uint32_t page_directory[1024] __attribute__((aligned(4096)));

// Static page tables for identity mapping the first 64MB (16 tables * 4MB)
uint32_t boot_page_tables[16][1024] __attribute__((aligned(4096)));

// We will define this in assembly later
extern void vmm_load_page_directory(uint32_t* directory);
extern void vmm_enable_paging();

void vmm_init() {
    // 1. Clear the Directory (Set all to "Not Present")
    for (int i = 0; i < 1024; i++) {
        page_directory[i] = 0x00000002; // Not present, but Read/Write
    }

    // 2. Identity Map the first 64MB (0 to 0x4000000)
    // This provides enough mapped space for the kernel, heap, and page tables
    for (int table = 0; table < 16; table++) {
        for (int i = 0; i < 1024; i++) {
            uint32_t addr = (table * 4096 * 1024) + (i * 4096);
            boot_page_tables[table][i] = addr | (VMM_PRESENT | VMM_WRITABLE);
        }
        // Put the Page Table into the Page Directory
        page_directory[table] = ((uint32_t)boot_page_tables[table]) | (VMM_PRESENT | VMM_WRITABLE);
    }

    // 3. Hand the Directory to the CPU and Enable Paging
    vmm_load_page_directory(page_directory);
    vmm_enable_paging();
}

void vmm_map_page(uint32_t virtual_addr, uint32_t physical_addr) {
    uint32_t pd_index = virtual_addr >> 22;  // Top 10 bits
    uint32_t pt_index = (virtual_addr >> 12) & 0x3FF;  // Next 10 bits
    
    // Check if page table exists for this directory entry
    if (!(page_directory[pd_index] & VMM_PRESENT)) {
        // Allocate new page table
        uint32_t* new_table = (uint32_t*)pmm_alloc_block();
        if (!new_table) return;  // Out of memory
        
        // Clear the new table
        for (int i = 0; i < 1024; i++) {
            new_table[i] = 0;
        }
        
        // Add to directory
        page_directory[pd_index] = ((uint32_t)new_table) | (VMM_PRESENT | VMM_WRITABLE);
    }
    
    // Get the page table
    uint32_t* page_table = (uint32_t*)(page_directory[pd_index] & 0xFFFFF000);
    
    // Map the page
    page_table[pt_index] = physical_addr | (VMM_PRESENT | VMM_WRITABLE);
    
    // Flush TLB for this address
    asm volatile("invlpg (%0)" : : "r"(virtual_addr) : "memory");
}

#include "screen.h"

void page_fault_handler(uint32_t error_code, uint32_t faulting_addr) {
    asm volatile("cli"); // Disable interrupts

    print_string("\n\n\n");
    print_string("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
    print_string("           KERNEL PANIC: PAGE FAULT             \n");
    print_string("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n\n");

    print_string("Faulting Address: ");
    kprint_hex(faulting_addr);
    print_string("\n");

    print_string("Error Code: ");
    kprint_hex(error_code);
    print_string(" (");

    int present = !(error_code & 0x1);   // Page not present
    int rw = error_code & 0x2;           // Write operation
    int us = error_code & 0x4;           // Processor was in user-mode
    int reserved = error_code & 0x8;      // Overwritten CPU-reserved bits of page entry
    int id = error_code & 0x10;          // Caused by an instruction fetch

    if (present) print_string("not-present ");
    if (rw) print_string("read-only ");
    if (us) print_string("user-mode ");
    if (reserved) print_string("reserved ");
    if (id) print_string("instruction-fetch ");
    print_string(")\n\n");

    print_string("The system will automatically reboot in 5 seconds...\n");

    // Simple busy-wait since interrupts are disabled
    for (int i = 5; i > 0; i--) {
        print_string("Rebooting in ");
        kprint_dec(i);
        print_string("... \n");
        
        // Roughly 1 second delay (CPU dependent, works for QEMU)
        for (volatile uint32_t j = 0; j < 100000000; j++) {
            asm volatile("nop");
        }
    }

    print_string("Rebooting now!\n");
    sys_reboot();
}