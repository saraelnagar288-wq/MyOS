#include <myos/idt.h>
#include <myos/kernel.h>
#include <myos/cpu.h>

extern void idt_load(struct idt_ptr* ptr);

/* ISR Stub declarations from idt_stubs.asm */
extern void isr0(void);  extern void isr1(void);  extern void isr2(void);  extern void isr3(void);
extern void isr4(void);  extern void isr5(void);  extern void isr6(void);  extern void isr7(void);
extern void isr8(void);  extern void isr9(void);  extern void isr10(void); extern void isr11(void);
extern void isr12(void); extern void isr13(void); extern void isr14(void); extern void isr15(void);
extern void isr16(void); extern void isr17(void); extern void isr18(void); extern void isr19(void);
extern void isr20(void); extern void isr21(void); extern void isr22(void); extern void isr23(void);
extern void isr24(void); extern void isr25(void); extern void isr26(void); extern void isr27(void);
extern void isr28(void); extern void isr29(void); extern void isr30(void); extern void isr31(void);

extern void isr32(void); extern void isr33(void); extern void isr34(void); extern void isr35(void);
extern void isr36(void); extern void isr37(void); extern void isr38(void); extern void isr39(void);
extern void isr40(void); extern void isr41(void); extern void isr42(void); extern void isr43(void);
extern void isr44(void); extern void isr45(void); extern void isr46(void); extern void isr47(void);

extern void isr128(void);

static void* s_isr_table[48] = {
    isr0,  isr1,  isr2,  isr3,  isr4,  isr5,  isr6,  isr7,
    isr8,  isr9,  isr10, isr11, isr12, isr13, isr14, isr15,
    isr16, isr17, isr18, isr19, isr20, isr21, isr22, isr23,
    isr24, isr25, isr26, isr27, isr28, isr29, isr30, isr31,
    isr32, isr33, isr34, isr35, isr36, isr37, isr38, isr39,
    isr40, isr41, isr42, isr43, isr44, isr45, isr46, isr47
};

static struct idt_entry s_idt[256];
static struct idt_ptr s_idt_ptr;
static isr_handler_t s_handlers[256];

static const char* s_exception_messages[] = {
    "Division By Zero",
    "Debug Exception",
    "Non Maskable Interrupt",
    "Breakpoint",
    "Into Detected Overflow",
    "Out of Bounds",
    "Invalid Opcode",
    "No Coprocessor",
    "Double Fault",
    "Coprocessor Segment Overrun",
    "Bad TSS",
    "Segment Not Present",
    "Stack Fault",
    "General Protection Fault",
    "Page Fault",
    "Unknown Interrupt",
    "x87 FPU Floating-Point Error",
    "Alignment Check",
    "Machine Check",
    "SIMD Floating-Point Exception",
    "Virtualization Exception",
    "Control Protection Exception",
    "Reserved", "Reserved", "Reserved", "Reserved",
    "Reserved", "Reserved", "Reserved", "Reserved",
    "Security Exception",
    "Reserved"
};

void idt_set_gate(uint8_t num, uint64_t base, uint16_t sel, uint8_t flags) {
    s_idt[num].offset_low  = (uint16_t)(base & 0xFFFF);
    s_idt[num].selector    = sel;
    s_idt[num].ist         = 0;
    s_idt[num].type_attr   = flags;
    s_idt[num].offset_mid  = (uint16_t)((base >> 16) & 0xFFFF);
    s_idt[num].offset_high = (uint32_t)((base >> 32) & 0xFFFFFFFF);
    s_idt[num].zero        = 0;
}

void register_interrupt_handler(uint8_t n, isr_handler_t handler) {
    s_handlers[n] = handler;
}

void unregister_interrupt_handler(uint8_t n) {
    s_handlers[n] = NULL;
}

void isr_dispatch(struct interrupt_registers* regs) {
    if (s_handlers[regs->int_no] != NULL) {
        s_handlers[regs->int_no](regs);
    } else if (regs->int_no < 32) {
        /* Unhandled CPU Exception */
        LOG_ERROR("EXCEPTION", "Fatal Exception %llu: %s (err=0x%llx)",
                  regs->int_no,
                  (regs->int_no < 32) ? s_exception_messages[regs->int_no] : "Unknown",
                  regs->err_code);
        LOG_ERROR("CPU_DUMP", "RIP=0x%016llx CS=0x%llx RFLAGS=0x%016llx RSP=0x%016llx SS=0x%llx",
                  regs->rip, regs->cs, regs->rflags, regs->rsp, regs->ss);
        LOG_ERROR("REG_DUMP", "RAX=0x%016llx RBX=0x%016llx RCX=0x%016llx RDX=0x%016llx",
                  regs->rax, regs->rbx, regs->rcx, regs->rdx);
        LOG_ERROR("REG_DUMP", "RSI=0x%016llx RDI=0x%016llx RBP=0x%016llx",
                  regs->rsi, regs->rdi, regs->rbp);
        if (regs->int_no == 14) {
            uint64_t cr2 = read_cr2();
            LOG_ERROR("PAGE_FAULT", "Faulting Address CR2=0x%016llx", cr2);
        }
        kernel_panic("Unhandled CPU Exception", __FILE__, __LINE__);
    }

    /* Send EOI to PIC if this was a hardware IRQ (32..47) */
    if (regs->int_no >= 32 && regs->int_no < 48) {
        pic_send_eoi((uint8_t)(regs->int_no - 32));
    }
}

void idt_init(void) {
    kmemset(s_idt, 0, sizeof(s_idt));
    kmemset(s_handlers, 0, sizeof(s_handlers));

    s_idt_ptr.limit = sizeof(s_idt) - 1;
    s_idt_ptr.base  = (uint64_t)&s_idt;

    /* Initialize 8259 PIC (remap IRQ 0..15 to IDT 32..47) */
    pic_init();

    /* Populate standard exception and hardware IRQ gates */
    for (int i = 0; i < 48; i++) {
        idt_set_gate(i, (uint64_t)s_isr_table[i], 0x08, 0x8E);
    }

    /* Syscall gate (0x80) with Ring 3 accessibility (DPL=3: 0xEE) */
    idt_set_gate(128, (uint64_t)isr128, 0x08, 0xEE);

    idt_load(&s_idt_ptr);

    LOG_INFO("IDT", "Interrupt Descriptor Table (256 gates) and 8259 PIC initialized");
}
