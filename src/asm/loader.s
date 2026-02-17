bits 32
global loader
global gdt_flush
global idt_load
extern kmain

section .multiboot_header
    align 4
    dd 0x1BADB002
    dd 0x01
    dd -(0x1BADB002 + 0x01)

section .text
loader:
    mov esp, stack_top
    ; Enable FPU and SSE: clear CR0.EM, set CR0.MP and CR0.NE, set CR4.OSFXSR and CR4.OSXMMEXCPT
    mov eax, cr0
    ; Clear EM (bit 2) to enable x87 FPU, set MP (bit1) and NE (bit5)
    and eax, 0xFFFFFFFB    ; Clear bit 2 (EM)
    or  eax, 0x00000022    ; Set bits 1 (MP) and 5 (NE)
    mov cr0, eax

    mov eax, cr4
    ; Set OSFXSR (bit 9) and OSXMMEXCPT (bit 10) to enable SSE / FXSAVE support
    or  eax, 0x00000600
    mov cr4, eax

    ; Initialize x87 FPU state
    fninit

    push eax
    push ebx
    call kmain

; Takes the GDT pointer from C and applies it to the CPU
gdt_flush:
    mov eax, [esp + 4]
    lgdt [eax]          ; Load the GDT
    
    ; Reload all segment registers to use the new GDT
    mov ax, 0x10        ; Data segment offset (2nd entry * 8)
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    
    jmp 0x08:.flush     ; Far jump to reload CS to 0x08
.flush:
    ret

idt_load:
    mov eax, [esp + 4]  ; Get the pointer passed from C
    lidt [eax]          ; Load the IDT register
    ret

section .bss
align 16
stack_bottom: 
    resb 16384 ; 16 KB
stack_top:     ; ESP points here (grows downwards)