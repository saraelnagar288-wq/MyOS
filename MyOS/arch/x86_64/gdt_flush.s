BITS 64
section .text
global gdt_flush
global tss_flush

; gdt_flush(uint64_t gdt_ptr_addr)
; RDI = pointer to gdt_ptr
gdt_flush:
    lgdt [rdi]

    ; Reload segment registers
    mov ax, 0x10    ; Kernel Data Segment (0x10)
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; Reload CS via far return
    pop rdi         ; Save return address
    push 0x08       ; New CS (Kernel Code Segment)
    push rdi        ; Push return address back
    retfq

; tss_flush(uint16_t tss_sel)
; RDI = TSS segment selector (0x28)
tss_flush:
    ltr di
    ret
