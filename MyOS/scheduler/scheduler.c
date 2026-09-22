#include <myos/scheduler.h>
#include <myos/process.h>
#include <myos/kernel.h>
#include <myos/cpu.h>
#include <myos/gdt.h>

extern void switch_context(cpu_context_t* old_ctx, cpu_context_t* new_ctx, uint64_t new_cr3);

static thread_t* s_threads[MAX_THREADS];
static uint32_t s_thread_count = 0;
static uint32_t s_current_index = 0;
static thread_t* s_current_thread = NULL;
static bool s_scheduler_active = false;

void scheduler_init(void) {
    kmemset(s_threads, 0, sizeof(s_threads));
    s_thread_count = 0;
    s_current_index = 0;
    s_current_thread = NULL;
    s_scheduler_active = true;
    LOG_INFO("SCHED", "Preemptive Round-Robin Scheduler active");
}

void scheduler_add_thread(thread_t* th) {
    if (!th || s_thread_count >= MAX_THREADS) return;
    s_threads[s_thread_count++] = th;
    if (s_current_thread == NULL) {
        s_current_thread = th;
    }
}

void scheduler_remove_thread(thread_t* th) {
    if (!th) return;
    for (uint32_t i = 0; i < s_thread_count; i++) {
        if (s_threads[i] == th) {
            for (uint32_t j = i; j < s_thread_count - 1; j++) {
                s_threads[j] = s_threads[j + 1];
            }
            s_threads[--s_thread_count] = NULL;
            break;
        }
    }
}

void scheduler_tick(void) {
    if (!s_scheduler_active || s_thread_count <= 1) return;

    /* Wake up sleeping threads */
    for (uint32_t i = 0; i < s_thread_count; i++) {
        if (s_threads[i]->state == PROCESS_STATE_SLEEPING && s_threads[i]->sleep_until_tick > 0) {
            s_threads[i]->sleep_until_tick--;
            if (s_threads[i]->sleep_until_tick == 0) {
                s_threads[i]->state = PROCESS_STATE_READY;
            }
        }
    }

    if (s_current_thread) {
        s_current_thread->ticks_executed++;
        if (s_current_thread->time_slice > 0) {
            s_current_thread->time_slice--;
        }

        if (s_current_thread->time_slice == 0) {
            scheduler_yield();
        }
    }
}

void scheduler_yield(void) {
    if (!s_scheduler_active || s_thread_count <= 1) return;

    thread_t* prev = s_current_thread;
    if (prev && prev->state == PROCESS_STATE_RUNNING) {
        prev->state = PROCESS_STATE_READY;
    }

    /* Find next READY thread */
    uint32_t next_idx = (s_current_index + 1) % s_thread_count;
    uint32_t checked = 0;
    while (s_threads[next_idx]->state != PROCESS_STATE_READY && checked < s_thread_count) {
        next_idx = (next_idx + 1) % s_thread_count;
        checked++;
    }

    if (s_threads[next_idx]->state != PROCESS_STATE_READY) {
        return; /* No other thread is ready */
    }

    s_current_index = next_idx;
    s_current_thread = s_threads[next_idx];
    s_current_thread->state = PROCESS_STATE_RUNNING;
    s_current_thread->time_slice = 10;

    process_t* proc = process_get_by_pid(s_current_thread->pid);
    uint64_t cr3 = proc && proc->page_directory ? proc->page_directory->cr3_phys : 0;

    /* Update TSS kernel stack for Ring 3 interrupts */
    tss_set_stack(s_current_thread->kernel_stack);

    cpu_context_t* old_ctx = prev ? &prev->context : NULL;
    cpu_context_t* new_ctx = &s_current_thread->context;

    switch_context(old_ctx, new_ctx, cr3);
}

void scheduler_sleep(uint32_t ticks) {
    if (!s_current_thread) return;
    s_current_thread->state = PROCESS_STATE_SLEEPING;
    s_current_thread->sleep_until_tick = ticks;
    scheduler_yield();
}

thread_t* scheduler_get_current_thread(void) {
    return s_current_thread;
}
