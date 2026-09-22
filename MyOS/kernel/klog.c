#include <myos/kernel.h>
#include <myos/cpu.h>
#include <myos/gfx.h>
#include <stdarg.h>

extern void serial_putc(char c);
extern void serial_puts(const char* str);

/* Minimal string and memory functions */
void* kmemset(void* dest, int val, size_t count) {
    uint8_t* p = (uint8_t*)dest;
    while (count--) {
        *p++ = (uint8_t)val;
    }
    return dest;
}

void* kmemcpy(void* dest, const void* src, size_t count) {
    uint8_t* d = (uint8_t*)dest;
    const uint8_t* s = (const uint8_t*)src;
    while (count--) {
        *d++ = *s++;
    }
    return dest;
}

void* kmemmove(void* dest, const void* src, size_t count) {
    uint8_t* d = (uint8_t*)dest;
    const uint8_t* s = (const uint8_t*)src;
    if (d < s) {
        while (count--) {
            *d++ = *s++;
        }
    } else {
        d += count;
        s += count;
        while (count--) {
            *--d = *--s;
        }
    }
    return dest;
}

int kmemcmp(const void* s1, const void* s2, size_t n) {
    const uint8_t* p1 = (const uint8_t*)s1;
    const uint8_t* p2 = (const uint8_t*)s2;
    for (size_t i = 0; i < n; i++) {
        if (p1[i] != p2[i]) {
            return p1[i] - p2[i];
        }
    }
    return 0;
}

size_t kstrlen(const char* str) {
    size_t len = 0;
    while (str && str[len]) {
        len++;
    }
    return len;
}

char* kstrcpy(char* dest, const char* src) {
    char* d = dest;
    while ((*d++ = *src++));
    return dest;
}

char* kstrncpy(char* dest, const char* src, size_t n) {
    size_t i;
    for (i = 0; i < n && src[i] != '\0'; i++) {
        dest[i] = src[i];
    }
    for (; i < n; i++) {
        dest[i] = '\0';
    }
    return dest;
}

int kstrcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

