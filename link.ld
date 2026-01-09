ENTRY(loader)                /* The name of our entry label in loader.s */

SECTIONS
{
    . = 1M;                 /* The kernel starts at 1MB */

    _kernel_start = .;      /* <--- ADD THIS LINE HERE */

    .text : {
        *(.multiboot_header)
        *(.text)
    }

    .rodata : { *(.rodata) }
    .data   : { *(.data) }

    .bss : {
        *(COMMON)
        *(.bss)
    }

    . = ALIGN(4096);
    _kernel_end = .;        /* <--- ADD THIS LINE HERE */
}