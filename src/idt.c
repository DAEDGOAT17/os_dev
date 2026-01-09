#include "idt.h"
#include "io.h"
#include "screen.h"

struct idt_entry idt[256];
struct idt_ptr idtp;

extern void idt_load(uint32_t ptr);
extern void keyboard_asm_handler();
extern void timer_asm_handler();

// A simple timer handler to stop the crash
void timer_handler() {
    outb(0x20, 0x20); // Acknowledge the interrupt
}

void pic_remap() {
    outb(0x20, 0x11); 
    outb(0xA0, 0x11);
    outb(0x21, 0x20); // Remap Master to 32
    outb(0xA1, 0x28); // Remap Slave to 40
    outb(0x21, 0x04);
    outb(0xA1, 0x02);
    outb(0x21, 0x01);
    outb(0xA1, 0x01);
    outb(0x21, 0x0);
    outb(0xA1, 0x0);
}

void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags) {
    idt[num].base_low = (base & 0xFFFF);
    idt[num].base_high = (base >> 16) & 0xFFFF;
    idt[num].sel = sel; // Usually 0x08
    idt[num].always0 = 0;
    idt[num].flags = flags; // 0x8E for Interrupt Gate
}

void init_idt() {
    idtp.limit = (sizeof(struct idt_entry) * 256) - 1;
    idtp.base = (uint32_t)&idt;

    // Fill all 256 gates with 0 to prevent jumping to random memory
    for(int i = 0; i < 256; i++) {
        idt_set_gate(i, 0, 0, 0);
    }

    pic_remap();

    // Register both Timer and Keyboard
    idt_set_gate(32, (uint32_t)timer_asm_handler, 0x08, 0x8E);
    idt_set_gate(33, (uint32_t)keyboard_asm_handler, 0x08, 0x8E);

    idt_load((uint32_t)&idtp);
    asm volatile("sti");
}