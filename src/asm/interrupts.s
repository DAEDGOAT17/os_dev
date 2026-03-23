; ISR Stubs for CPU Exceptions
%macro ISR_NOERRCODE 1
global isr%1
isr%1:
    cli
    push byte 0             ; Push dummy error code
    push byte %1            ; Push interrupt number
    jmp isr_common_stub
%endmacro

%macro ISR_ERRCODE 1
global isr%1
isr%1:
    cli
    ; CPU already pushed an error code
    push byte %1            ; Push interrupt number
    jmp isr_common_stub
%endmacro

; ISRs 0-31
ISR_NOERRCODE 0
ISR_NOERRCODE 1
ISR_NOERRCODE 2
ISR_NOERRCODE 3
ISR_NOERRCODE 4
ISR_NOERRCODE 5
ISR_NOERRCODE 6
ISR_NOERRCODE 7
ISR_ERRCODE   8
ISR_NOERRCODE 9
ISR_ERRCODE   10
ISR_ERRCODE   11
ISR_ERRCODE   12
ISR_ERRCODE   13
ISR_ERRCODE   14
ISR_NOERRCODE 15
ISR_NOERRCODE 16
ISR_ERRCODE   17
ISR_NOERRCODE 18
ISR_NOERRCODE 19
ISR_NOERRCODE 20
ISR_NOERRCODE 21
ISR_NOERRCODE 22
ISR_NOERRCODE 23
ISR_NOERRCODE 24
ISR_NOERRCODE 25
ISR_NOERRCODE 26
ISR_NOERRCODE 27
ISR_NOERRCODE 28
ISR_NOERRCODE 29
ISR_NOERRCODE 30
ISR_NOERRCODE 31

; IRQs 32-47
%macro IRQ 2
global irq%2
irq%2:
    cli
    push byte 0
    push byte %2
    jmp irq_common_stub
%endmacro

IRQ 0, 32
IRQ 1, 33
IRQ 2, 34
IRQ 3, 35
IRQ 4, 36
IRQ 5, 37
IRQ 6, 38
IRQ 7, 39
IRQ 8, 40
IRQ 9, 41
IRQ 10, 42
IRQ 11, 43
IRQ 12, 44
IRQ 13, 45
IRQ 14, 46
IRQ 15, 47

extern exception_handler
extern timer_handler
extern keyboard_handler

isr_common_stub:
    pusha                   ; Pushes edi,esi,ebp,esp,ebx,edx,ecx,eax
    mov ax, ds              ; Lower 16-bits of eax = ds
    push eax                ; save data segment descriptor

    mov ax, 0x10            ; load kernel data segment descriptor
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push esp                ; Pass pointer to registers_t
    call exception_handler
    add esp, 4              ; Clean up pushed pointer

    pop eax                 ; reload original data segment descriptor
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    popa                    ; Pops edi,esi,ebp...
    add esp, 8              ; Cleans up pushed error code and pushed ISR number
    sti
    iretd                   ; pops 5 things: CS, EIP, EFLAGS, SS, ESP

irq_common_stub:
    pusha
    mov ax, ds
    push eax
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    mov eax, [esp + 36]     ; Get interrupt number from stack (32 stack + 4 seg)
    
    cmp eax, 32
    je .timer
    cmp eax, 33
    je .keyboard
    jmp .done

.timer:
    call timer_handler
    jmp .done
.keyboard:
    call keyboard_handler
    jmp .done

.done:
    pop eax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    popa
    add esp, 8
    sti
    iretd

global page_fault_asm_handler
page_fault_asm_handler:
    jmp isr14

global dummy_exception_handler
dummy_exception_handler:
    jmp isr31