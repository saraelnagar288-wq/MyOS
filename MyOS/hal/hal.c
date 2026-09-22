#include <myos/hal.h>
#include <myos/cpu.h>
#include <myos/kernel.h>

extern void pit_init(void);
extern uint64_t pit_get_ticks(void);
extern void pit_sleep_ms(uint32_t ms);
extern void rtc_read(rtc_time_t* t);

void hal_init(void) {
    pit_init();
    LOG_INFO("HAL", "Hardware Abstraction Layer initialized");
}

uint64_t hal_get_uptime_ms(void) {
    return pit_get_ticks();
}

uint64_t hal_get_ticks(void) {
    return pit_get_ticks();
}

void hal_sleep_ms(uint32_t ms) {
    pit_sleep_ms(ms);
}

void hal_get_time(rtc_time_t* t) {
    rtc_read(t);
}

void hal_reboot(void) {
    LOG_INFO("HAL", "Rebooting system...");
    cli();

    /* 1. Pulse 8042 keyboard controller reset pin */
    uint8_t temp;
    do {
        temp = inb(0x64);
        if ((temp & 1) != 0) {
            inb(0x60);
            continue;
        }
    } while ((temp & 2) != 0);
    outb(0x64, 0xFE);

    /* 2. Fallback: load empty IDT and trigger triple fault */
    struct {
        uint16_t limit;
        uint64_t base;
    } __attribute__((packed)) null_idt = {0, 0};
    __asm__ volatile ("lidt %0; int3" : : "m"(null_idt));

    while (1) hlt();
}

void hal_shutdown(void) {
    LOG_INFO("HAL", "Shutting down system...");
    cli();

    /* QEMU / Bochs / VirtualBox ACPI shutdown ports */
    outw(0x604, 0x2000);  /* QEMU -device isa-debug-exit / modern ACPI */
    outw(0xB004, 0x2000); /* Bochs / older QEMU */
    outw(0x4004, 0x3400); /* VirtualBox */

    LOG_INFO("HAL", "System halted. Safe to turn off computer.");
    while (1) hlt();
}
