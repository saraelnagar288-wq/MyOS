#ifndef MYOS_IDT_H
#define MYOS_IDT_H

#include <myos/types.h>

struct idt_entry {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t  ist;
    uint8_t  type_attr;
    uint16_t offset_mid;
    uint32_t offset_high;
    uint32_t zero;
} __attribute__((packed));

struct idt_ptr {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

struct interrupt_registers {
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;
    uint64_t int_no, err_code;
    uint64_t rip, cs, rflags, rsp, ss;
} __attribute__((packed));

typedef void (*isr_handler_t)(struct interrupt_registers* regs);

void idt_init(void);
void idt_set_gate(uint8_t num, uint64_t base, uint16_t sel, uint8_t flags);
void register_interrupt_handler(uint8_t n, isr_handler_t handler);
void unregister_interrupt_handler(uint8_t n);

void pic_init(void);
void pic_send_eoi(uint8_t irq);
void pic_unmask_irq(uint8_t irq);
void pic_mask_irq(uint8_t irq);

#endif /* MYOS_IDT_H */
