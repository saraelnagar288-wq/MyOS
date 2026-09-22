#ifndef MYOS_KERNEL_H
#define MYOS_KERNEL_H

#include <myos/types.h>

#define MYOS_NAME        "MyOS"
#define MYOS_VERSION     "0.1.0"
#define MYOS_ARCH        "x86_64"
#define MYOS_BUILD_DATE  __DATE__ " " __TIME__

/* Multiboot 1 Structures */
#define MULTIBOOT_BOOTLOADER_MAGIC 0x2BADB002

struct multiboot_mmap_entry {
    uint32_t size;
    uint64_t addr;
    uint64_t len;
    uint32_t type;
} __attribute__((packed));

struct multiboot_info {
    uint32_t flags;
    uint32_t mem_lower;
    uint32_t mem_upper;
    uint32_t boot_device;
    uint32_t cmdline;
    uint32_t mods_count;
    uint32_t mods_addr;
    uint32_t syms[4];
    uint32_t mmap_length;
    uint32_t mmap_addr;
    uint32_t drives_length;
    uint32_t drives_addr;
    uint32_t config_table;
    uint32_t boot_loader_name;
    uint32_t apm_table;
    uint32_t vbe_control_info;
    uint32_t vbe_mode_info;
    uint16_t vbe_mode;
    uint16_t vbe_interface_seg;
    uint16_t vbe_interface_off;
    uint16_t vbe_interface_len;
    uint64_t framebuffer_addr;
    uint32_t framebuffer_pitch;
    uint32_t framebuffer_width;
    uint32_t framebuffer_height;
    uint8_t  framebuffer_bpp;
    uint8_t  framebuffer_type;
    uint8_t  color_info[6];
} __attribute__((packed));

/* Logging Levels */
typedef enum {
    LOG_LEVEL_DEBUG = 0,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARNING,
    LOG_LEVEL_ERROR
} log_level_t;

/* Serial & Console Logging */
void klog(log_level_t level, const char* component, const char* fmt, ...);
void kprintf(const char* fmt, ...);
void ksprintf(char* buf, const char* fmt, ...);
int ksnprintf(char* str, size_t size, const char* format, ...);

#define LOG_DEBUG(comp, fmt, ...)   klog(LOG_LEVEL_DEBUG, comp, fmt, ##__VA_ARGS__)
#define LOG_INFO(comp, fmt, ...)    klog(LOG_LEVEL_INFO, comp, fmt, ##__VA_ARGS__)
#define LOG_WARNING(comp, fmt, ...) klog(LOG_LEVEL_WARNING, comp, fmt, ##__VA_ARGS__)
#define LOG_WARN(comp, fmt, ...)    klog(LOG_LEVEL_WARNING, comp, fmt, ##__VA_ARGS__)
#define LOG_ERROR(comp, fmt, ...)   klog(LOG_LEVEL_ERROR, comp, fmt, ##__VA_ARGS__)

void kputchar(char c);

/* Panic */
void kernel_panic(const char* message, const char* file, int line);
#define PANIC(msg) kernel_panic(msg, __FILE__, __LINE__)

/* Memory & String Libs */
void* kmemset(void* dest, int val, size_t count);
void* kmemcpy(void* dest, const void* src, size_t count);
void* kmemmove(void* dest, const void* src, size_t count);
int kmemcmp(const void* s1, const void* s2, size_t n);
size_t kstrlen(const char* str);
char* kstrcpy(char* dest, const char* src);
char* kstrncpy(char* dest, const char* src, size_t n);
int kstrcmp(const char* s1, const char* s2);
int kstrncmp(const char* s1, const char* s2, size_t n);
char* kstrcat(char* dest, const char* src);

#endif /* MYOS_KERNEL_H */
