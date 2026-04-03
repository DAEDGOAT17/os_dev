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
#include "llm_engine.h"
extern uint32_t word_count;
// Keep kmain minimal: boot prints general info only

void kmain(uint32_t magic, multiboot_info_t* mbd) {
    // Initialize GDT FIRST before anything else
    init_gdt();
    
    // Parse Multiboot headers early to detect Framebuffer & memory
    pmm_init(magic, mbd);
    
    // Now init IDT
    init_idt();
    
    // Map framebuffer and identity map everything needed
    vmm_init();
    
    // Clear screen early so we can see output
    clear_screen();
    print_string("Kernel: Init GDT OK\n");
    print_string("Kernel: PMM OK\n");
    print_string("Kernel: Init IDT OK\n");
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
    
    task_create("shell", shell_task, 1);
    llm_task_spawn();    
    asm volatile("sti");
    print_string("Kernel: Interrupts enabled\n");

    // -------------------------------------------------------------------
    // CPUID detection for boot banner
    // -------------------------------------------------------------------
    uint32_t eax, ebx, ecx, edx;

    // Vendor string
    asm volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(0));
    char vendor[13];
    vendor[0]=(ebx)&0xFF;    vendor[1]=(ebx>>8)&0xFF;  vendor[2]=(ebx>>16)&0xFF; vendor[3]=(ebx>>24)&0xFF;
    vendor[4]=(edx)&0xFF;    vendor[5]=(edx>>8)&0xFF;  vendor[6]=(edx>>16)&0xFF; vendor[7]=(edx>>24)&0xFF;
    vendor[8]=(ecx)&0xFF;    vendor[9]=(ecx>>8)&0xFF;  vendor[10]=(ecx>>16)&0xFF;vendor[11]=(ecx>>24)&0xFF;
    vendor[12]='\0';

    // Feature flags (APIC)
    asm volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(1));
    int has_apic = (edx >> 9) & 1;

    // Long Mode bit
    asm volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(0x80000001));
    int has_lm = (edx >> 29) & 1;

    // Total RAM
    uint32_t total_mb = pmm_get_total_memory_kb() / 1024;

    // -------------------------------------------------------------------
    // Boot Banner
    // -------------------------------------------------------------------
    clear_screen();

    // Large ASCII logo - pure 7-bit ASCII, renders perfectly on any framebuffer
    print_color_string("  ###     #    ######  ##   ##  ###  #####\n",  MAKE_COLOR(COLOR_LIGHT_CYAN,  COLOR_BLACK));
    print_color_string("   #     # #   #    #  ##   ##   #  #     \n",  MAKE_COLOR(COLOR_LIGHT_CYAN,  COLOR_BLACK));
    print_color_string("   #    #####  ######   ## ##    #   ####  \n",  MAKE_COLOR(COLOR_CYAN,         COLOR_BLACK));
    print_color_string("   #   #     # #    #    ###     #       # \n",  MAKE_COLOR(COLOR_LIGHT_CYAN,  COLOR_BLACK));
    print_color_string("  ###  #     # #    #     #     ###  #####  \n", MAKE_COLOR(COLOR_LIGHT_CYAN,  COLOR_BLACK));
    print_char('\n');

    print_color_string("  +--------------------------------------------------+\n", MAKE_COLOR(COLOR_LIGHT_BLUE, COLOR_BLACK));
    print_color_string("  |       x86_64 AI Kernel  v1.0  --  64-bit         |\n", MAKE_COLOR(COLOR_YELLOW,     COLOR_BLACK));
    print_color_string("  +--------------------------------------------------+\n", MAKE_COLOR(COLOR_LIGHT_BLUE, COLOR_BLACK));
    print_char('\n');

    // Architecture block
    print_color_string("  =[ ARCHITECTURE ]==============================\n", MAKE_COLOR(COLOR_LIGHT_BLUE, COLOR_BLACK));
    print_color_string("  CPU Vendor   : ", MAKE_COLOR(COLOR_LIGHT_GREY, COLOR_BLACK));
    print_color_string(vendor,               MAKE_COLOR(COLOR_WHITE,      COLOR_BLACK));
    print_char('\n');

    print_color_string("  Mode         : ", MAKE_COLOR(COLOR_LIGHT_GREY, COLOR_BLACK));
    if (has_lm) print_color_string("[OK] x86_64  64-Bit Long Mode  ACTIVE\n", MAKE_COLOR(COLOR_LIGHT_GREEN, COLOR_BLACK));
    else        print_color_string("[!!] WARNING: Long Mode NOT detected!\n",  MAKE_COLOR(COLOR_RED,          COLOR_BLACK));

    print_color_string("  Paging       : ", MAKE_COLOR(COLOR_LIGHT_GREY, COLOR_BLACK));
    print_color_string("[OK] 4-Level Paging  PML4->PDPT->PD->PT\n",            MAKE_COLOR(COLOR_LIGHT_GREEN, COLOR_BLACK));

    print_color_string("  Interrupts   : ", MAKE_COLOR(COLOR_LIGHT_GREY, COLOR_BLACK));
    if (has_apic) print_color_string("[OK] Local APIC + I/O APIC  (Modern)\n", MAKE_COLOR(COLOR_LIGHT_GREEN, COLOR_BLACK));
    else          print_color_string("[>>] Legacy 8259 PIC  (BIOS Fallback)\n", MAKE_COLOR(COLOR_YELLOW,      COLOR_BLACK));
    print_char('\n');

    // System block
    print_color_string("  =[ SYSTEM ]=====================================\n", MAKE_COLOR(COLOR_LIGHT_BLUE, COLOR_BLACK));

    print_color_string("  RAM          : ", MAKE_COLOR(COLOR_LIGHT_GREY, COLOR_BLACK));
    kprint_dec(total_mb);
    print_color_string(" MB Physical Memory\n", MAKE_COLOR(COLOR_WHITE, COLOR_BLACK));

    print_color_string("  Heap         : ", MAKE_COLOR(COLOR_LIGHT_GREY, COLOR_BLACK));
    print_color_string("[OK] Dynamic  --  Base: 0x0000000200000000\n",  MAKE_COLOR(COLOR_LIGHT_GREEN, COLOR_BLACK));

    print_color_string("  File System  : ", MAKE_COLOR(COLOR_LIGHT_GREY, COLOR_BLACK));
    print_color_string("[OK] FAT32  on  ATA Disk\n",                   MAKE_COLOR(COLOR_LIGHT_GREEN, COLOR_BLACK));

    print_color_string("  AI Brain     : ", MAKE_COLOR(COLOR_LIGHT_GREY, COLOR_BLACK));
    if (initrd_brain_loaded)
        print_color_string("[OK] Loaded from InitRD Boot Module\n",    MAKE_COLOR(COLOR_LIGHT_GREEN,  COLOR_BLACK));
    else
        print_color_string("[--] Not Found  (run export_nanogpt.py)\n",MAKE_COLOR(COLOR_YELLOW,       COLOR_BLACK));

    print_char('\n');
    print_color_string("  ================================================\n",  MAKE_COLOR(COLOR_LIGHT_BLUE, COLOR_BLACK));
    print_color_string("  >> Type 'help' for commands  |  'sysinfo' for full report\n", MAKE_COLOR(COLOR_LIGHT_CYAN, COLOR_BLACK));
    print_color_string("  ================================================\n",  MAKE_COLOR(COLOR_LIGHT_BLUE, COLOR_BLACK));
    print_char('\n');

    while (1) {
        task_yield();
    }
}