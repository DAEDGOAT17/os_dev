; =============================================================================
; vmm_asm.s – x86_64 VMM helpers  (no-ops kept for API compatibility)
; The actual CR3 load and long-mode enable are already done in loader.s.
; These stubs are called from vmm.c to reload CR3 when updating the final PML4.
; =============================================================================
bits 64

global vmm_load_page_directory   ; legacy name kept so old code compiles
global vmm_enable_paging          ; no-op

; void vmm_load_page_directory(uint64_t pml4_phys)
; rdi = physical address of PML4 table
vmm_load_page_directory:
    mov cr3, rdi
    ret

; void vmm_enable_paging()
vmm_enable_paging:
    ; Paging is already enabled by loader.s; this is a no-op
    ret
