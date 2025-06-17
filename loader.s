global loader
extern kmain

section .multiboot_header
    align 4
    dd 0x1BADB002          ; magic number
    dd 0x0                 ; flags (set to 0 for now)
    dd -(0x1BADB002)       ; checksum (so sum is zero)

section .text
    align 4
loader:
    mov esp, stack_top     ; set stack pointer
    call kmain             ; call kernel entry point

.loop:
    jmp .loop              ; infinite loop

section .bss
    align 4
stack_bottom:
    resb 4096              ; reserve 4KB for stack
stack_top:
