bits 32
global loader
global idt_load
extern kmain

section .multiboot_header
    align 4
    dd 0x1BADB002               ; Magic number
    dd 0x01                     ; Flags (Request memory info)
    dd -(0x1BADB002 + 0x01)     ; Checksum

section .text
loader:
    mov esp, stack_top          ; Setup stack
    push eax                    ; Multiboot magic
    push ebx                    ; Multiboot info pointer
    call kmain

; Function called from idt.c to load the IDT pointer
idt_load:
    mov eax, [esp + 4]          ; Get pointer to IDT info from stack
    lidt [eax]                  ; Load the IDT register
    ret

.loop:
    hlt
    jmp .loop

section .bss
align 16
stack_bottom: resb 16384        ; 16KB Stack
stack_top: