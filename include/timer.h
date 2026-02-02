#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>

// Initialize the PIT timer
void timer_init(uint32_t frequency);

// Get total ticks since boot
uint32_t timer_get_ticks();

// Wait for specified number of ticks
void timer_wait(uint32_t ticks);

// Get uptime in hours, minutes, seconds
void timer_get_uptime(uint32_t* hours, uint32_t* minutes, uint32_t* seconds);

// Timer interrupt handler (called from assembly)
void timer_handler();

#endif