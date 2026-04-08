#ifndef KMALLOC_H
#define KMALLOC_H

#include <stdint.h>
#include <stddef.h>

// Initialize the kernel heap
void kmalloc_init();

// Allocate memory
void* kmalloc(size_t size);

// Free memory
void kfree(void* ptr);

// Get heap statistics
typedef struct {
    uint32_t total_size;
    uint32_t used_size;
    uint32_t free_size;
    uint32_t num_blocks;
    uint32_t num_free_blocks;
} heap_stats_t;

void kmalloc_get_stats(heap_stats_t* stats);

#endif