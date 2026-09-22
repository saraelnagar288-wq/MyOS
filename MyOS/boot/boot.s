; ==============================================================================
; MyOS Bootloader Entry & x86_64 Long Mode Trampoline
; Multiboot 1 Compliant
; ==============================================================================

BITS 32

; Multiboot 1 Constants
MB_MAGIC        equ 0x1BADB002
MB_ALIGNED      equ 1 << 0
MB_MEMINFO      equ 1 << 1
MB_VIDINFO      equ 1 << 2
MB_FLAGS        equ MB_ALIGNED | MB_MEMINFO | MB_VIDINFO
MB_CHECKSUM     equ -(MB_MAGIC + MB_FLAGS)

section .multiboot
align 4
    dd MB_MAGIC
    dd MB_FLAGS
    dd MB_CHECKSUM
    ; A.out header kludge (unused)
    dd 0, 0, 0, 0, 0
    ; Video mode request: linear 1024x768x32
    dd 0    ; mode_type: linear graphics framebuffer
    dd 1024 ; width
    dd 768  ; height
    dd 32   ; depth (bpp)

section .bss
align 4096
; Initial Page Tables (Identity mapping the first 1 GiB using 2MiB huge pages)
pml4_table:
    resb 4096
pdp_table:
    resb 4096
pd_table:
    resb 4096

; Early Kernel Stack (32 KiB)
stack_bottom:
    resb 32768
stack_top:

mb_magic_store:
    resd 1
mb_info_store:
    resd 1

section .data
align 16
; 64-bit Global Descriptor Table
gdt64:
    dq 0 ; Null descriptor
.code: equ $ - gdt64
    dq (1 << 43) | (1 << 44) | (1 << 47) | (1 << 53) ; 64-bit Code: Executable, Descriptor, Present, 64-bit
.data: equ $ - gdt64
    dq (1 << 41) | (1 << 44) | (1 << 47)             ; 64-bit Data: Writable, Descriptor, Present
.pointer:
    dw $ - gdt64 - 1
    dq gdt64

section .text
global _start
extern kernel_main

_start:
    ; Disable interrupts
    cli

    ; Initialize stack pointer
    mov esp, stack_top

    ; Save Multiboot registers (EAX = magic, EBX = info structure pointer)
    mov [mb_magic_store], eax
    mov [mb_info_store], ebx

    ; Check if Multiboot magic is valid (0x2BADB002)
    cmp eax, 0x2BADB002
    jne .no_multiboot

    ; Check CPUID availability
    call check_cpuid
    ; Check Long Mode availability
    call check_long_mode

    ; Setup initial paging (identity map 1 GiB)
    call setup_page_tables
    call enable_paging

    ; Load 64-bit GDT
    lgdt [gdt64.pointer]

    ; Far jump to 64-bit long mode code
    jmp gdt64.code:long_mode_start

.no_multiboot:
    mov dword [0xb8000], 0x4f4d4f4e ; "NO"
    hlt
    jmp $

; ------------------------------------------------------------------------------
; Verification Functions (32-bit)
; ------------------------------------------------------------------------------
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
    mov al, 'C'
    jmp error_32

check_long_mode:
    mov eax, 0x80000000
    cpuid
    cmp eax, 0x80000001
    jb .no_long_mode

    mov eax, 0x80000001
    cpuid
    test edx, 1 << 29 ; Long mode bit
    jz .no_long_mode
    ret
.no_long_mode:
    mov al, 'L'
    jmp error_32

setup_page_tables:
    ; Map first PML4 entry to PDP table
    mov eax, pdp_table
    or eax, 0b11 ; Present + Writable
    mov [pml4_table], eax

    ; Map first PDP entry to PD table
    mov eax, pd_table
    or eax, 0b11 ; Present + Writable
    mov [pdp_table], eax

    ; Map each PD entry to 2MiB huge page (maps 512 * 2MiB = 1 GiB)
    mov ecx, 0
.map_pd_table:
    mov eax, 0x200000 ; 2 MiB
    mul ecx
    or eax, 0b10000011 ; Present + Writable + Huge page (bit 7)
    mov [pd_table + ecx * 8], eax

    inc ecx
    cmp ecx, 512
    jne .map_pd_table
    ret

enable_paging:
    ; Load CR3 with PML4 base address
    mov eax, pml4_table
    mov cr3, eax

    ; Enable PAE (Physical Address Extension) in CR4
    mov eax, cr4
    or eax, 1 << 5
    mov cr4, eax

    ; Enable Long Mode in EFER MSR
    mov ecx, 0xC0000080
    rdmsr
    or eax, 1 << 8
    wrmsr

    ; Enable Paging in CR0
    mov eax, cr0
    or eax, 1 << 31
    mov cr0, eax
    ret

error_32:
    mov dword [0xb8000], 0x4f524f45 ; "ERR"
    hlt
    jmp $

; ==============================================================================
; 64-bit Long Mode Entry
; ==============================================================================
BITS 64
section .text
long_mode_start:
    ; Reload data segments with 0 (64-bit mode)
    mov ax, 0
    mov ss, ax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; Setup 64-bit stack pointer
    mov rsp, stack_top

    ; Load Multiboot parameters
    ; System V AMD64 ABI: RDI = 1st argument (magic), RSI = 2nd argument (mb_info*)
    mov edi, [mb_magic_store]
    mov esi, [mb_info_store]

    ; Call 64-bit kernel main
    call kernel_main

    ; If kernel_main returns, halt CPU
.halt:
    cli
    hlt
    jmp .halt
