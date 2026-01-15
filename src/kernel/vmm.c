#include "vmm.h"
#include "pmm.h"
#include <stdint.h>

// The Page Directory (1024 entries, 4KB aligned)
uint32_t page_directory[1024] __attribute__((aligned(4096)));

// The first Page Table (Maps the first 4MB of RAM)
uint32_t first_page_table[1024] __attribute__((aligned(4096)));

// We will define this in assembly later
extern void vmm_load_page_directory(uint32_t* directory);
extern void vmm_enable_paging();

void vmm_init() {
    // 1. Clear the Directory (Set all to "Not Present")
    for (int i = 0; i < 1024; i++) {
        page_directory[i] = 0x00000002; // Not present, but Read/Write
    }

    // 2. Identity Map the first 4MB (0 to 0x400000)
    // We fill the Page Table with physical addresses
    for (int i = 0; i < 1024; i++) {
        // (i * 4096) is the physical address
        // Attributes: Present + Writable
        first_page_table[i] = (i * 4096) | (VMM_PRESENT | VMM_WRITABLE);
    }

    // 3. Put the Page Table into the Page Directory
    // Entry 0 in the directory covers the first 4MB
    page_directory[0] = ((uint32_t)first_page_table) | (VMM_PRESENT | VMM_WRITABLE);

    // 4. Hand the Directory to the CPU and Enable Paging
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