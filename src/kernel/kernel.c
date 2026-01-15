#include "pmm.h"
#include "vmm.h"
#include "gdt.h"
#include "idt.h"
#include "screen.h"
#include "kmalloc.h" 
#include "task.h"

void kmain(multiboot_info_t* mbd, uint32_t magic) {
    // Initialize core systems
    init_gdt();
    init_idt();
    task_init();
    pmm_init(mbd);
    vmm_init();
    kmalloc_init();  
    
    // Clear screen and show welcome message
    clear_screen();
    print_string("=================================\n");
    print_string("     JARVIS OS v0.1 Alpha\n");
    print_string("=================================\n");
    print_string("Memory Manager: Active\n");
    print_string("Virtual Memory: Enabled\n");
    print_string("Heap Allocator: Ready\n");  // ADD THIS LINE
    print_string("Interrupts: Ready\n");
    print_string("\nType 'help' for available commands\n\n");
    print_string("> ");
    
    // Main kernel loop
    while (1) {
        __asm__ volatile("hlt");
    }
}