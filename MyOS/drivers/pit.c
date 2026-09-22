#include <myos/kernel.h>
#include <myos/cpu.h>
#include <myos/idt.h>
#include <myos/scheduler.h>

#define PIT_CHANNEL0 0x40
#define PIT_COMMAND  0x43
#define PIT_BASE_FREQ 1193182
#define TARGET_FREQ  1000 /* 1000 Hz = 1 ms per tick */

static volatile uint64_t g_pit_ticks = 0;

static void pit_interrupt_handler(struct interrupt_registers* regs) {
    (void)regs;
    g_pit_ticks++;

    /* Trigger preemptive scheduler slice if running */
    scheduler_tick();
}

void pit_init(void) {
    uint32_t divisor = PIT_BASE_FREQ / TARGET_FREQ;

    /* Channel 0, lobyte/hibyte, Mode 3 (Square wave generator) */
    outb(PIT_COMMAND, 0x36);
    outb(PIT_CHANNEL0, (uint8_t)(divisor & 0xFF));
    outb(PIT_CHANNEL0, (uint8_t)((divisor >> 8) & 0xFF));

    register_interrupt_handler(32, pit_interrupt_handler); /* IRQ0 -> Vector 32 */
    pic_unmask_irq(0);

    LOG_INFO("PIT", "Programmable Interval Timer set to %d Hz", TARGET_FREQ);
}

uint64_t pit_get_ticks(void) {
    return g_pit_ticks;
}

void pit_sleep_ms(uint32_t ms) {
    uint64_t target = g_pit_ticks + ms;
    while (g_pit_ticks < target) {
        hlt();
    }
}
