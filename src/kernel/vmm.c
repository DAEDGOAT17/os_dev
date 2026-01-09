#include "vmm.h"
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