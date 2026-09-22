#include <myos/idt.h>
#include <myos/cpu.h>
#include <myos/kernel.h>

#define PIC1_CMD  0x20
#define PIC1_DATA 0x21
#define PIC2_CMD  0xA0
#define PIC2_DATA 0xA1

#define ICW1_INIT 0x11
#define ICW4_8086 0x01
#define PIC_EOI   0x20

void pic_init(void) {
    uint8_t mask1 = inb(PIC1_DATA);
    uint8_t mask2 = inb(PIC2_DATA);

    /* ICW1: Start initialization in cascade mode */
    outb(PIC1_CMD, ICW1_INIT);
    io_wait();
    outb(PIC2_CMD, ICW1_INIT);
    io_wait();

    /* ICW2: Remap vector offsets: Master=32 (0x20), Slave=40 (0x28) */
    outb(PIC1_DATA, 0x20);
    io_wait();
    outb(PIC2_DATA, 0x28);
    io_wait();

    /* ICW3: Master-Slave cascading connection */
    outb(PIC1_DATA, 0x04); /* Slave at IRQ2 */
    io_wait();
    outb(PIC2_DATA, 0x02); /* Cascade identity */
    io_wait();

    /* ICW4: Set 8086 mode */
    outb(PIC1_DATA, ICW4_8086);
    io_wait();
    outb(PIC2_DATA, ICW4_8086);
    io_wait();

    /* Restore masks (or mask all except timer, keyboard, cascade, and mouse) */
    outb(PIC1_DATA, 0xF8); /* Enable IRQ0 (Timer), IRQ1 (Keyboard), IRQ2 (Cascade) */
    outb(PIC2_DATA, 0xEF); /* Enable IRQ12 (PS/2 Mouse) */

    LOG_INFO("PIC", "8259 Programmable Interrupt Controller remapped to 0x20-0x2F");
}

void pic_send_eoi(uint8_t irq) {
    if (irq >= 8) {
        outb(PIC2_CMD, PIC_EOI);
    }
    outb(PIC1_CMD, PIC_EOI);
}

void pic_unmask_irq(uint8_t irq) {
    uint16_t port;
    uint8_t value;

    if (irq < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        irq -= 8;
    }
    value = inb(port) & ~(1 << irq);
    outb(port, value);
}

void pic_mask_irq(uint8_t irq) {
    uint16_t port;
    uint8_t value;

    if (irq < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        irq -= 8;
    }
    value = inb(port) | (1 << irq);
    outb(port, value);
}
