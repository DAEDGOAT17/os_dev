#include "io.h"

unsigned char inb(unsigned short port) {
    unsigned char ret;
    asm volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

void outb(unsigned short port, unsigned char val) {
    asm volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

void wait_io() {
    outb(0x80, 0); 
}

uint32_t inl(unsigned short port) {
    uint32_t ret;
    asm volatile ("inl %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

void outl(unsigned short port, uint32_t val) {
    asm volatile ("outl %0, %1" : : "a"(val), "Nd"(port));
}

void sys_reboot() {
    uint8_t temp;
    asm volatile("cli");
    
    // Standard keyboard controller reset pulse
    do {
        temp = inb(0x64);
        if (temp & 0x01) inb(0x60);
    } while (temp & 0x02);
    
    outb(0x64, 0xFE);
    
    // If it fails, halt
    while(1) {
        asm volatile("hlt");
    }
}