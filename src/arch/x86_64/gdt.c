#include "gdt.h"

struct gdt_entry gdt[3];
struct gdt_ptr gp;

extern void gdt_flush(uint64_t ptr);

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
    gp.base  = (uint64_t)&gdt;

    gdt_set_gate(0, 0, 0, 0, 0);

    gdt_set_gate(1, 0, 0, 
                 GDT_ACCESS_PRESENT | GDT_ACCESS_RING0 | GDT_ACCESS_CODE, 
                 GDT_FLAG_64BIT);

    gdt_set_gate(2, 0, 0, 
                 GDT_ACCESS_PRESENT | GDT_ACCESS_RING0 | GDT_ACCESS_DATA, 
                 0);

    gdt_flush((uint64_t)&gp);
}