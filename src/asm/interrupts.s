bits 32
extern keyboard_handler
extern timer_handler

global keyboard_asm_handler
global timer_asm_handler
global irq_common_stub

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