int kstrncmp(const char* s1, const char* s2, size_t n) {
    while (n && *s1 && (*s1 == *s2)) {
        s1++;
        s2++;
        n--;
    }
    if (n == 0) return 0;
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

char* kstrcat(char* dest, const char* src) {
    char* d = dest + kstrlen(dest);
    while ((*d++ = *src++));
    return dest;
}

/* VGA Text Mode fallback buffer (0xB8000) */
#define VGA_MEMORY ((volatile uint16_t*)0xB8000)
#define VGA_WIDTH  80
#define VGA_HEIGHT 25

static uint32_t g_vga_row = 0;
static uint32_t g_vga_col = 0;
static uint8_t  g_vga_color = 0x0F; /* White on Black */

static void vga_putc(char c) {
    if (c == '\n') {
        g_vga_col = 0;
        g_vga_row++;
    } else if (c == '\r') {
        g_vga_col = 0;
    } else if (c == '\b') {
        if (g_vga_col > 0) g_vga_col--;
    } else {
        VGA_MEMORY[g_vga_row * VGA_WIDTH + g_vga_col] = ((uint16_t)g_vga_color << 8) | (uint8_t)c;
        g_vga_col++;
        if (g_vga_col >= VGA_WIDTH) {
            g_vga_col = 0;
            g_vga_row++;
        }
    }

    if (g_vga_row >= VGA_HEIGHT) {
        /* Scroll screen up */
        for (int r = 0; r < VGA_HEIGHT - 1; r++) {
            for (int col = 0; col < VGA_WIDTH; col++) {
                VGA_MEMORY[r * VGA_WIDTH + col] = VGA_MEMORY[(r + 1) * VGA_WIDTH + col];
            }
        }
        for (int col = 0; col < VGA_WIDTH; col++) {
            VGA_MEMORY[(VGA_HEIGHT - 1) * VGA_WIDTH + col] = ((uint16_t)g_vga_color << 8) | ' ';
        }
        g_vga_row = VGA_HEIGHT - 1;
    }
}

void kputchar(char c) {
    serial_putc(c);
    vga_putc(c);
}

/* Number to String formatting */
static void utoa(uint64_t val, char* buf, int base, int width, char pad) {
    char temp[65];
    int i = 0;
    if (val == 0) {
        temp[i++] = '0';
    } else {
        while (val > 0) {
            int rem = val % base;
            temp[i++] = (rem < 10) ? ('0' + rem) : ('a' + rem - 10);
            val /= base;
        }
    }

    int len = i;
    int pad_count = (width > len) ? (width - len) : 0;
    int pos = 0;

    while (pad_count-- > 0) {
        buf[pos++] = pad;
    }
    while (i > 0) {
        buf[pos++] = temp[--i];
    }
    buf[pos] = '\0';
}

static void itoa(int64_t val, char* buf, int base) {
    if (val < 0 && base == 10) {
        *buf++ = '-';
        val = -val;
    }
    utoa((uint64_t)val, buf, base, 0, ' ');
}

int kvsnprintf(char* buf, size_t size, const char* fmt, va_list args) {
    size_t pos = 0;
    #define EMIT(c) do { if (buf && pos + 1 < size) buf[pos] = (c); pos++; } while (0)

    while (*fmt) {
        if (*fmt == '%') {
            fmt++;
            int width = 0;
            char pad = ' ';
            if (*fmt == '0') {
                pad = '0';
                fmt++;
            }
            while (*fmt >= '0' && *fmt <= '9') {
                width = width * 10 + (*fmt - '0');
                fmt++;
            }

            bool is_long = false;
            if (*fmt == 'l') {
                is_long = true;
                fmt++;
            }

            char num_buf[65];
            switch (*fmt) {
                case 'd':
                case 'i': {
                    int64_t v = is_long ? va_arg(args, int64_t) : va_arg(args, int32_t);
                    itoa(v, num_buf, 10);
                    for (int j = 0; num_buf[j]; j++) EMIT(num_buf[j]);
                    break;
                }
                case 'u': {
                    uint64_t v = is_long ? va_arg(args, uint64_t) : va_arg(args, uint32_t);
                    utoa(v, num_buf, 10, width, pad);
                    for (int j = 0; num_buf[j]; j++) EMIT(num_buf[j]);
                    break;
                }
                case 'x':
                case 'p': {
                    uint64_t v = is_long ? va_arg(args, uint64_t) : va_arg(args, uint32_t);
                    utoa(v, num_buf, 16, width, pad);
                    for (int j = 0; num_buf[j]; j++) EMIT(num_buf[j]);
                    break;
                }
                case 'c': {
                    char c = (char)va_arg(args, int);
                    EMIT(c);
                    break;
                }
                case 's': {
                    const char* s = va_arg(args, const char*);
                    if (!s) s = "(null)";
                    while (*s) EMIT(*s++);
                    break;
                }
                case '%': {
                    EMIT('%');
                    break;
                }
                default:
                    EMIT('%');
                    EMIT(*fmt);
                    break;
            }
        } else {
            EMIT(*fmt);
        }
        fmt++;
    }

    if (buf && size > 0) {
        if (pos < size) buf[pos] = '\0';
        else buf[size - 1] = '\0';
    }
    return (int)pos;
}

void kprintf(const char* fmt, ...) {
    char buf[1024];
    va_list args;
    va_start(args, fmt);
    kvsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    for (int i = 0; buf[i]; i++) {
        kputchar(buf[i]);
    }
}

void ksprintf(char* buf, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    kvsnprintf(buf, 2048, fmt, args);
    va_end(args);
}

void klog(log_level_t level, const char* component, const char* fmt, ...) {
    char msg[1024];
    va_list args;
    va_start(args, fmt);
    kvsnprintf(msg, sizeof(msg), fmt, args);
    va_end(args);

    const char* level_str = "INFO";
    switch (level) {
        case LOG_LEVEL_DEBUG:   level_str = "DEBUG"; break;
        case LOG_LEVEL_INFO:    level_str = "INFO "; break;
        case LOG_LEVEL_WARNING: level_str = "WARN "; break;
        case LOG_LEVEL_ERROR:   level_str = "ERROR"; break;
    }

    kprintf("[%s][%s] %s\n", level_str, component, msg);
}

void kernel_panic(const char* message, const char* file, int line) {
    cli();
    kprintf("\n\n");
    kprintf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
    kprintf("                 KERNEL PANIC - MyOS                   \n");
    kprintf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
    kprintf("Fatal: %s\n", message);
    kprintf("Location: %s:%d\n", file, line);
    kprintf("System halted.\n");

    while (1) {
        hlt();
    }
}
