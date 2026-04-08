#include "lwip_port/arch/cc.h"
#include "timer.h"

// Returns the system time in milliseconds
u32_t sys_now(void) {
    // timer runs at 100 Hz = 10 ms per tick
    return timer_get_ticks() * 10;
}
