#ifndef VMM_H
#define VMM_H

#include <stdint.h>

// Page Table Entry flags
#define VMM_PRESENT  0x1   // Page is in RAM
#define VMM_WRITABLE 0x2   // Page can be written to
#define VMM_USER     0x4   // Page accessible by user-mode

void vmm_init();
void vmm_map_page(uint32_t virtual_addr, uint32_t physical_addr);

#endif