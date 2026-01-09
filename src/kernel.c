#include "screen.h"
#include "gdt.h"
#include "idt.h"


void kmain(void* mb, uint32_t magic) {
    clear_screen();
    print_string("JARVIS OS ONLINE\n");
    
    // Initialize the IDT (which now includes PIC remapping)
    init_gdt();
    init_idt();

    print_string("Interrupts Enabled. Try typing: \n");

    while(1); // Stay here and let interrupts do the work
}