#include "screen.h"
#include "idt.h"
#include "io.h"

/* Shell entry point */
extern void shell_prompt(void);

void kmain(void* mb, uint32_t magic) {
    (void)mb;
    (void)magic;

    clear_screen();
    print_string("JARVIS OS ONLINE\n");

    /* 1. Initialize IDT (ISRs + IRQ handlers) */
    init_idt();

    /* 2. Remap PIC and enable keyboard IRQ */
    pic_remap();
    pic_unmask_irq(1);   // IRQ1 = keyboard

    /* 3. Enable CPU interrupts */
    asm volatile("sti");

    /* 4. Start shell */
    shell_prompt();

    /* 5. Idle loop */
    while (1) {
        asm volatile("hlt"); // sleep until interrupt
    }
}
