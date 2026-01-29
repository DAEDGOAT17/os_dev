global keyboard_asm_handler
global timer_asm_handler
global page_fault_asm_handler
global dummy_exception_handler

extern keyboard_handler
extern timer_handler
extern page_fault_handler

; Page Fault Handler (IDT 14)
; Note: CPU pushes an error code onto the stack automatically for Page Fault
page_fault_asm_handler:
    pusha                   ; Push EDI, ESI, EBP, ESP, EBX, EDX, ECX, EAX
    
    mov eax, cr2            ; The address that caused the fault is in CR2
    push eax                ; Pass CR2 as the second argument
    
    mov eax, [esp + 36]     ; The error code is pushed by CPU before pusha (32 bytes) + CR2 (4 bytes)
    push eax                ; Pass error code as the first argument
    
    call page_fault_handler
    
    add esp, 8              ; Clean up the two arguments we pushed
    popa                    ; Restore registers
    add esp, 4              ; Clean up the error code pushed by the CPU
    iretd

; Generic Exception Handler (for IDT 0-31)
dummy_exception_handler:
    cli                     ; Disable interrupts immediately
    pusha                   ; Save all registers
    ; You could call a C function here to print "CPU EXCEPTION"
.hang:
    hlt                     ; Halt the CPU
    jmp .hang               ; Permanent loop to stop oscillation

; Timer Handler (IRQ 0 -> IDT 32)
timer_asm_handler:
    pusha
    call timer_handler
    popa
    iretd

; Keyboard Handler (IRQ 1 -> IDT 33)
keyboard_asm_handler:
    pusha
    call keyboard_handler
    popa
    iretd