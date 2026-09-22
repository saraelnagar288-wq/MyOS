#include <myos/kernel.h>
#include <myos/drivers.h>
#include <myos/cpu.h>

typedef __builtin_va_list va_list;
#define va_start(v,l) __builtin_va_start(v,l)
#define va_end(v)     __builtin_va_end(v)
#define va_arg(v,l)   __builtin_va_arg(v,l)

static char s_log_buffer[16384];
static size_t s_log_head = 0;

static void log_char(char c) {
    serial_write_char(c);
    s_log_buffer[s_log_head] = c;
    s_log_head = (s_log_head + 1) % sizeof(s_log_buffer);
}

static void log_string(const char* s) {
    if (!s) s = "(null)";
    while (*s) {
        log_char(*s++);
    }
}

static void print_num(uint64_t n, int base, bool uppercase, int min_width, char pad) {
    char buf[65];
    int i = 0;
    const char* digits = uppercase ? "0123456789ABCDEF" : "0123456789abcdef";

    if (n == 0) {
        buf[i++] = '0';
    } else {
        while (n > 0) {
            buf[i++] = digits[n % base];
            n /= base;
        }
    }

    while (i < min_width) {
        buf[i++] = pad;
    }

    while (i > 0) {
        log_char(buf[--i]);
    }
}

void kvprintf(const char* fmt, va_list args) {
    while (*fmt) {
        if (*fmt == '%') {
            fmt++;
            int min_width = 0;
            char pad = ' ';
            if (*fmt == '0') {
                pad = '0';
                fmt++;
            }
            while (*fmt >= '0' && *fmt <= '9') {
                min_width = min_width * 10 + (*fmt - '0');
                fmt++;
            }

            bool is_longlong = false;
            if (*fmt == 'l' && *(fmt + 1) == 'l') {
                is_longlong = true;
                fmt += 2;
            } else if (*fmt == 'l') {
                is_longlong = true;
                fmt++;
            }

            switch (*fmt) {
                case 'c': {
                    char c = (char)va_arg(args, int);
                    log_char(c);
                    break;
                }
                case 's': {
                    const char* s = va_arg(args, const char*);
                    log_string(s);
                    break;
                }
                case 'd':
                case 'i': {
                    int64_t val = is_longlong ? va_arg(args, int64_t) : va_arg(args, int);
                    if (val < 0) {
                        log_char('-');
                        val = -val;
                    }
                    print_num((uint64_t)val, 10, false, min_width, pad);
                    break;
                }
                case 'u': {
                    uint64_t val = is_longlong ? va_arg(args, uint64_t) : va_arg(args, unsigned int);
                    print_num(val, 10, false, min_width, pad);
                    break;
                }
                case 'x': {
                    uint64_t val = is_longlong ? va_arg(args, uint64_t) : va_arg(args, unsigned int);
                    print_num(val, 16, false, min_width, pad);
                    break;
                }
                case 'X': {
                    uint64_t val = is_longlong ? va_arg(args, uint64_t) : va_arg(args, unsigned int);
                    print_num(val, 16, true, min_width, pad);
                    break;
                }
                case 'p': {
                    uint64_t val = (uint64_t)va_arg(args, void*);
                    log_string("0x");
                    print_num(val, 16, false, 16, '0');
                    break;
                }
                case '%':
                    log_char('%');
                    break;
                default:
                    log_char('%');
                    log_char(*fmt);
                    break;
            }
        } else {
            log_char(*fmt);
        }
        fmt++;
    }
}

void kprintf(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    kvprintf(fmt, args);
    va_end(args);
}

