; =============================================================================
; interrupts.s – x86_64 interrupt stubs
; In 64-bit mode: no PUSHA/POPA, use IRETQ, System V ABI (args in RDI, RSI)
; =============================================================================
bits 64

global keyboard_asm_handler
global timer_asm_handler
global page_fault_asm_handler
global dummy_exception_handler

extern keyboard_handler
extern timer_handler
extern page_fault_handler

; =============================================================================
; Macro: save / restore caller-saved registers around an IRQ handler
; The CPU has already pushed: SS, RSP, RFLAGS, CS, RIP
; For exceptions WITH error code: error_code is also pushed by CPU (below RIP)
; =============================================================================
%macro SAVE_REGS 0
    push rax
    push rcx
    push rdx
    push rdi
    push rsi
    push r8
    push r9
    push r10
    push r11
%endmacro

%macro RESTORE_REGS 0
    pop r11
    pop r10
    pop r9
    pop r8
    pop rsi
    pop rdi
    pop rdx
    pop rcx
    pop rax
%endmacro

; =============================================================================
; Page Fault Handler (IDT vector 14)
; CPU stack on entry (top → bottom):
;   [rsp+0]  error_code   (pushed by CPU)
;   [rsp+8]  RIP
;   [rsp+16] CS
;   [rsp+24] RFLAGS
;   [rsp+32] RSP (old)
;   [rsp+40] SS
;
; void page_fault_handler(uint64_t error_code, uint64_t faulting_addr)
;   rdi = error_code,  rsi = CR2
; =============================================================================
page_fault_asm_handler:
    SAVE_REGS

    ; error_code is at [rsp + 9*8] (9 saved regs × 8 bytes)
    mov  rdi, [rsp + 9*8]     ; error code  → rdi (arg1)
    mov  rsi, cr2             ; faulting addr → rsi (arg2)

    call page_fault_handler

    RESTORE_REGS
    add  rsp, 8               ; discard error code pushed by CPU
    iretq

; =============================================================================
; Timer Handler (IRQ 0 → IDT vector 32)
; CPU does NOT push an error code for hardware IRQs.
; =============================================================================
timer_asm_handler:
    SAVE_REGS
    call timer_handler
    RESTORE_REGS
    iretq

; =============================================================================
; Keyboard Handler (IRQ 1 → IDT vector 33)
; =============================================================================
keyboard_asm_handler:
    SAVE_REGS
    call keyboard_handler
    RESTORE_REGS
    iretq

; =============================================================================
; Generic/Dummy Exception Handler (vectors 0–31 not otherwise assigned)
; Halts the CPU permanently.
; =============================================================================
dummy_exception_handler:
    cli
.hang:
    hlt
    jmp .hang
