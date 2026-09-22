#include <myos/syscall.h>
#include <myos/idt.h>
#include <myos/process.h>
#include <myos/scheduler.h>
#include <myos/drivers.h>
#include <myos/fs.h>
#include <myos/hal.h>
#include <myos/kernel.h>

static void syscall_interrupt_handler(struct interrupt_registers* regs) {
    uint64_t syscall_num = regs->rax;
    uint64_t arg1 = regs->rdi;
    uint64_t arg2 = regs->rsi;
    uint64_t arg3 = regs->rdx;

    int64_t ret = 0;
    process_t* curr = process_get_current();

    switch (syscall_num) {
        case SYS_YIELD:
            scheduler_yield();
            ret = 0;
            break;

        case SYS_EXIT:
            if (curr) process_destroy(curr);
            scheduler_yield();
            ret = 0;
            break;

        case SYS_WRITE: {
            int fd = (int)arg1;
            const char* buf = (const char*)arg2;
            size_t count = (size_t)arg3;

            if (fd == 1 || fd == 2) {
                for (size_t i = 0; i < count; i++) {
                    serial_putc(buf[i]);
                }
                ret = (int64_t)count;
            } else {
                ret = -1;
            }
            break;
        }

        case SYS_READ: {
            int fd = (int)arg1;
            (void)arg2;
            (void)arg3;
            if (fd == 0) {
                ret = 0;
            } else {
                ret = -1;
            }
            break;
        }

        case SYS_GETPID:
            ret = curr ? curr->pid : 0;
            break;

        case SYS_SLEEP:
            hal_sleep_ms((uint32_t)arg1);
            ret = 0;
            break;

        case SYS_UPTIME:
            ret = (int64_t)hal_get_uptime_ms();
            break;

        case SYS_GET_TIME: {
            rtc_time_t t;
            hal_get_time(&t);
            if (arg1) {
                kmemcpy((void*)arg1, &t, sizeof(rtc_time_t));
            }
            ret = 0;
            break;
        }

        case SYS_KILL: {
            process_t* target = process_get_by_pid((uint32_t)arg1);
            if (target) {
                process_destroy(target);
                ret = 0;
            } else {
                ret = -1;
            }
            break;
        }

        case SYS_REBOOT:
            hal_reboot();
            break;

        case SYS_SHUTDOWN:
            hal_shutdown();
            break;

        default:
            LOG_WARNING("SYSCALL", "Unknown syscall number: %llu", syscall_num);
            ret = -1;
            break;
    }

    regs->rax = (uint64_t)ret;
}

void syscall_init(void) {
    register_interrupt_handler(0x80, syscall_interrupt_handler);
    LOG_INFO("SYSCALL", "System Call interface registered on interrupt 0x80");
}
