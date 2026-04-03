; =============================================================================
; loader.s  –  x86_64 Jarwis OS entry point
; Boot flow: GRUB multiboot2 → 32-bit protected mode (here) → 64-bit long mode
;            → call kmain(magic:rdi, mbi:rsi)
; =============================================================================

; ── GRUB loads us in 32-bit protected mode regardless of target architecture ──
bits 32

global loader
global gdt_flush
global idt_load
global vmm_load_pml4
global vmm_enable_long_mode
global read_cr2
global invlpg_addr
global pml4_table
extern kmain

; =============================================================================
; Multiboot2 Header  (must appear within first 32 KB of the image)
; =============================================================================
section .multiboot2_header
align 8
.mb2_start:
    dd 0xE85250D6                                        ; Magic
    dd 0                                                 ; Architecture: i386 protected mode
    dd (.mb2_end - .mb2_start)                           ; Header length
    dd -(0xE85250D6 + 0 + (.mb2_end - .mb2_start))       ; Checksum

    ; Framebuffer tag (type=5, size=20)
    align 8
    dw 5        ; Type
    dw 0        ; Flags (0 = required)
    dd 20       ; Size
    dd 0        ; Width  (0 = no preference)
    dd 0        ; Height (0 = no preference)
    dd 0        ; Depth  (0 = no preference)

    ; End tag
    align 8
    dw 0
    dw 0
    dd 8
.mb2_end:

; =============================================================================
; Multiboot1 Header  (legacy BIOS fallback)
; =============================================================================
section .multiboot_header
align 4
    dd 0x1BADB002
    dd 0x05
    dd -(0x1BADB002 + 0x05)
    dd 0 ; header_addr
    dd 0 ; load_addr
    dd 0 ; load_end_addr
    dd 0 ; bss_end_addr
    dd 0 ; entry_addr
    dd 0 ; mode_type
    dd 1024
    dd 768
    dd 32

; =============================================================================
; 32-bit bootstrap: save Multiboot regs, build minimal GDT, enter long mode
; =============================================================================
section .text

; ── Minimal GDT (fits in 32-bit .data for the trampoline) ───────────────────
align 8
gdt64:
    dq 0                          ; Null descriptor
.code: equ $ - gdt64
    dq (1<<43)|(1<<44)|(1<<47)|(1<<53) ; 64-bit code: L=1, P=1, S=1, Ex=1
.data: equ $ - gdt64
    dq (1<<44)|(1<<47)|(1<<41)        ; data: P=1, S=1, W=1
gdt64_ptr:
    dw $ - gdt64 - 1
    dd gdt64       ; 32-bit address (fine before long mode)

loader:
    cli

    ; Save Multiboot values before anything trashes them
    mov [saved_magic], eax
    mov [saved_mbi],   ebx

    ; Set up a temporary 32-bit stack
    mov esp, stack32_top

    ; ── Enable PAE (required for long mode) ─────────────────────────────────
    mov eax, cr4
    or  eax, 1 << 5        ; PAE bit
    mov cr4, eax

    ; ── Set up minimal 4-level page tables (identity map lower 2 GB) ────────
    ; PML4[0] → PDPT
    mov edi, pml4_table
    mov cr3, edi
    mov eax, pdpt_table
    or  eax, 0x3               ; P + RW
    mov [pml4_table], eax

    ; PDPT[0] → PD
    mov eax, pd_table
    or  eax, 0x3
    mov [pdpt_table], eax

    ; PDPT[1] → PD2
    mov eax, pd_table2
    or  eax, 0x3
    mov [pdpt_table + 8], eax

    ; PDPT[2] → PD3
    mov eax, pd_table3
    or  eax, 0x3
    mov [pdpt_table + 16], eax

    ; PDPT[3] → PD4
    mov eax, pd_table4
    or  eax, 0x3
    mov [pdpt_table + 24], eax

    ; PD: 2048 × 2 MB huge pages covering 0 – 4 GB
    mov ecx, 0