int ksnprintf(char* str, size_t size, const char* format, ...) {
    if (!str || size == 0) return 0;

    va_list args;
    va_start(args, format);

    size_t out_idx = 0;
    const char* fmt = format;

    #define APPEND_CHAR(c) do { if (out_idx + 1 < size) str[out_idx++] = (c); } while(0)

    while (*fmt && out_idx + 1 < size) {
        if (*fmt == '%') {
            fmt++;
            int min_width = 0;
            char pad = ' ';
            if (*fmt == '0') {
                pad = '0';
                fmt++;
            }
            while (*fmt >= '0' && *fmt <= '9') {
                min_width = min_width * 10 + (*fmt - '0');
                fmt++;
            }

            bool is_longlong = false;
            if (*fmt == 'l' && *(fmt + 1) == 'l') {
                is_longlong = true;
                fmt += 2;
            } else if (*fmt == 'l') {
                is_longlong = true;
                fmt++;
            }

            switch (*fmt) {
                case 'c': {
                    char c = (char)va_arg(args, int);
                    APPEND_CHAR(c);
                    break;
                }
                case 's': {
                    const char* s = va_arg(args, const char*);
                    if (!s) s = "(null)";
                    while (*s && out_idx + 1 < size) {
                        APPEND_CHAR(*s++);
                    }
                    break;
                }
                case 'd':
                case 'i': {
                    int64_t val = is_longlong ? va_arg(args, int64_t) : va_arg(args, int);
                    if (val < 0) {
                        APPEND_CHAR('-');
                        val = -val;
                    }
                    char num_buf[32];
                    int ni = 0;
                    if (val == 0) num_buf[ni++] = '0';
                    else {
                        while (val > 0) {
                            num_buf[ni++] = '0' + (val % 10);
                            val /= 10;
                        }
                    }
                    while (ni < min_width) num_buf[ni++] = pad;
                    while (ni > 0) APPEND_CHAR(num_buf[--ni]);
                    break;
                }
                case 'u': {
                    uint64_t val = is_longlong ? va_arg(args, uint64_t) : va_arg(args, unsigned int);
                    char num_buf[32];
                    int ni = 0;
                    if (val == 0) num_buf[ni++] = '0';
                    else {
                        while (val > 0) {
                            num_buf[ni++] = '0' + (val % 10);
                            val /= 10;
                        }
                    }
                    while (ni < min_width) num_buf[ni++] = pad;
                    while (ni > 0) APPEND_CHAR(num_buf[--ni]);
                    break;
                }
                case 'x': {
                    uint64_t val = is_longlong ? va_arg(args, uint64_t) : va_arg(args, unsigned int);
                    char num_buf[32];
                    int ni = 0;
                    const char* hex = "0123456789abcdef";
                    if (val == 0) num_buf[ni++] = '0';
                    else {
                        while (val > 0) {
                            num_buf[ni++] = hex[val % 16];
                            val /= 16;
                        }
                    }
                    while (ni < min_width) num_buf[ni++] = pad;
                    while (ni > 0) APPEND_CHAR(num_buf[--ni]);
                    break;
                }
                default:
                    APPEND_CHAR(*fmt);
                    break;
            }
        } else {
            APPEND_CHAR(*fmt);
        }
        fmt++;
    }

    str[out_idx] = '\0';
    va_end(args);
    return (int)out_idx;
}

static const char* s_level_names[] = {
    "DEBUG",
    "INFO",
    "WARN",
    "ERROR"
};

void klog(log_level_t level, const char* component, const char* fmt, ...) {
    const char* lvl = (level <= LOG_LEVEL_ERROR) ? s_level_names[level] : "INFO";
    kprintf("[%s] (%s): ", lvl, component ? component : "KERNEL");
    va_list args;
    va_start(args, fmt);
    kvprintf(fmt, args);
    va_end(args);
    kprintf("\n");
}

void kernel_panic(const char* message, const char* file, int line) {
    cli();
    kprintf("\n=======================================================\n");
    kprintf("                *** KERNEL PANIC ***\n");
    kprintf("=======================================================\n");
    kprintf("Panic Message: %s\n", message ? message : "Unknown failure");
    kprintf("Location:      %s:%d\n", file ? file : "unknown", line);
    kprintf("System halted. Please reboot the computer.\n");
    kprintf("=======================================================\n");
    while (1) {
        hlt();
    }
}
