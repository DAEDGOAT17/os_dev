bits 32

global vmm_load_page_directory
global vmm_enable_paging

vmm_load_page_directory:
    mov eax, [esp + 4]  ; Get the pointer to the page_directory array
    mov cr3, eax        ; CR3 must hold the physical address of the directory
    ret

vmm_enable_paging:
    mov eax, cr0
    or eax, 0x80000000  ; Set bit 31 (PG - Paging) in CR0
    mov cr0, eax
    ret