.fill_pd:
    mov eax, 0x200000          ; 2 MB
    mul ecx
    or  eax, 0x83              ; P + RW + Huge
    mov [pd_table + ecx*8], eax
    mov dword [pd_table + ecx*8 + 4], 0
    inc ecx
    cmp ecx, 2048
    jl  .fill_pd

    ; ── Activate long mode via EFER MSR ─────────────────────────────────────
    mov ecx, 0xC0000080        ; EFER MSR
    rdmsr
    or  eax, 1 << 8            ; LME bit
    wrmsr

    ; ── Enable paging → long mode active ────────────────────────────────────
    mov eax, cr0
    or  eax, 1 << 31           ; PG
    or  eax, 1 << 0            ; PE (should already be set by GRUB)
    mov cr0, eax

    ; ── Load 64-bit GDT and far-jump to 64-bit code segment ─────────────────
    lgdt [gdt64_ptr]
    push 0x08
    mov eax, .start64
    push eax
    retf

; =============================================================================
bits 64
.start64:
    ; Reload data segments
    mov ax, gdt64.data
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; Set up a proper 64-bit stack
    mov rsp, stack_top

    ; Enable SSE2 (safe after long mode)
    mov rax, cr0
    and eax, 0xFFFFFFFB        ; clear EM
    or  eax, 0x00000002        ; set MP
    mov cr0, rax
    mov rax, cr4
    or  eax, 0x00000600        ; OSFXSR + OSXMMEXCPT
    mov cr4, rax
    fninit

    ; Pass saved Multiboot info as 1st (rdi) and 2nd (rsi) arguments
    ; (Both were stored in 32-bit .data, zero-extend to 64-bit)
    xor rdi, rdi
    xor rsi, rsi
    mov edi, dword [saved_magic]
    mov esi, dword [saved_mbi]

    call kmain

.halt:
    cli
    hlt
    jmp .halt

; =============================================================================
; gdt_flush(uint64_t gdt_ptr_vaddr)  — called from C after repo-init
; RDI = pointer to a struct { uint16_t limit; uint64_t base; }
; =============================================================================
bits 64
gdt_flush:
    lgdt [rdi]
    ; Reload segment registers
    mov ax, 0x10       ; data segment (index 2 x 8 = 0x10)
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    ; Far return to reload CS with selector 0x08
    pop  rax           ; return address
    push qword 0x08    ; new CS
    push rax
    retfq

; =============================================================================
; idt_load(uint64_t idtp_vaddr)
; =============================================================================
idt_load:
    lidt [rdi]
    ret

; =============================================================================
; vmm_load_pml4(uint64_t phys_addr)
; =============================================================================
vmm_load_pml4:
    mov cr3, rdi
    ret

; =============================================================================
; vmm_enable_long_mode()  — no-op (already done), kept for API compat
; =============================================================================
vmm_enable_long_mode:
    ret

; =============================================================================
; read_cr2() → uint64_t
; =============================================================================
read_cr2:
    mov rax, cr2
    ret

; =============================================================================
; invlpg_addr(uint64_t vaddr)
; =============================================================================
invlpg_addr:
    invlpg [rdi]
    ret

; =============================================================================
; 32-bit data (used before long mode)
; =============================================================================
bits 32
section .data
saved_magic: dd 0
saved_mbi:   dd 0

; =============================================================================
; Early boot page tables  (4-KB aligned, placed in BSS)
; =============================================================================
section .bss
align 4096
pml4_table: resb 4096
pdpt_table:  resb 4096
pd_table:    resb 4096
pd_table2:   resb 4096
pd_table3:   resb 4096
pd_table4:   resb 4096

; 32-bit bootstrap stack (small, only used during transition)
align 16
stack32_bottom: resb 4096
stack32_top:

; Final 64-bit kernel stack (64 KB)
align 16
stack_bottom: resb 65536
stack_top:
