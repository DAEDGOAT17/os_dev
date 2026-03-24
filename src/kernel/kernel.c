#include "pmm.h"
#include "vmm.h"
#include "gdt.h"
#include "idt.h"
#include "screen.h"
#include "kmalloc.h" 
#include "task.h"
#include "shell.h"
#include "ata.h"
#include "fat32.h"

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
    
    // Initialize Filesystem
    if (ata_init()) {
        fat32_mount(0);
        print_string("Kernel: FAT32 OK\n");
    } else {
        print_string("Kernel: ATA FAIL\n");
    }
    // =====================================
    
    task_create("shell", shell_task, 1);
    
    asm volatile("sti");
    print_string("Kernel: Interrupts enabled\n");
    
    clear_screen();
    print_string("=================================\n");
    print_string("           JARVIS OS\n");
    print_string("=================================\n");
    print_string("Memory Manager: Active\n");
    print_string("Virtual Memory: Enabled\n");
    print_string("Heap Allocator: Ready\n");
    print_string("File System:    Ready\n");
    print_string("Interrupts: Ready\n");
    print_string("\nType 'help' for available commands\n\n");
    
        while (1)
        {
            task_yield();
        }
}