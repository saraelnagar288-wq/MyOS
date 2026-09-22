; Interrupt Descriptor Table and ISR stubs for x86_64

BITS 64

SECTION .text
global idt_flush_asm
extern isr_handler

idt_flush_asm:
    lidt [rdi]
    ret

; Common ISR stub: saves state, sets up arguments, calls C handler, restores state
isr_common_stub:
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    mov rdi, rsp                ; RDI = pointer to struct interrupt_registers
    cld
    call isr_handler

    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax

    ; Drop int_no and err_code from stack
    add rsp, 16
    iretq

; Macro for exceptions that don't push an error code (dummy error code 0 pushed)
%macro ISR_NOERRCODE 1
isr_stub_%1:
    push qword 0                ; Dummy error code
    push qword %1               ; Interrupt vector number
    jmp isr_common_stub
%endmacro

; Macro for exceptions that do push an error code
%macro ISR_ERRCODE 1
isr_stub_%1:
    push qword %1               ; Interrupt vector number
    jmp isr_common_stub
%endmacro

; Exceptions 0..31
ISR_NOERRCODE 0
ISR_NOERRCODE 1
ISR_NOERRCODE 2
ISR_NOERRCODE 3
ISR_NOERRCODE 4
ISR_NOERRCODE 5
ISR_NOERRCODE 6
ISR_NOERRCODE 7
ISR_ERRCODE   8
ISR_NOERRCODE 9
ISR_ERRCODE   10
ISR_ERRCODE   11
ISR_ERRCODE   12
ISR_ERRCODE   13
ISR_ERRCODE   14
ISR_NOERRCODE 15
ISR_NOERRCODE 16
ISR_ERRCODE   17
ISR_NOERRCODE 18
ISR_NOERRCODE 19
ISR_NOERRCODE 20
ISR_ERRCODE   21
ISR_NOERRCODE 22
ISR_NOERRCODE 23
ISR_NOERRCODE 24
ISR_NOERRCODE 25
ISR_NOERRCODE 26
ISR_NOERRCODE 27
ISR_NOERRCODE 28
ISR_ERRCODE   29
ISR_ERRCODE   30
ISR_NOERRCODE 31

; IRQs and General Interrupts 32..255
%assign i 32
%rep 224
ISR_NOERRCODE i
%assign i i+1
%endrep

SECTION .rodata
global isr_stub_table
isr_stub_table:
%assign i 0
%rep 256
    dq isr_stub_%+i
%assign i i+1
%endrep
