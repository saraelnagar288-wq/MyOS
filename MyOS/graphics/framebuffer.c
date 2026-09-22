#include <myos/gfx.h>
#include <myos/kheap.h>
#include <myos/kernel.h>

extern const uint8_t g_font_8x16[128][16];

static framebuffer_t s_fb;
static uint32_t* s_backbuffer = NULL;
static size_t s_buffer_size = 0;

void gfx_init(struct multiboot_info* mb_info) {
    kmemset(&s_fb, 0, sizeof(s_fb));

    /* Check for Multiboot linear framebuffer */
    if (mb_info->flags & (1 << 12)) {
        s_fb.addr   = (uint8_t*)(uint64_t)mb_info->framebuffer_addr;
        s_fb.width  = mb_info->framebuffer_width;
        s_fb.height = mb_info->framebuffer_height;
        s_fb.pitch  = mb_info->framebuffer_pitch;
        s_fb.bpp    = mb_info->framebuffer_bpp;
    } else {
        /* Fallback: standard 1024x768x32 assumption */
        s_fb.addr   = (uint8_t*)0xFD000000ULL; /* Typical QEMU Bochs VBE LFB */
        s_fb.width  = 1024;
        s_fb.height = 768;
        s_fb.pitch  = 1024 * 4;
        s_fb.bpp    = 32;
    }

    s_buffer_size = (size_t)s_fb.pitch * s_fb.height;
    s_backbuffer = (uint32_t*)kmalloc_aligned(s_buffer_size, 64);
    if (!s_backbuffer) {
        LOG_WARNING("GFX", "Could not allocate backbuffer, using direct VRAM");
    }

    LOG_INFO("GFX", "Graphics Framebuffer initialized: %ux%u @ %u bpp (VRAM=0x%llx)",
             s_fb.width, s_fb.height, s_fb.bpp, (uint64_t)s_fb.addr);
}

const framebuffer_t* gfx_get_fb(void) {
    return &s_fb;
}

uint32_t* gfx_get_backbuffer(void) {
    return s_backbuffer ? s_backbuffer : (uint32_t*)s_fb.addr;
}

void gfx_clear(color_t color) {
    uint32_t* buf = gfx_get_backbuffer();
    uint32_t total_pixels = (s_fb.pitch / 4) * s_fb.height;
    for (uint32_t i = 0; i < total_pixels; i++) {
        buf[i] = color;
    }
}

void gfx_put_pixel(int32_t x, int32_t y, color_t color) {
    if (x < 0 || y < 0 || (uint32_t)x >= s_fb.width || (uint32_t)y >= s_fb.height) return;
    uint32_t* buf = gfx_get_backbuffer();
    buf[y * (s_fb.pitch / 4) + x] = color;
}

color_t gfx_get_pixel(int32_t x, int32_t y) {
    if (x < 0 || y < 0 || (uint32_t)x >= s_fb.width || (uint32_t)y >= s_fb.height) return 0;
    uint32_t* buf = gfx_get_backbuffer();
    return buf[y * (s_fb.pitch / 4) + x];
}

void gfx_draw_line(int32_t x0, int32_t y0, int32_t x1, int32_t y1, color_t color) {
    int32_t dx = (x1 > x0) ? (x1 - x0) : (x0 - x1);
    int32_t dy = (y1 > y0) ? (y1 - y0) : (y0 - y1);
    int32_t sx = (x0 < x1) ? 1 : -1;
    int32_t sy = (y0 < y1) ? 1 : -1;
    int32_t err = ((dx > dy) ? dx : -dy) / 2;
    int32_t e2;

    while (1) {
        gfx_put_pixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        e2 = err;
        if (e2 > -dx) { err -= dy; x0 += sx; }
        if (e2 <  dy) { err += dx; y0 += sy; }
    }
}

void gfx_draw_rect(int32_t x, int32_t y, uint32_t w, uint32_t h, color_t color) {
    if (w == 0 || h == 0) return;
    gfx_draw_line(x, y, x + w - 1, y, color);
    gfx_draw_line(x, y + h - 1, x + w - 1, y + h - 1, color);
    gfx_draw_line(x, y, x, y + h - 1, color);
    gfx_draw_line(x + w - 1, y, x + w - 1, y + h - 1, color);
}

void gfx_fill_rect(int32_t x, int32_t y, uint32_t w, uint32_t h, color_t color) {
    if (w == 0 || h == 0) return;
    int32_t x0 = (x < 0) ? 0 : x;
    int32_t y0 = (y < 0) ? 0 : y;
    int32_t x1 = x + (int32_t)w;
    int32_t y1 = y + (int32_t)h;
    if (x1 > (int32_t)s_fb.width)  x1 = (int32_t)s_fb.width;
    if (y1 > (int32_t)s_fb.height) y1 = (int32_t)s_fb.height;

    uint32_t* buf = gfx_get_backbuffer();
    uint32_t row_stride = s_fb.pitch / 4;

    for (int32_t py = y0; py < y1; py++) {
        uint32_t* row = &buf[py * row_stride + x0];
        for (int32_t px = x0; px < x1; px++) {
            *row++ = color;
        }
    }
}

void gfx_draw_char(int32_t x, int32_t y, char c, color_t fg, color_t bg) {
    if ((uint8_t)c > 127) c = '?';
    const uint8_t* glyph = g_font_8x16[(uint8_t)c];

    for (int row = 0; row < 16; row++) {
        uint8_t bits = glyph[row];
        for (int col = 0; col < 8; col++) {
            if (bits & (0x80 >> col)) {
                gfx_put_pixel(x + col, y + row, fg);
            } else if ((bg & 0xFF000000) != 0) {
                gfx_put_pixel(x + col, y + row, bg);
            }
        }
    }
}

void gfx_draw_string(int32_t x, int32_t y, const char* str, color_t fg, color_t bg) {
    if (!str) return;
    int32_t cx = x;
    int32_t cy = y;
    while (*str) {
        if (*str == '\n') {
            cx = x;
            cy += 18;
        } else {
            gfx_draw_char(cx, cy, *str, fg, bg);
            cx += 8;
        }
        str++;
    }
}

void gfx_draw_cursor(int32_t x, int32_t y) {
    static const char* cursor_bitmap[] = {
        "X               ",
        "XX              ",
        "X.X             ",
        "X..X            ",
        "X...X           ",
        "X....X          ",
        "X.....X         ",
        "X......X        ",
        "X.......X       ",
        "X........X      ",
        "X.....XXXXX     ",
        "X..X..X         ",
        "X.X X..X        ",
        "XX   X..X       ",
        "X     X..X      ",
        "       XX       "
    };

    for (int row = 0; row < 16; row++) {
        for (int col = 0; col < 16; col++) {
            char p = cursor_bitmap[row][col];
            if (p == 'X') {
                gfx_put_pixel(x + col, y + row, COLOR_BLACK);
            } else if (p == '.') {
                gfx_put_pixel(x + col, y + row, COLOR_WHITE);
            }
        }
    }
}

void gfx_swap_buffers(void) {
    if (!s_backbuffer || !s_fb.addr) return;
    kmemcpy(s_fb.addr, s_backbuffer, s_buffer_size);
}
