#include "pmm.h"
#include "vmm.h"
#include "gdt.h"
#include "idt.h"
#include "screen.h"
#include "kmalloc.h" 
#include "task.h"
#include "shell.h"

void kmain(multiboot_info_t* mbd, uint32_t magic) {
    // Initialize core systems
    init_gdt();
    init_idt();
    pmm_init(mbd);
    vmm_init();
    kmalloc_init();  
    task_init();
    task_create("shell", shell_task, 1);
    asm volatile("sti");

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
    
    // Main kernel loop
    while (1) {
        task_yield();
    }
}