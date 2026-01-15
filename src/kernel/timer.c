#include "timer.h"
#include "io.h"
#include <stdint.h>

// Tick counter - incremented by timer interrupt
static uint32_t timer_ticks = 0;

// Initialize the PIT (Programmable Interval Timer)
// PIT runs at 1193182 Hz, so divisor = 1193182 / frequency
void timer_init(uint32_t frequency) {
    uint32_t divisor = 1193182 / frequency;
    
    // Send command byte: channel 0, lobyte/hibyte, mode 3 (square wave), binary
    outb(0x43, 0x36);
    
    // Send divisor (low byte then high byte)
    outb(0x40, divisor & 0xFF);
    outb(0x40, (divisor >> 8) & 0xFF);
    
    // Reset tick counter
    timer_ticks = 0;
}

// Get total ticks since boot
uint32_t timer_get_ticks() {
    return timer_ticks;
}

// Wait for specified number of ticks
void timer_wait(uint32_t ticks) {
    uint32_t end = timer_ticks + ticks;
    while (timer_ticks < end) {
        // Busy wait - could be improved with interrupts
        asm volatile("hlt");
    }
}

// Get uptime in hours, minutes, seconds
// Assuming 100 Hz (10ms per tick)
void timer_get_uptime(uint32_t* hours, uint32_t* minutes, uint32_t* seconds) {
    uint32_t total_seconds = timer_ticks / 100;  // 100 ticks per second
    
    *hours = total_seconds / 3600;
    *minutes = (total_seconds % 3600) / 60;
    *seconds = total_seconds % 60;
}

// Timer interrupt handler - increments tick counter
// This is called from the assembly interrupt handler
void timer_handler() {
    timer_ticks++;
    // Send EOI to PIC
    outb(0x20, 0x20);
}
