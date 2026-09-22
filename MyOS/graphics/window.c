#include <myos/window.h>
#include <myos/gfx.h>
#include <myos/kheap.h>
#include <myos/kernel.h>

static window_t* s_windows[MAX_WINDOWS];
static uint32_t s_window_count = 0;
static window_t* s_focused_window = NULL;
static uint32_t s_next_window_id = 1;

/* Dragging state */
static window_t* s_dragging_window = NULL;
static int32_t s_drag_offset_x = 0;
static int32_t s_drag_offset_y = 0;

void wm_init(void) {
    kmemset(s_windows, 0, sizeof(s_windows));
    s_window_count = 0;
    s_focused_window = NULL;
    s_dragging_window = NULL;
    LOG_INFO("WM", "Window Manager & 2D Compositor initialized");
}

window_t* wm_create_window(const char* title, int32_t x, int32_t y, uint32_t w, uint32_t h, uint32_t flags) {
    if (s_window_count >= MAX_WINDOWS) return NULL;

    window_t* win = (window_t*)kzalloc(sizeof(window_t));
    if (!win) return NULL;

    win->id = s_next_window_id++;
    kstrncpy(win->title, title, 63);
    win->x = x;
    win->y = y;
    win->width = w;
    win->height = h;
    win->flags = flags;
    win->is_visible = true;
    win->is_focused = true;

    /* Allocate client buffer */
    win->buffer = (uint32_t*)kmalloc(w * h * sizeof(uint32_t));
    if (win->buffer) {
        for (uint32_t i = 0; i < w * h; i++) {
            win->buffer[i] = COLOR_WHITE;
        }
    }

    s_windows[s_window_count++] = win;
    wm_bring_to_front(win);

    LOG_INFO("WM", "Created window '%s' (%ux%u at %d,%d)", title, w, h, x, y);
    return win;
}

void wm_destroy_window(window_t* win) {
    if (!win) return;
    for (uint32_t i = 0; i < s_window_count; i++) {
        if (s_windows[i] == win) {
            if (win->buffer) kfree(win->buffer);
            kfree(win);
            for (uint32_t j = i; j < s_window_count - 1; j++) {
                s_windows[j] = s_windows[j + 1];
            }
            s_windows[--s_window_count] = NULL;
            if (s_focused_window == win) {
                s_focused_window = (s_window_count > 0) ? s_windows[s_window_count - 1] : NULL;
            }
            break;
        }
    }
}

void wm_bring_to_front(window_t* win) {
    if (!win || s_window_count <= 1) return;
    int index = -1;
    for (uint32_t i = 0; i < s_window_count; i++) {
        if (s_windows[i] == win) {
            index = i;
            break;
        }
    }
    if (index >= 0 && (uint32_t)index < s_window_count - 1) {
        for (uint32_t i = index; i < s_window_count - 1; i++) {
            s_windows[i] = s_windows[i + 1];
        }
        s_windows[s_window_count - 1] = win;
    }
    if (s_focused_window) s_focused_window->is_focused = false;
    s_focused_window = win;
    win->is_focused = true;
}

void wm_render_window(window_t* win) {
    if (!win || !win->is_visible) return;

    int32_t x = win->x;
    int32_t y = win->y;
    uint32_t w = win->width;
    uint32_t h = win->height;

    /* Drop shadow */
    gfx_fill_rect(x + 4, y + 4, w + 4, h + 32, 0x33000000);

    /* Window outer border */
    gfx_fill_rect(x - 1, y - 1, w + 2, h + 30, COLOR_DARK_GRAY);

    /* Title bar */
    color_t title_bg = win->is_focused ? COLOR_NAVY : COLOR_GRAY;
    gfx_fill_rect(x, y, w, 28, title_bg);

    /* Title text */
    gfx_draw_string(x + 10, y + 6, win->title, COLOR_WHITE, title_bg);

    /* Window controls: Close button [X] */
    int32_t btn_close_x = x + w - 24;
    int32_t btn_y = y + 4;
    gfx_fill_rect(btn_close_x, btn_y, 20, 20, COLOR_RED);
    gfx_draw_char(btn_close_x + 6, btn_y + 2, 'x', COLOR_WHITE, COLOR_RED);

    /* Minimize button [-] */
    int32_t btn_min_x = x + w - 48;
    gfx_fill_rect(btn_min_x, btn_y, 20, 20, COLOR_DARK_GRAY);
    gfx_draw_char(btn_min_x + 6, btn_y + 2, '-', COLOR_WHITE, COLOR_DARK_GRAY);

    /* Client area */
    int32_t client_y = y + 28;
    if (win->render_content != NULL) {
        win->render_content(win);
    } else if (win->buffer != NULL) {
        /* Render client buffer */
        const framebuffer_t* fb = gfx_get_fb();
        uint32_t* backbuf = gfx_get_backbuffer();
        uint32_t stride = fb->pitch / 4;

        for (uint32_t cy = 0; cy < h; cy++) {
            int32_t screen_y = client_y + cy;
            if (screen_y < 0 || (uint32_t)screen_y >= fb->height) continue;
            for (uint32_t cx = 0; cx < w; cx++) {
                int32_t screen_x = x + cx;
                if (screen_x < 0 || (uint32_t)screen_x >= fb->width) continue;
                backbuf[screen_y * stride + screen_x] = win->buffer[cy * w + cx];
            }
        }
    } else {
        gfx_fill_rect(x, client_y, w, h, COLOR_WHITE);
    }
}

void wm_render_all(void) {
    for (uint32_t i = 0; i < s_window_count; i++) {
        wm_render_window(s_windows[i]);
    }
}

bool wm_handle_mouse(int32_t mx, int32_t my, uint8_t buttons) {
    bool left_clicked = (buttons & 1) != 0;

    if (!left_clicked) {
        s_dragging_window = NULL;
    }

    if (s_dragging_window != NULL) {
        s_dragging_window->x = mx - s_drag_offset_x;
        s_dragging_window->y = my - s_drag_offset_y;
        return true;
    }

    if (left_clicked) {
        /* Check windows from top to bottom */
        for (int i = (int)s_window_count - 1; i >= 0; i--) {
            window_t* win = s_windows[i];
            if (!win->is_visible) continue;

            /* Check close button */
            int32_t close_x = win->x + win->width - 24;
            if (mx >= close_x && mx <= close_x + 20 && my >= win->y + 4 && my <= win->y + 24) {
                wm_destroy_window(win);
                return true;
            }

            /* Check title bar for dragging */
            if (mx >= win->x && mx <= win->x + (int32_t)win->width &&
                my >= win->y && my <= win->y + 28) {
                wm_bring_to_front(win);
                s_dragging_window = win;
                s_drag_offset_x = mx - win->x;
                s_drag_offset_y = my - win->y;
                return true;
            }

            /* Check client area click */
            if (mx >= win->x && mx <= win->x + (int32_t)win->width &&
                my >= win->y + 28 && my <= win->y + 28 + (int32_t)win->height) {
                wm_bring_to_front(win);
                return true;
            }
        }
    }

    return false;
}

window_t* wm_get_focused(void) {
    return s_focused_window;
}

uint32_t wm_get_window_count(void) {
    return s_window_count;
}

window_t* wm_get_window_at(uint32_t index) {
    if (index >= s_window_count) return NULL;
    return s_windows[index];
}
