#include "pmm.h"

// Global bitmap - resides in .bss section
uint8_t pmm_bitmap[PMM_BITMAP_SIZE];

// Symbols from link.ld
extern uint32_t _kernel_start;
extern uint32_t _kernel_end;

static void pmm_set_bit(uint32_t bit) {
    pmm_bitmap[bit / 8] |= (1 << (bit % 8));
}

static void pmm_clear_bit(uint32_t bit) {
    pmm_bitmap[bit / 8] &= ~(1 << (bit % 8));
}

void pmm_init(multiboot_info_t* mbd) {
    // 1. Mark everything as RESERVED
    for (int i = 0; i < PMM_BITMAP_SIZE; i++) pmm_bitmap[i] = 0xFF;

    // 2. Parse Multiboot Map
    multiboot_mmap_entry_t* mmap = (multiboot_mmap_entry_t*)mbd->mmap_addr;
    uint32_t mmap_end = mbd->mmap_addr + mbd->mmap_length;

    while((uint32_t)mmap < mmap_end) {
        if (mmap->type == 1) { // Available RAM
            uint32_t addr = (uint32_t)mmap->addr;
            uint32_t end_addr = addr + (uint32_t)mmap->len;

            while (addr < end_addr) {
                uint32_t block = addr / PMM_BLOCK_SIZE;
                // Safety: Don't write past our 128MB bitmap limit
                if (block < (PMM_BITMAP_SIZE * 8)) {
                    pmm_clear_bit(block);
                }
                addr += PMM_BLOCK_SIZE;
            }
        }
        mmap = (multiboot_mmap_entry_t*)((uint32_t)mmap + mmap->size + sizeof(mmap->size));
    }

    // 3. Protect Kernel Memory
    uint32_t start_block = (uint32_t)&_kernel_start / PMM_BLOCK_SIZE;
    uint32_t end_block = (uint32_t)&_kernel_end / PMM_BLOCK_SIZE;
    for (uint32_t i = start_block; i <= end_block; i++) {
        pmm_set_bit(i);
    }

    // 4. Protect Page 0 (NULL pointer protection)
    pmm_set_bit(0);
}

void* pmm_alloc_block() {
    // Start searching from bit 256 (256 * 4096 = 1,048,576 bytes = 1MB)
    uint32_t start_bit = 256; 

    for (uint32_t i = (start_bit / 8); i < PMM_BITMAP_SIZE; i++) {
        if (pmm_bitmap[i] != 0xFF) {
            for (int j = 0; j < 8; j++) {
                uint32_t block = (i * 8) + j;
                if (block >= start_bit && !(pmm_bitmap[i] & (1 << j))) {
                    pmm_set_bit(block);
                    return (void*)(block * PMM_BLOCK_SIZE);
                }
            }
        }
    }
    return 0;
}

void pmm_free_block(void* addr) {
    uint32_t block = (uint32_t)addr / PMM_BLOCK_SIZE;
    pmm_clear_bit(block);
}