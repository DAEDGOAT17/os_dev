#include "gdt.h"

struct gdt_entry gdt[3];
struct gdt_ptr gp;

// Defined in loader.s
extern void gdt_flush(uint32_t);

void gdt_set_gate(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    gdt[num].base_low    = (base & 0xFFFF);
    gdt[num].base_middle = (base >> 16) & 0xFF;
    gdt[num].base_high   = (base >> 24) & 0xFF;

    gdt[num].limit_low   = (limit & 0xFFFF);
    gdt[num].granularity = (limit >> 16) & 0x0F;

    gdt[num].granularity |= gran & 0xF0;
    gdt[num].access      = access;
}

void init_gdt() {
    gp.limit = (sizeof(struct gdt_entry) * 3) - 1;
    gp.base  = (uint32_t)&gdt;

    // 0x00: Null segment (Required)
    gdt_set_gate(0, 0, 0, 0, 0);

    // 0x08: Code segment (Base 0, Limit 4GB, 32-bit, Ring 0)
    gdt_set_gate(1, 0, 0xFFFFFFFF, 
                 GDT_ACCESS_PRESENT | GDT_ACCESS_RING0 | GDT_ACCESS_CODE, 
                 GDT_FLAG_32BIT | GDT_FLAG_4K_GRAN);

    // 0x10: Data segment (Base 0, Limit 4GB, 32-bit, Ring 0)
    gdt_set_gate(2, 0, 0xFFFFFFFF, 
                 GDT_ACCESS_PRESENT | GDT_ACCESS_RING0 | GDT_ACCESS_DATA, 
                 GDT_FLAG_32BIT | GDT_FLAG_4K_GRAN);

    gdt_flush((uint32_t)&gp);
}