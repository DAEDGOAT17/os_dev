#ifndef VMM_H
#define VMM_H

#include <stdint.h>

// Standard ISR Register state
typedef struct {
  uint32_t ds;                                     // Data segment selector
  uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax; // Pushed by pusha
  uint32_t int_no, err_code;                       // Pushed by isr stub
  uint32_t eip, cs, eflags, useresp,
      ss; // Pushed by the processor automatically
} registers_t;

// Page Table Entry flags
#define VMM_PRESENT 0x1  // Page is in RAM
#define VMM_WRITABLE 0x2 // Page can be written to
#define VMM_USER 0x4     // Page accessible by user-mode

void vmm_init();
void vmm_map_page(uint32_t virtual_addr, uint32_t physical_addr);
void exception_handler(registers_t *regs);

#endif