#include "pmm.h"
#include "vmm.h"
#include "gdt.h"
#include "idt.h"
#include "screen.h"

void kmain(multiboot_info_t* mbd, uint32_t magic) {
    init_gdt();
    init_idt();
    pmm_init(mbd);
    vmm_init();
    clear_screen();
    print_string("Paging is now active!\n");
    // If you see this message, identity mapping worked!
    while (1);
}