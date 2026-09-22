#ifndef MYOS_PROCESS_H
#define MYOS_PROCESS_H

#include <myos/types.h>
#include <myos/vmm.h>

#define MAX_PROCESSES 64
#define MAX_THREADS   128
#define KERNEL_STACK_SIZE 16384 /* 16 KiB */
#define USER_STACK_SIZE   32768 /* 32 KiB */

typedef enum {
    PROCESS_STATE_UNUSED = 0,
    PROCESS_STATE_EMBRYO,
    PROCESS_STATE_READY,
    PROCESS_STATE_RUNNING,
    PROCESS_STATE_BLOCKED,
    PROCESS_STATE_SLEEPING,
    PROCESS_STATE_ZOMBIE,
    PROCESS_STATE_TERMINATED
} process_state_t;

typedef struct cpu_context {
    uint64_t r15;
    uint64_t r14;
    uint64_t r13;
    uint64_t r12;
    uint64_t rbx;
    uint64_t rbp;
    uint64_t rip;
    uint64_t rsp;
} cpu_context_t;

struct process;

typedef struct thread {
    uint32_t tid;
    uint32_t pid;
    process_state_t state;
    uint8_t priority;
    uint64_t kernel_stack;
    uint64_t user_stack;
    uint64_t rsp;
    uint64_t stack_base;
    size_t   stack_size;
    cpu_context_t context;
    uint64_t sleep_until_tick;
    uint64_t ticks_executed;
    uint32_t time_slice;
    struct thread* next;
    struct process* owner;
} thread_t;

typedef struct process {
    uint32_t pid;
    uint32_t parent_pid;
    char name[32];
    process_state_t state;
    int exit_code;
    uint8_t priority;
    page_directory_t* page_directory;
    page_directory_t* page_dir;
    uint64_t kernel_stack;
    thread_t* main_thread;
    thread_t* threads;
    uint32_t thread_count;
    uint64_t cpu_time_ticks;
    size_t memory_used;
} process_t;

void process_init(void);
process_t* process_create(const char* name, void (*entry_point)(void), uint8_t priority);
void process_terminate(process_t* proc, int exit_code);
void process_destroy(process_t* proc);
process_t* process_get_current(void);
process_t* process_get_by_pid(uint32_t pid);
uint32_t process_get_count(void);
const process_t* process_get_table(uint32_t* out_count);
const process_t* process_get_at(uint32_t index);

#endif /* MYOS_PROCESS_H */
