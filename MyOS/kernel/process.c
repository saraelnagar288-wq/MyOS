#include <myos/process.h>
#include <myos/kheap.h>
#include <myos/kernel.h>

static process_t g_process_table[MAX_PROCESSES];
static uint32_t  g_next_pid = 1;
static uint32_t  g_next_tid = 1;

void process_init(void) {
    kmemset(g_process_table, 0, sizeof(g_process_table));

    /* Create Kernel Idle Process (PID 0) */
    process_t* idle = &g_process_table[0];
    idle->pid = 0;
    kstrcpy(idle->name, "kernel_idle");
    idle->state = PROCESS_STATE_RUNNING;
    idle->page_dir = vmm_get_kernel_directory();

    thread_t* idle_thread = (thread_t*)kzalloc(sizeof(thread_t));
    idle_thread->tid = 0;
    idle_thread->owner = idle;
    idle_thread->state = PROCESS_STATE_RUNNING;
    idle_thread->priority = 0;
    idle->threads = idle_thread;

    LOG_INFO("PROC", "Process subsystem initialized with Idle process (PID 0)");
}

process_t* process_create(const char* name, void (*entry_point)(void), uint8_t priority) {
    for (int i = 1; i < MAX_PROCESSES; i++) {
        if (g_process_table[i].pid == 0) {
            process_t* proc = &g_process_table[i];
            proc->pid = g_next_pid++;
            kstrcpy(proc->name, name);
            proc->state = PROCESS_STATE_READY;
            proc->page_dir = vmm_create_address_space();

            /* Allocate 64 KiB kernel stack */
            void* kstack = kmalloc(64 * 1024);
            proc->kernel_stack = (uint64_t)kstack + (64 * 1024);

            thread_t* thread = (thread_t*)kzalloc(sizeof(thread_t));
            thread->tid = g_next_tid++;
            thread->owner = proc;
            thread->state = PROCESS_STATE_READY;
            thread->priority = priority;
            thread->stack_base = (uint64_t)kstack;
            thread->stack_size = 64 * 1024;

            /* Prepare initial thread stack context for switching */
            uint64_t* sp = (uint64_t*)proc->kernel_stack;
            *(--sp) = 0x202;                /* RFLAGS: interrupts enabled */
            *(--sp) = 0x08;                 /* CS: kernel code */
            *(--sp) = (uint64_t)entry_point;/* RIP */
            *(--sp) = 0;                    /* Error code */
            *(--sp) = 0;                    /* Int no */
            /* General purpose registers */
            for (int r = 0; r < 15; r++) {
                *(--sp) = 0;
            }
            thread->rsp = (uint64_t)sp;
            proc->threads = thread;

            LOG_INFO("PROC", "Created process '%s' (PID: %u, Priority: %u)", name, proc->pid, priority);
            return proc;
        }
    }
    return NULL;
}

process_t* process_get_by_pid(uint32_t pid) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (g_process_table[i].pid == pid) {
            return &g_process_table[i];
        }
    }
    return NULL;
}

void process_terminate(process_t* proc, int exit_code) {
    if (!proc || proc->pid == 0) return;

    proc->state = PROCESS_STATE_TERMINATED;
    proc->exit_code = exit_code;

    LOG_INFO("PROC", "Process '%s' (PID: %u) terminated with exit code %d", proc->name, proc->pid, exit_code);
}

const process_t* process_get_table(uint32_t* out_count) {
    if (out_count) *out_count = MAX_PROCESSES;
    return g_process_table;
}

process_t* process_get_current(void) {
    return &g_process_table[0];
}

void process_destroy(process_t* proc) {
    process_terminate(proc, 0);
}

