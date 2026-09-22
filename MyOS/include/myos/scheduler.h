#ifndef MYOS_SCHEDULER_H
#define MYOS_SCHEDULER_H

#include <myos/types.h>
#include <myos/process.h>

void scheduler_init(void);
void scheduler_start(void);
void scheduler_add_thread(thread_t* thread);
void scheduler_remove_thread(thread_t* thread);
void scheduler_yield(void);
void scheduler_tick(void);
process_t* scheduler_get_current_process(void);
thread_t* scheduler_get_current_thread(void);
void scheduler_sleep(uint32_t ticks);
uint32_t scheduler_get_cpu_usage(void);

#endif /* MYOS_SCHEDULER_H */
