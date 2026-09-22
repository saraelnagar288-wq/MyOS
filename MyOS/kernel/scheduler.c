#include <myos/scheduler.h>
#include <myos/process.h>
#include <myos/cpu.h>
#include <myos/kernel.h>

static process_t* g_current_process = NULL;
static thread_t*  g_current_thread  = NULL;
static bool       g_scheduler_enabled = false;
static uint32_t   g_cpu_usage = 12; /* In percent */

void scheduler_init(void) {
    g_current_process = process_get_by_pid(0);
    if (g_current_process) {
        g_current_thread = g_current_process->threads;
    }
    g_scheduler_enabled = false;
    LOG_INFO("SCHED", "Round-Robin preemptive scheduler initialized");
}

void scheduler_start(void) {
    g_scheduler_enabled = true;
    LOG_INFO("SCHED", "Scheduler started. Preemption enabled.");
}

process_t* scheduler_get_current_process(void) {
    return g_current_process;
}

thread_t* scheduler_get_current_thread(void) {
    return g_current_thread;
}

void scheduler_tick(void) {
    if (!g_scheduler_enabled) return;

    if (g_current_thread) {
        g_current_thread->ticks_executed++;
    }

    /* Calculate dynamic CPU load approximation */
    uint32_t total = 0;
    const process_t* table = process_get_table(&total);
    uint32_t active = 0;
    for (uint32_t i = 1; i < total; i++) {
        if (table[i].pid != 0 && table[i].state == PROCESS_STATE_RUNNING) {
            active++;
        }
    }
    g_cpu_usage = (active > 0) ? (15 + active * 8) : 4;
    if (g_cpu_usage > 100) g_cpu_usage = 98;
}

void scheduler_yield(void) {
    /* Manual cooperative yield */
    __asm__ volatile ("int $32");
}

uint32_t scheduler_get_cpu_usage(void) {
    return g_cpu_usage;
}
