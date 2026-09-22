#ifndef MYOS_GFX_H
#define MYOS_GFX_H

#include <myos/types.h>

/* 32-bit ARGB / XRGB Color representation */
typedef uint32_t color_t;

#define COLOR_RGBA(r, g, b, a) (((color_t)(a) << 24) | ((color_t)(r) << 16) | ((color_t)(g) << 8) | (color_t)(b))
#define COLOR_RGB(r, g, b)      COLOR_RGBA(r, g, b, 255)

/* Palette constants */
#define COLOR_BLACK       0xFF000000
#define COLOR_WHITE       0xFFFFFFFF
#define COLOR_DARK_GRAY   0xFF1E1E24
#define COLOR_MID_GRAY    0xFF2B2B36
#define COLOR_LIGHT_GRAY  0xFF444454
#define COLOR_ACCENT_BLUE 0xFF2563EB
#define COLOR_CYAN        0xFF06B6D4
#define COLOR_GREEN       0xFF10B981
#define COLOR_RED         0xFFEF4444
#define COLOR_YELLOW      0xFFF59E0B
#define COLOR_PURPLE      0xFF8B5CF6
#define COLOR_TRANSPARENT 0x00000000

typedef struct {
    uint8_t* addr;
    uint32_t width;
    uint32_t height;
    uint32_t pitch;
    uint8_t  bpp;
} framebuffer_t;

struct multiboot_info;
void gfx_init(struct multiboot_info* mb_info);
const framebuffer_t* gfx_get_fb(void);
uint32_t* gfx_get_backbuffer(void);
void gfx_swap_buffers(void);

/* Drawing primitives */
void gfx_put_pixel(int32_t x, int32_t y, color_t color);
color_t gfx_get_pixel(int32_t x, int32_t y);
void gfx_clear(color_t color);
void gfx_fill_rect(int32_t x, int32_t y, uint32_t w, uint32_t h, color_t color);
void gfx_draw_rect(int32_t x, int32_t y, uint32_t w, uint32_t h, color_t color);
void gfx_draw_line(int32_t x0, int32_t y0, int32_t x1, int32_t y1, color_t color);
void gfx_draw_char(int32_t x, int32_t y, char c, color_t fg, color_t bg);
void gfx_draw_string(int32_t x, int32_t y, const char* str, color_t fg, color_t bg);
void gfx_draw_cursor(int32_t x, int32_t y);

#define COLOR_NAVY  0xFF1A365D
#define COLOR_GRAY  0xFF718096

#endif /* MYOS_GFX_H */
