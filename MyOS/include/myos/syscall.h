#ifndef MYOS_SYSCALL_H
#define MYOS_SYSCALL_H

#include <myos/types.h>

/* System Call Numbers */
#define SYS_YIELD         0
#define SYS_EXIT          1
#define SYS_SLEEP         2
#define SYS_GETPID        3
#define SYS_WRITE         4
#define SYS_READ          5
#define SYS_OPEN          6
#define SYS_CLOSE         7
#define SYS_MMAP          8
#define SYS_MUNMAP        9
#define SYS_SPAWN         10
#define SYS_KILL          11
#define SYS_UPTIME        12
#define SYS_GET_TIME      13
#define SYS_DRAW_RECT     14
#define SYS_DRAW_STRING   15
#define SYS_BLIT          16
#define SYS_POLL_EVENT    17
#define SYS_CREATE_WINDOW 18
#define SYS_DESTROY_WINDOW 19
#define SYS_REBOOT        20
#define SYS_SHUTDOWN      21

void syscall_init(void);
int64_t syscall_handler(uint64_t num, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5);

#endif /* MYOS_SYSCALL_H */
