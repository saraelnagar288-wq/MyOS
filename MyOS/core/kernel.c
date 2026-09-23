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
    for (;;) { poll_input(); __asm__ volatile ("hlt"); }
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

static void draw_char(uint32_t x, uint32_t y, int ch, uint32_t fg) {
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

static void line(uint32_t x,uint32_t y,uint32_t w,uint32_t c){fill_rect(x,y,w,1,c);}
static void frame(uint32_t x,uint32_t y,uint32_t w,uint32_t h,uint32_t c){
    line(x,y,w,c); line(x,y+h-1,w,c); fill_rect(x,y,1,h,c); fill_rect(x+w-1,y,1,h,c);
}
static void icon_grid(uint32_t x,uint32_t y,uint32_t c){
    fill_rect(x,y,7,7,c); fill_rect(x+10,y,7,7,c);
    fill_rect(x,y+10,7,7,c); fill_rect(x+10,y+10,7,7,c);
}
static void icon_folder(uint32_t x,uint32_t y){
    fill_rect(x,y+5,25,17,0xFFF4C95D); fill_rect(x+3,y+2,11,6,0xFFFFDD8A);
}
static void icon_terminal(uint32_t x,uint32_t y){
    frame(x,y,27,22,0xFFB7C3D0); fill_rect(x+5,y+9,5,2,0xFF5EEAD4);
    fill_rect(x+12,y+9,8,2,0xFF5EEAD4);
}
static void icon_settings(uint32_t x,uint32_t y){
    fill_rect(x+9,y,9,24,0xFFB8C2D1); fill_rect(x,y+8,27,9,0xFFB8C2D1);
    fill_rect(x+10,y+10,7,7,0xFF151A23);
}
static void modern_window(uint32_t x,uint32_t y,uint32_t w,uint32_t h,const char*t,uint32_t a){
    fill_rect(x+7,y+7,w,h,0x55000000); fill_rect(x,y,w,h,0xFF182130); frame(x,y,w,h,0xFF354155);
    fill_rect(x,y,w,38,0xFF202A3A); fill_rect(x,y,4,38,a);
    draw_text(x+18,y+15,t,0xFFFFFFFF);
    fill_rect(x+w-72,y+14,8,8,0xFF8794A8); fill_rect(x+w-50,y+14,8,8,0xFF8794A8);
    fill_rect(x+w-28,y+14,8,8,0xFFF87171);
}
static void draw_launcher(void){
    uint32_t w=fb_width>850?560:fb_width-80,h=fb_height>650?430:fb_height-150;
    uint32_t x=(fb_width-w)/2,y=(fb_height-h)/2;
    fill_rect(x+8,y+8,w,h,0x55000000); fill_rect(x,y,w,h,0xFF141B28); frame(x,y,w,h,0xFF40506A);
    fill_rect(x,y,w,62,0xFF1E2736); draw_text(x+24,y+22,"Search MyOS",0xFF8B98AB);
    line(x+24,y+48,w-48,0xFF344155); draw_text(x+24,y+86,"Applications",0xFF8B98AB);
    fill_rect(x+24,y+112,150,88,0xFF202A3A); icon_folder(x+48,y+132); draw_text(x+48,y+166,"Files",0xFFFFFFFF);
    fill_rect(x+192,y+112,150,88,0xFF202A3A); icon_terminal(x+214,y+132); draw_text(x+214,y+166,"Terminal",0xFFFFFFFF);
    fill_rect(x+360,y+112,150,88,0xFF202A3A); icon_settings(x+382,y+132); draw_text(x+382,y+166,"Settings",0xFFFFFFFF);
    draw_text(x+24,y+238,"System",0xFF8B98AB); draw_text(x+24,y+268,"Display",0xFFD5DCE7);
    draw_text(x+24,y+290,"Network",0xFFD5DCE7); draw_text(x+24,y+312,"Appearance",0xFFD5DCE7);
    draw_text(x+24,y+356,"Press ESC to return to desktop",0xFF6EE7B7);
}
static void draw_desktop(void) {
    fill_rect(0,0,fb_width,fb_height,0xFF0B1020);
    fill_rect(0,60,fb_width,fb_height-112,0xFF101827);
    fill_rect(fb_width/2,60,fb_width/2,fb_height-112,0xFF111D31);
    fill_rect(0,0,fb_width,60,0xFF171D29); line(0,59,fb_width,0xFF2D394C);
    fill_rect(18,16,28,28,0xFF3B82F6); icon_grid(23,20,0xFFFFFFFF);
    draw_text(58,18,"MyOS",0xFFFFFFFF); draw_text(58,34,"Native Desktop",0xFF8B98AB);
    if(fb_width>520){draw_text(fb_width-305,23,"CPU",0xFF8B98AB);draw_text(fb_width-265,23,"RAM",0xFF8B98AB);
        draw_text(fb_width-215,23,"NET",0xFF8B98AB);draw_text(fb_width-155,23,"17:42",0xFFFFFFFF);}
    icon_folder(34,92); draw_text(30,120,"Files",0xFFE8EDF5);
    icon_terminal(34,156); draw_text(25,184,"Terminal",0xFFE8EDF5);
    icon_settings(34,220); draw_text(22,248,"Settings",0xFFE8EDF5);
    uint32_t w=fb_width>900?650:(fb_width>700?570:fb_width-120),h=330,x=(fb_width-w)/2,y=100;
    modern_window(x,y,w,h,"Welcome to MyOS",0xFF3B82F6);
    draw_text(x+34,y+70,"A native x86-64 desktop.",0xFFFFFFFF);
    draw_text(x+34,y+94,"Fast, simple and built from the kernel up.",0xFFB7C3D0);
    line(x+34,y+130,w-68,0xFF344155); draw_text(x+34,y+154,"Quick access",0xFF8B98AB);
    fill_rect(x+34,y+180,150,72,0xFF202A3A); icon_folder(x+52,y+198); draw_text(x+52,y+224,"Files",0xFFFFFFFF);
    fill_rect(x+204,y+180,150,72,0xFF202A3A); icon_terminal(x+222,y+198); draw_text(x+222,y+224,"Terminal",0xFFFFFFFF);
    fill_rect(x+374,y+180,150,72,0xFF202A3A); icon_settings(x+392,y+198); draw_text(x+392,y+224,"Settings",0xFFFFFFFF);
    draw_text(x+34,y+286,"Keyboard: S = launcher   Esc = close",0xFF6EE7B7);
    uint32_t ty=fb_height-52; fill_rect(0,ty,fb_width,52,0xFF171D29); line(0,ty,fb_width,0xFF303B4E);
    fill_rect(14,ty+10,34,32,0xFF2563EB); icon_grid(22,ty+16,0xFFFFFFFF);
    fill_rect(62,ty+10,44,32,0xFF202A3A); icon_folder(72,ty+15);
    fill_rect(112,ty+10,44,32,0xFF202A3A); icon_terminal(121,ty+15);
    fill_rect(162,ty+10,44,32,0xFF202A3A); icon_settings(171,ty+15);
    if(launcher_open) draw_launcher();
}

static uint8_t keyboard_scancode(void){ if(!(inb(0x64)&1)) return 0; return inb(0x60); }
static void poll_input(void){ uint8_t sc=keyboard_scancode(); if(!sc || (sc&0x80)) return;
    if(sc==0x1F){ launcher_open=1; draw_desktop(); }
    if(sc==0x01){ launcher_open=0; draw_desktop(); }
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
