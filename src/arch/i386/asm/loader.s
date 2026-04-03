bits 32
global loader
global gdt_flush
global idt_load
extern kmain

; =============================================
; Multiboot 1 Header (for GRUB BIOS/legacy boot)
; =============================================
section .multiboot_header
    align 4
    dd 0x1BADB002           ; Magic
    dd 0x05                 ; Flags: align modules (1), give memory map (0), video mode (4) -> 0x05
    dd -(0x1BADB002 + 0x05) ; Checksum
    
    ; The following fields are ignored if bit 16 is not set, 
    ; but we need them as padding so the video mode fields are at the right offset (32 bytes).
    dd 0    ; header_addr
    dd 0    ; load_addr
    dd 0    ; load_end_addr
    dd 0    ; bss_end_addr
    dd 0    ; entry_addr
    
    ; Video mode request (valid if bit 2 is set in flags)
    dd 0    ; mode_type (0 = linear graphics mode)
    dd 1024 ; width
    dd 768  ; height
    dd 32   ; depth

; =============================================
; Multiboot 2 Header (for GRUB EFI / UEFI boot)
; Must be 8-byte aligned and within first 32KB
; =============================================
section .multiboot2_header
    align 8
.mb2_start:
    dd 0xE85250D6           ; Magic
    dd 0                    ; Architecture: 0 = i386 protected mode
    dd (.mb2_end - .mb2_start) ; Header length
    dd -(0xE85250D6 + 0 + (.mb2_end - .mb2_start)) ; Checksum

    ; Framebuffer tag (type=5, size=20)
    align 8
    dw 5                    ; Type: framebuffer
    dw 0                    ; Flags
    dd 20                   ; Size
    dd 0                    ; Width
    dd 0                    ; Height
    dd 0                    ; Depth

    align 8
    ; End tag (type=0, size=8)
    dw 0                    ; Type: end
    dw 0                    ; Flags
    dd 8                    ; Size
.mb2_end:

section .text
loader:
    cli                     ; Disable interrupts immediately
    
    ; Save Multiboot magic (eax) and info pointer (ebx) BEFORE anything modifies them
    mov [saved_magic], eax
    mov [saved_mbi],   ebx

    ; Set up a fresh stack
    mov esp, stack_top

    ; Enable FPU: clear CR0.EM (bit2), set CR0.MP (bit1) and CR0.NE (bit5)
    mov eax, cr0
    and eax, 0xFFFFFFFB
    or  eax, 0x00000022
    mov cr0, eax

    ; Enable SSE: set CR4.OSFXSR (bit9) and CR4.OSXMMEXCPT (bit10)
    mov eax, cr4
    or  eax, 0x00000600
    mov cr4, eax

    ; Initialize x87 FPU state
    fninit

    ; Call kmain(magic, mbi*) with saved values
    push dword [saved_mbi]
    push dword [saved_magic]
    call kmain
    
    cli
    hlt

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

section .data
saved_magic: dd 0
saved_mbi:   dd 0

section .bss
align 16
stack_bottom: 
    resb 16384 ; 16 KB
stack_top:     ; ESP points here (grows downwards)