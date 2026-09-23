BITS 32

MB_MAGIC    equ 0x1BADB002
MB_FLAGS    equ (1 << 0) | (1 << 1) | (1 << 2)
MB_CHECKSUM equ -(MB_MAGIC + MB_FLAGS)

section .multiboot
align 4
    dd MB_MAGIC
    dd MB_FLAGS
    dd MB_CHECKSUM
    dd 0, 0, 0, 0, 0
    dd 0
    dd 1024
    dd 768
    dd 32

section .bss
align 4096
pml4_table: resb 4096
pdp_table:  resb 4096
pd_table0:  resb 4096
pd_table1:  resb 4096
pd_table2:  resb 4096
pd_table3:  resb 4096
stack_bottom: resb 32768
stack_top:

section .data
align 16
gdt64:
    dq 0
.code equ $ - gdt64
    dq (1 << 43) | (1 << 44) | (1 << 47) | (1 << 53)
.data equ $ - gdt64
    dq (1 << 41) | (1 << 44) | (1 << 47)
.pointer:
    dw $ - gdt64 - 1
    dq gdt64

section .text
global _start
extern kernel_main

_start:
    cli
    mov esp, stack_top

    cmp eax, 0x2BADB002
    jne .bad_multiboot

    call check_cpuid
    call check_long_mode
    call setup_page_tables
    call enable_paging

    lgdt [gdt64.pointer]
    jmp gdt64.code:long_mode_start

.bad_multiboot:
    mov word [0xb8000], 0x4f4d
    mov word [0xb8002], 0x4f42
    hlt
    jmp $

check_cpuid:
    pushfd
    pop eax
    mov ecx, eax
    xor eax, 1 << 21
    push eax
    popfd
    pushfd
    pop eax
    push ecx
    popfd
    cmp eax, ecx
    je .no_cpuid
    ret
.no_cpuid:
    mov word [0xb8000], 0x4f43
    hlt
    jmp $

check_long_mode:
    mov eax, 0x80000000
    cpuid
    cmp eax, 0x80000001
    jb .no_long_mode
    mov eax, 0x80000001
    cpuid
    test edx, 1 << 29
    jz .no_long_mode
    ret
.no_long_mode:
    mov word [0xb8000], 0x4f4c
    hlt
    jmp $

setup_page_tables:
    mov eax, pdp_table
    or eax, 0x3
    mov [pml4_table], eax

    mov eax, pd_table0
    or eax, 0x3
    mov [pdp_table], eax
    mov eax, pd_table1
    or eax, 0x3
    mov [pdp_table + 8], eax
    mov eax, pd_table2
    or eax, 0x3
    mov [pdp_table + 16], eax
    mov eax, pd_table3
    or eax, 0x3
    mov [pdp_table + 24], eax

    mov edi, pd_table0
    xor ebx, ebx
    call map_pd
    mov edi, pd_table1
    mov ebx, 512
    call map_pd
    mov edi, pd_table2
    mov ebx, 1024
    call map_pd
    mov edi, pd_table3
    mov ebx, 1536
    call map_pd
    ret

map_pd:
    xor ecx, ecx
.map:
    mov eax, ebx
    add eax, ecx
    shl eax, 21
    or eax, 0x83
    mov [edi + ecx * 8], eax
    inc ecx
    cmp ecx, 512
    jne .map
    ret

enable_paging:
    mov eax, pml4_table
    mov cr3, eax

    mov eax, cr4
    or eax, 1 << 5
    mov cr4, eax

    mov ecx, 0xC0000080
    rdmsr
    or eax, 1 << 8
    wrmsr

    mov eax, cr0
    or eax, 1 << 31
    mov cr0, eax
    ret

BITS 64
long_mode_start:
    xor eax, eax
    mov ss, ax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    mov rsp, stack_top

    ; System V AMD64: RDI = multiboot magic, RSI = multiboot info pointer.
    mov edi, 0x2BADB002
    mov esi, ebx
    call kernel_main

.hang:
    cli
    hlt
    jmp .hang
