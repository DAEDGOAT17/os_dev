ENTRY(loader)
OUTPUT_FORMAT(elf32-i386)

SECTIONS {
    . = 0x100000; /* 1MB mark */

    .text : {
        *(.multiboot_header)
        *(.text)
    }

    .rodata : { *(.rodata) }
    .data : { *(.data) }
    
    .bss : {
        *(COMMON)
        *(.bss)
    }
}