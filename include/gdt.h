#ifndef GDT_H
#define GDT_H

#include <stdint.h>

// Defines a GDT entry (8 bytes)
struct gdt_entry {
    uint16_t limit_low;           // The lower 16 bits of the limit
    uint16_t base_low;            // The lower 16 bits of the base
    uint8_t  base_middle;         // The next 8 bits of the base
    uint8_t  access;              // Access flags (Ring, Type, etc)
    uint8_t  granularity;         // High 4 bits of limit + Flags
    uint8_t  base_high;           // The last 8 bits of the base
} __attribute__((packed));

// Defines the GDT pointer for the lgdt instruction
struct gdt_ptr {
    uint16_t limit;               // Size of gdt table - 1
    uint32_t base;                // Address of the first gdt_entry
} __attribute__((packed));

// Access Byte constants
#define GDT_ACCESS_PRESENT     0x80 // Must be 1 for all valid segments
#define GDT_ACCESS_RING0       0x00 // Kernel mode
#define GDT_ACCESS_RING3       0x60 // User mode
#define GDT_ACCESS_CODE        0x1A // Executable segment
#define GDT_ACCESS_DATA        0x12 // Writable segment

// Flags constants
#define GDT_FLAG_32BIT         0x40 // 32-bit protected mode
#define GDT_FLAG_4K_GRAN       0x80 // Limit is in 4KB blocks

void init_gdt();

#endif