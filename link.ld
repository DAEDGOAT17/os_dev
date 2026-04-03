ENTRY(loader)
OUTPUT_FORMAT(elf64-x86-64)

SECTIONS
{
    . = 1M;
    
    _kernel_start = .;

    .text : {
        build/loader.o(.multiboot_header)
        build/loader.o(.multiboot2_header)
        build/loader.o(.text)
        *(.text)
    }

    .rodata : { *(.rodata) }
    .data   : { *(.data) }

    .bss : {
        *(COMMON)
        *(.bss)
    }

    . = ALIGN(4096);
    _kernel_end = .;
}