#include <stdint.h>
#include <stddef.h>

#define MULTIBOOT_MAGIC 0x2BADB002u
#define FB_FLAG (1u << 12)

struct multiboot_info {
    uint32_t flags;
    uint32_t mem_lower, mem_upper, boot_device, cmdline, mods_count, mods_addr;
    uint32_t syms[4];
    uint32_t mmap_length, mmap_addr;
    uint32_t drives_length, drives_addr;
    uint32_t config_table, boot_loader_name, apm_table;
    uint32_t vbe_control_info, vbe_mode_info;
    uint16_t vbe_mode, vbe_interface_seg, vbe_interface_off, vbe_interface_len;
    uint64_t framebuffer_addr;
    uint32_t framebuffer_pitch, framebuffer_width, framebuffer_height;
    uint8_t framebuffer_bpp, framebuffer_type;
    uint8_t color_info[6];
} __attribute__((packed));

static volatile uint8_t *serial = (volatile uint8_t *)0x3F8;

static void outb(uint16_t port, uint8_t value) {
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

static void serial_init(void) {
    outb(0x3F9, 0);
    outb(0x3FB, 0x80);
    outb(0x3F8, 3);
    outb(0x3F9, 0);
    outb(0x3FB, 3);
    outb(0x3FC, 3);
    outb(0x3FA, 0xC7);
}

static void serial_putc(char c) {
    while (!(serial[5] & 0x20)) {}
    serial[0] = (uint8_t)c;
}

static void serial_puts(const char *s) {
    while (*s) serial_putc(*s++);
}

static void halt(void) {
    for (;;) __asm__ volatile ("hlt");
}

static uint32_t *fb;
static uint32_t fb_pitch;
static uint32_t fb_width;
static uint32_t fb_height;

static void put_pixel(uint32_t x, uint32_t y, uint32_t c) {
    if (!fb || x >= fb_width || y >= fb_height) return;
    fb[y * (fb_pitch / 4) + x] = c;
}

static void fill_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t c) {
    if (!fb) return;
    if (x >= fb_width || y >= fb_height) return;
    if (x + w > fb_width) w = fb_width - x;
    if (y + h > fb_height) h = fb_height - y;

    for (uint32_t yy = y; yy < y + h; ++yy)
        for (uint32_t xx = x; xx < x + w; ++xx)
            put_pixel(xx, yy, c);
}

static const uint8_t font8x8[96][8] = {
#include "font8x8.inc"
};

static void draw_char(uint32_t x, uint32_t y, char ch, uint32_t fg) {
    if (ch < 32 || ch > 127) ch = '?';
    const uint8_t *g = font8x8[(uint8_t)ch - 32];
    for (uint32_t row = 0; row < 8; ++row)
        for (uint32_t col = 0; col < 8; ++col)
            if (g[row] & (1u << (7 - col))) put_pixel(x + col, y + row, fg);
}

static void draw_text(uint32_t x, uint32_t y, const char *s, uint32_t fg) {
    while (*s) {
        if (*s == '\n') { y += 10; x = 32; }
        else { draw_char(x, y, *s, fg); x += 8; }
        ++s;
    }
}

static void draw_desktop(void) {
    fill_rect(0, 0, fb_width, fb_height, 0xFF111318);
    fill_rect(0, 0, fb_width, 42, 0xFF1C2028);
    fill_rect(0, fb_height - 52, fb_width, 52, 0xFF1C2028);

    fill_rect(22, fb_height - 40, 30, 30, 0xFF2563EB);
    fill_rect(64, fb_height - 40, 30, 30, 0xFF2B303A);
    fill_rect(106, fb_height - 40, 30, 30, 0xFF2B303A);

    fill_rect(30, 76, 560, 320, 0xFF1C2028);
    fill_rect(30, 76, 560, 34, 0xFF2563EB);
    fill_rect(42, 88, 8, 8, 0xFFFFFFFF);
    fill_rect(58, 88, 8, 8, 0xFFFFD166);
    fill_rect(74, 88, 8, 8, 0xFFFF5C5C);

    draw_text(46, 136, "MyOS", 0xFFFFFFFF);
    draw_text(46, 158, "Native x86-64 kernel", 0xFF9CA3AF);
    draw_text(46, 180, "Bootloader -> kernel -> framebuffer", 0xFF9CA3AF);
    draw_text(46, 214, "Foundation build is active.", 0xFF06B6D4);
    draw_text(46, 236, "Next: memory, interrupts, PCI, storage,", 0xFFD1D5DB);
    draw_text(46, 248, "processes, drivers and native applications.", 0xFFD1D5DB);

    draw_text(32, 20, "MyOS Desktop", 0xFFFFFFFF);
    draw_text(fb_width > 180 ? fb_width - 150 : 10, 20, "x86-64", 0xFF9CA3AF);
    draw_text(34, fb_height - 22, "MyOS", 0xFFFFFFFF);
}

void kernel_main(uint64_t magic, struct multiboot_info *mb_info) {
    serial_init();
    serial_puts("MyOS: entering kernel\r\n");

    if ((uint32_t)magic != MULTIBOOT_MAGIC) {
        serial_puts("MyOS: invalid Multiboot magic\r\n");
        halt();
    }

    if (!mb_info || !(mb_info->flags & FB_FLAG) || !mb_info->framebuffer_addr) {
        serial_puts("MyOS: no Multiboot framebuffer supplied\r\n");
        halt();
    }

    fb = (uint32_t *)(uintptr_t)mb_info->framebuffer_addr;
    fb_pitch = mb_info->framebuffer_pitch;
    fb_width = mb_info->framebuffer_width;
    fb_height = mb_info->framebuffer_height;

    serial_puts("MyOS: framebuffer initialized\r\n");
    draw_desktop();
    serial_puts("MyOS: desktop rendered successfully\r\n");

    halt();
}
