BITS 64
section .text
global switch_context

; void switch_context(cpu_context_t* old_ctx, cpu_context_t* new_ctx, uint64_t new_cr3)
; RDI = old_ctx
; RSI = new_ctx
; RDX = new_cr3
switch_context:
    ; Check if old context needs saving
    test rdi, rdi
    jz .load_new

    ; Save callee-saved registers into old_ctx
    mov [rdi + 0x00], r15
    mov [rdi + 0x08], r14
    mov [rdi + 0x10], r13
    mov [rdi + 0x18], r12
    mov [rdi + 0x20], rbx
    mov [rdi + 0x28], rbp

    ; Save return address (RIP) and stack pointer (RSP)
    mov rax, [rsp]       ; Return address from call
    mov [rdi + 0x30], rax
    lea rax, [rsp + 8]   ; Stack pointer before call
    mov [rdi + 0x38], rax

.load_new:
    ; Switch address space (CR3) if non-zero
    test rdx, rdx
    jz .restore_regs
    mov rax, cr3
    cmp rax, rdx
    je .restore_regs
    mov cr3, rdx

.restore_regs:
    ; Restore callee-saved registers from new_ctx
    mov r15, [rsi + 0x00]
    mov r14, [rsi + 0x08]
    mov r13, [rsi + 0x10]
    mov r12, [rsi + 0x18]
    mov rbx, [rsi + 0x20]
    mov rbp, [rsi + 0x28]

    ; Load new stack pointer and jump to new RIP
    mov rsp, [rsi + 0x38]
    push qword [rsi + 0x30] ; Push new RIP onto new stack
    ret
