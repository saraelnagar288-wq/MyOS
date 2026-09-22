#include <myos/window.h>
#include <myos/kheap.h>
#include <myos/kernel.h>

static window_t* g_windows[MAX_WINDOWS];
static uint32_t  g_window_count = 0;
static window_t* g_focused_window = NULL;
static uint32_t  g_next_window_id = 1;

void wm_init(void) {
    kmemset(g_windows, 0, sizeof(g_windows));
    g_window_count = 0;
    g_focused_window = NULL;
    LOG_INFO("WM", "Window Manager initialized");
}

window_t* wm_create_window(const char* title, int32_t x, int32_t y, uint32_t width, uint32_t height, uint32_t flags) {
    if (g_window_count >= MAX_WINDOWS) return NULL;

    window_t* win = (window_t*)kzalloc(sizeof(window_t));
    win->id = g_next_window_id++;
    kstrcpy(win->title, title);
    win->x = x;
    win->y = y;
    win->width = width;
    win->height = height;
    win->flags = flags;
    win->state = WINDOW_STATE_NORMAL;
    win->z_order = g_window_count;

    /* Allocate window client buffer */
    win->buffer = (uint32_t*)kmalloc(width * height * sizeof(uint32_t));
    if (win->buffer) {
        for (uint32_t i = 0; i < width * height; i++) {
            win->buffer[i] = COLOR_WINDOW_BG;
        }
    }

    g_windows[g_window_count++] = win;
    g_focused_window = win;

    return win;
}

void wm_destroy_window(window_t* win) {
    if (!win) return;

    for (uint32_t i = 0; i < g_window_count; i++) {
        if (g_windows[i] == win) {
            if (win->buffer) {
                kfree(win->buffer);
            }
            kfree(win);

            for (uint32_t j = i; j < g_window_count - 1; j++) {
                g_windows[j] = g_windows[j + 1];
            }
            g_windows[--g_window_count] = NULL;
            break;
        }
    }

    g_focused_window = (g_window_count > 0) ? g_windows[g_window_count - 1] : NULL;
}

void wm_focus_window(window_t* win) {
    if (!win || g_focused_window == win) return;

    /* Move window to top of z-order list */
    for (uint32_t i = 0; i < g_window_count; i++) {
        if (g_windows[i] == win) {
            for (uint32_t j = i; j < g_window_count - 1; j++) {
                g_windows[j] = g_windows[j + 1];
            }
            g_windows[g_window_count - 1] = win;
            break;
        }
    }
    g_focused_window = win;
}

window_t* wm_get_window_at(int32_t x, int32_t y) {
    /* Search in reverse z-order (topmost first) */
    for (int i = (int)g_window_count - 1; i >= 0; i--) {
        window_t* w = g_windows[i];
        if (w->state == WINDOW_STATE_MINIMIZED) continue;

        if (x >= w->x && x < (int32_t)(w->x + w->width) &&
            y >= w->y && y < (int32_t)(w->y + w->height)) {
            return w;
        }
    }
    return NULL;
}

window_t* wm_get_focused_window(void) {
    return g_focused_window;
}

void wm_render_window(window_t* win) {
    if (!win || win->state == WINDOW_STATE_MINIMIZED) return;

    bool is_focused = (win == g_focused_window);
    uint32_t title_color = is_focused ? COLOR_TITLE_ACTIVE : COLOR_TITLE_INACTIVE;

    /* Window Shadow effect (simple dark border/outline) */
    gfx_fill_rect(win->x + 3, win->y + 3, win->width, win->height, COLOR_RGB(10, 15, 25));

    /* Window Background */
    gfx_fill_rect(win->x, win->y, win->width, win->height, COLOR_WINDOW_BG);

    /* Window Border */
    gfx_draw_rect(win->x, win->y, win->width, win->height, COLOR_BORDER);

    /* Title Bar */
    gfx_fill_rect(win->x + 1, win->y + 1, win->width - 2, TITLEBAR_HEIGHT, title_color);

    /* Title text */
    gfx_draw_string(win->x + 10, win->y + 6, win->title, COLOR_WHITE, title_color);

    /* Window Control Buttons */
    /* Close Button (Red) */
    uint32_t btn_close_x = win->x + win->width - 24;
    gfx_fill_rect(btn_close_x, win->y + 6, 16, 16, COLOR_BTN_CLOSE);
    gfx_draw_string(btn_close_x + 5, win->y + 6, "x", COLOR_WHITE, COLOR_BTN_CLOSE);

    /* Maximize Button (Green) */
    uint32_t btn_max_x = win->x + win->width - 44;
    gfx_fill_rect(btn_max_x, win->y + 6, 16, 16, COLOR_BTN_MAX);
    gfx_draw_string(btn_max_x + 4, win->y + 6, "+", COLOR_WHITE, COLOR_BTN_MAX);

    /* Minimize Button (Yellow) */
    uint32_t btn_min_x = win->x + win->width - 64;
    gfx_fill_rect(btn_min_x, win->y + 6, 16, 16, COLOR_BTN_MIN);
    gfx_draw_string(btn_min_x + 4, win->y + 6, "-", COLOR_BLACK, COLOR_BTN_MIN);

    /* Blit client area buffer if present */
    if (win->buffer) {
        uint32_t client_h = win->height - TITLEBAR_HEIGHT - 2;
        uint32_t client_w = win->width - 2;
        gfx_draw_bitmap(win->x + 1, win->y + TITLEBAR_HEIGHT + 1, client_w, client_h, win->buffer, false);
    }
}

void wm_render_all(void) {
    for (uint32_t i = 0; i < g_window_count; i++) {
        wm_render_window(g_windows[i]);
    }
}
