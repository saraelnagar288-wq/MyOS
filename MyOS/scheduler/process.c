#include <myos/process.h>
#include <myos/scheduler.h>
#include <myos/kheap.h>
#include <myos/kernel.h>
#include <myos/gdt.h>

static process_t s_processes[MAX_PROCESSES];
static uint32_t s_next_pid = 1;
static uint32_t s_next_tid = 1;
static process_t* s_current_process = NULL;

void process_init(void) {
    kmemset(s_processes, 0, sizeof(s_processes));

    /* Create Kernel Idle / Root Process (PID 0) */
    process_t* root = &s_processes[0];
    root->pid = 0;
    root->parent_pid = 0;
    kstrcpy(root->name, "kernel_idle");
    root->state = PROCESS_STATE_RUNNING;
    root->page_directory = vmm_get_kernel_directory();

    thread_t* root_thread = (thread_t*)kzalloc(sizeof(thread_t));
    root_thread->tid = s_next_tid++;
    root_thread->pid = 0;
    root_thread->state = PROCESS_STATE_RUNNING;
    root_thread->time_slice = 10;
    root->main_thread = root_thread;
    root->thread_count = 1;

    s_current_process = root;
    scheduler_add_thread(root_thread);

    LOG_INFO("PROC", "Process subsystem initialized with Root Process (PID 0)");
}

process_t* process_create(const char* name, void (*entry_point)(void), bool is_user) {
    process_t* proc = NULL;
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (s_processes[i].state == PROCESS_STATE_UNUSED) {
            proc = &s_processes[i];
            break;
        }
    }
    if (!proc) {
        LOG_ERROR("PROC", "Maximum process limit reached");
        return NULL;
    }

    kmemset(proc, 0, sizeof(process_t));
    proc->pid = s_next_pid++;
    proc->parent_pid = s_current_process ? s_current_process->pid : 0;
    kstrncpy(proc->name, name, 31);
    proc->state = PROCESS_STATE_READY;

    if (is_user) {
        proc->page_directory = vmm_create_address_space();
    } else {
        proc->page_directory = vmm_get_kernel_directory();
    }

    /* Allocate Thread and Stacks */
    thread_t* th = (thread_t*)kzalloc(sizeof(thread_t));
    th->tid = s_next_tid++;
    th->pid = proc->pid;
    th->state = PROCESS_STATE_READY;
    th->time_slice = 10; /* 10 ms default slice */

    void* kstack = kmalloc_aligned(KERNEL_STACK_SIZE, 16);
    th->kernel_stack = (uint64_t)kstack + KERNEL_STACK_SIZE;

    /* Setup initial execution context */
    th->context.rsp = th->kernel_stack - 16;
    th->context.rip = (uint64_t)entry_point;
    th->context.rbp = th->context.rsp;

    proc->main_thread = th;
    proc->thread_count = 1;

    scheduler_add_thread(th);
    LOG_INFO("PROC", "Created process '%s' (PID %u, TID %u)", proc->name, proc->pid, th->tid);
    return proc;
}

void process_destroy(process_t* proc) {
    if (!proc || proc->pid == 0) return;
    proc->state = PROCESS_STATE_ZOMBIE;
    if (proc->main_thread) {
        scheduler_remove_thread(proc->main_thread);
        proc->main_thread->state = PROCESS_STATE_ZOMBIE;
    }
    LOG_INFO("PROC", "Terminated process '%s' (PID %u)", proc->name, proc->pid);
}

process_t* process_get_current(void) {
    return s_current_process;
}

process_t* process_get_by_pid(uint32_t pid) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (s_processes[i].state != PROCESS_STATE_UNUSED && s_processes[i].pid == pid) {
            return &s_processes[i];
        }
    }
    return NULL;
}

uint32_t process_get_count(void) {
    uint32_t count = 0;
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (s_processes[i].state != PROCESS_STATE_UNUSED) count++;
    }
    return count;
}

const process_t* process_get_at(uint32_t index) {
    if (index >= MAX_PROCESSES) return NULL;
    return &s_processes[index];
}
