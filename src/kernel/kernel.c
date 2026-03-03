#include "pmm.h"
#include "vmm.h"
#include "gdt.h"
#include "idt.h"
#include "screen.h"
#include "kmalloc.h" 
#include "task.h"
#include "shell.h"
#include "intent_learner.h"
#include "word_embeddings.h"

extern uint32_t word_count;
// Keep kmain minimal: boot prints general info only

void kmain(uint32_t magic, multiboot_info_t* mbd) {
    // Initialize GDT FIRST before anything else
    init_gdt();
    
    // Clear screen early so we can see output
    clear_screen();
    print_string("Kernel: Init GDT OK\n");
    
    // Now init IDT after screen works
    init_idt();
    print_string("Kernel: Init IDT OK\n");
    
    pmm_init(mbd);
    print_string("Kernel: PMM OK\n");
    
    vmm_init();
    print_string("Kernel: VMM OK\n");
    
    kmalloc_init();
    print_string("Kernel: Kmalloc OK\n");
    
    task_init();
    print_string("Kernel: Task OK\n");
    
    task_create("shell", shell_task, 1);
    
    // Enable interrupts AFTER everything is ready
    asm volatile("sti");
    print_string("Kernel: Interrupts enabled\n");

    // Clear screen and show welcome message
    clear_screen();

        print_string("Kernel: Initializing AI Intent System...\n");
    
    // Train the AI with examples
    // (These are the initial "seed" examples - it learns from user input after)
    
    train_intent("mem", "show memory");
    train_intent("mem", "how much ram");
    train_intent("mem", "check memory usage");
    train_intent("mem", "display memory");
    
    train_intent("ps", "list processes");
    train_intent("ps", "show running tasks");
    train_intent("ps", "what is running");
    train_intent("ps", "list all tasks");
    
    train_intent("clear", "clear screen");
    train_intent("clear", "clean display");
    train_intent("clear", "reset screen");
    
    train_intent("reboot", "restart system");
    train_intent("reboot", "reboot now");
    train_intent("reboot", "shutdown");
    
    print_string("Kernel: AI Intent System Ready\n");
    print_string("Kernel: Vocabulary: ");
    kprint_dec(get_word_count());

    print_string(" words learned\n");
    print_string("=================================\n");
    print_string("     JARVIS OS v0.1 Alpha\n");
    print_string("=================================\n");
    print_string("Memory Manager: Active\n");
    print_string("Virtual Memory: Enabled\n");
    print_string("Heap Allocator: Ready\n");  // ADD THIS LINE
    print_string("Interrupts: Ready\n");
    print_string("\nType 'help' for available commands\n\n");
    
    // No automatic hardware scans or tests at boot; use shell commands instead.
    
    // Main kernel loop
    while (1) {
        task_yield();
    }
}