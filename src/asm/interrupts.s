bits 32
extern keyboard_handler
extern timer_handler
global keyboard_asm_handler
global timer_asm_handler
global dummy_exception_handler

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