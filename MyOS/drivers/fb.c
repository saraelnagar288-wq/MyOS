#include <myos/gfx.h>
#include <myos/kheap.h>
#include <myos/kernel.h>

extern const uint8_t font8x16_data[128][16];

static framebuffer_t g_fb;
static bool g_fb_initialized = false;

/* Fallback video memory if multiboot did not provide graphics */
static uint32_t g_fallback_buffer[1024 * 768];

void gfx_init(struct multiboot_info* mb_info) {
    kmemset(&g_fb, 0, sizeof(framebuffer_t));

    if (mb_info && (mb_info->flags & (1 << 12)) && mb_info->framebuffer_addr != 0) {
        g_fb.buffer = (uint32_t*)(uint64_t)mb_info->framebuffer_addr;
        g_fb.width  = mb_info->framebuffer_width;
        g_fb.height = mb_info->framebuffer_height;
        g_fb.pitch  = mb_info->framebuffer_pitch / 4;
        g_fb.bpp    = mb_info->framebuffer_bpp;
    } else {
        /* Default to 1024x768 32bpp software buffer */
        g_fb.buffer = g_fallback_buffer;
        g_fb.width  = 1024;
        g_fb.height = 768;
        g_fb.pitch  = 1024;
        g_fb.bpp    = 32;
    }

    /* Allocate back buffer for double buffering */
    size_t buffer_size = g_fb.width * g_fb.height * sizeof(uint32_t);
    g_fb.back_buffer = (uint32_t*)kmalloc(buffer_size);
    if (!g_fb.back_buffer) {
        /* If kmalloc failed, point back buffer to front buffer */
        g_fb.back_buffer = g_fb.buffer;
    } else {
        kmemset(g_fb.back_buffer, 0, buffer_size);
    }

    g_fb_initialized = true;
    LOG_INFO("GFX", "Framebuffer initialized: %ux%u (%ubpp) at 0x%016lx",
             g_fb.width, g_fb.height, g_fb.bpp, (uint64_t)g_fb.buffer);
}

framebuffer_t* gfx_get_framebuffer(void) {
    return &g_fb;
}

void gfx_swap_buffers(void) {
    if (!g_fb_initialized || g_fb.back_buffer == g_fb.buffer) return;
    kmemcpy(g_fb.buffer, g_fb.back_buffer, g_fb.width * g_fb.height * sizeof(uint32_t));
}

void gfx_putpixel(uint32_t x, uint32_t y, uint32_t color) {
    if (!g_fb_initialized || x >= g_fb.width || y >= g_fb.height) return;
    g_fb.back_buffer[y * g_fb.pitch + x] = color;
}

void gfx_clear(uint32_t color) {
    if (!g_fb_initialized) return;
    uint32_t total = g_fb.width * g_fb.height;
    for (uint32_t i = 0; i < total; i++) {
        g_fb.back_buffer[i] = color;
    }
}

void gfx_fill_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color) {
    if (!g_fb_initialized) return;

    if (x >= g_fb.width || y >= g_fb.height) return;
    if (x + w > g_fb.width)  w = g_fb.width - x;
    if (y + h > g_fb.height) h = g_fb.height - y;

    for (uint32_t cy = y; cy < y + h; cy++) {
        uint32_t* line = &g_fb.back_buffer[cy * g_fb.pitch + x];
        for (uint32_t cx = 0; cx < w; cx++) {
            line[cx] = color;
        }
    }
}

void gfx_draw_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color) {
    if (!g_fb_initialized || w == 0 || h == 0) return;
    gfx_fill_rect(x, y, w, 1, color);
    gfx_fill_rect(x, y + h - 1, w, 1, color);
    gfx_fill_rect(x, y, 1, h, color);
    gfx_fill_rect(x + w - 1, y, 1, h, color);
}

void gfx_draw_line(int x0, int y0, int x1, int y1, uint32_t color) {
    int dx = (x1 > x0) ? (x1 - x0) : (x0 - x1);
    int dy = (y1 > y0) ? (y1 - y0) : (y0 - y1);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;

    while (1) {
        if (x0 >= 0 && (uint32_t)x0 < g_fb.width && y0 >= 0 && (uint32_t)y0 < g_fb.height) {
            gfx_putpixel((uint32_t)x0, (uint32_t)y0, color);
        }
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y0 += sy;
        }
    }
}

void gfx_draw_char(uint32_t x, uint32_t y, char c, uint32_t fg, uint32_t bg) {
    if (!g_fb_initialized || (uint8_t)c > 127) return;

    const uint8_t* glyph = font8x16_data[(uint8_t)c];
    for (int row = 0; row < 16; row++) {
        uint8_t bits = glyph[row];
        for (int col = 0; col < 8; col++) {
            if (bits & (0x80 >> col)) {
                gfx_putpixel(x + col, y + row, fg);
            } else if (bg != 0) {
                gfx_putpixel(x + col, y + row, bg);
            }
        }
    }
}

void gfx_draw_string(uint32_t x, uint32_t y, const char* str, uint32_t fg, uint32_t bg) {
    if (!str) return;
    uint32_t cur_x = x;
    uint32_t cur_y = y;

    while (*str) {
        if (*str == '\n') {
            cur_x = x;
            cur_y += 16;
        } else if (*str == '\t') {
            cur_x += 8 * 4;
        } else {
            gfx_draw_char(cur_x, cur_y, *str, fg, bg);
            cur_x += 8;
        }
        str++;
    }
}

void gfx_draw_bitmap(uint32_t x, uint32_t y, uint32_t w, uint32_t h, const uint32_t* bitmap, bool has_alpha) {
    if (!g_fb_initialized || !bitmap) return;

    for (uint32_t cy = 0; cy < h; cy++) {
        uint32_t py = y + cy;
        if (py >= g_fb.height) break;

        for (uint32_t cx = 0; cx < w; cx++) {
            uint32_t px = x + cx;
            if (px >= g_fb.width) break;

            uint32_t pixel = bitmap[cy * w + cx];
            if (has_alpha) {
                uint8_t alpha = (pixel >> 24) & 0xFF;
                if (alpha == 0) continue;
                if (alpha == 255) {
                    gfx_putpixel(px, py, pixel & 0xFFFFFF);
                } else {
                    /* Alpha blending */
                    uint32_t bg = g_fb.back_buffer[py * g_fb.pitch + px];
                    uint8_t r = (((pixel >> 16) & 0xFF) * alpha + ((bg >> 16) & 0xFF) * (255 - alpha)) / 255;
                    uint8_t g = (((pixel >> 8)  & 0xFF) * alpha + ((bg >> 8)  & 0xFF) * (255 - alpha)) / 255;
                    uint8_t b = ((pixel & 0xFF)         * alpha + (bg & 0xFF)         * (255 - alpha)) / 255;
                    gfx_putpixel(px, py, COLOR_RGB(r, g, b));
                }
            } else {
                gfx_putpixel(px, py, pixel);
            }
        }
    }
}
