#include <myos/kernel.h>
#include <myos/cpu.h>

#define COM1_PORT 0x3F8

static bool g_serial_initialized = false;

void serial_init(void) {
    outb(COM1_PORT + 1, 0x00);    /* Disable all interrupts */
    outb(COM1_PORT + 3, 0x80);    /* Enable DLAB (set baud rate divisor) */
    outb(COM1_PORT + 0, 0x01);    /* Set divisor to 1 (low byte) 115200 baud */
    outb(COM1_PORT + 1, 0x00);    /*                  (high byte) */
    outb(COM1_PORT + 3, 0x03);    /* 8 bits, no parity, one stop bit */
    outb(COM1_PORT + 2, 0xC7);    /* Enable FIFO, clear them, with 14-byte threshold */
    outb(COM1_PORT + 4, 0x0B);    /* IRQs enabled, RTS/DSR set */

    g_serial_initialized = true;
    LOG_INFO("SERIAL", "COM1 115200 8N1 initialized for kernel debugging");
}

static int serial_is_transmit_empty(void) {
    return inb(COM1_PORT + 5) & 0x20;
}

void serial_putc(char c) {
    if (!g_serial_initialized) return;
    while (serial_is_transmit_empty() == 0);
    outb(COM1_PORT, (uint8_t)c);
}

void serial_write_char(char c) {
    serial_putc(c);
}

void serial_puts(const char* str) {
    if (!str) return;
    while (*str) {
        if (*str == '\n') {
            serial_putc('\r');
        }
        serial_putc(*str++);
    }
}
