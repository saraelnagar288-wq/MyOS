#ifndef MYOS_WINDOW_H
#define MYOS_WINDOW_H

#include <myos/types.h>
#include <myos/gfx.h>
#include <myos/input.h>

#define MAX_WINDOWS 32
#define WINDOW_TITLE_HEIGHT 26
#define TITLEBAR_HEIGHT     WINDOW_TITLE_HEIGHT

#define WINDOW_FLAG_NONE      0
#define WINDOW_FLAG_RESIZABLE 1
#define WINDOW_FLAG_MODAL     2

#define WINDOW_STATE_NORMAL    0
#define WINDOW_STATE_MINIMIZED 1
#define WINDOW_STATE_MAXIMIZED 2

/* Aesthetic UI Themes */
#define COLOR_DESKTOP_BG     COLOR_RGB(30, 41, 59)
#define COLOR_TASKBAR_BG     COLOR_RGB(15, 23, 42)
#define COLOR_START_BTN      COLOR_RGB(37, 99, 235)
#define COLOR_START_MENU_BG  COLOR_RGB(30, 41, 59)
#define COLOR_WINDOW_BG      COLOR_RGB(248, 250, 252)
#define COLOR_BORDER         COLOR_RGB(148, 163, 184)
#define COLOR_TITLE_ACTIVE   COLOR_RGB(37, 99, 235)
#define COLOR_TITLE_INACTIVE COLOR_RGB(100, 116, 139)
#define COLOR_BTN_CLOSE      COLOR_RGB(239, 68, 68)
#define COLOR_BTN_MAX        COLOR_RGB(34, 197, 94)
#define COLOR_BTN_MIN        COLOR_RGB(234, 179, 8)

typedef struct window window_t;

typedef void (*window_paint_fn)(window_t* win);
typedef void (*window_event_fn)(window_t* win, const input_event_t* ev);

struct window {
    uint32_t id;
    char title[64];
    int32_t x;
    int32_t y;
    uint32_t width;
    uint32_t height;
    uint32_t flags;
    uint8_t state;
    uint32_t z_order;
    bool is_visible;
    bool is_minimized;
    bool is_maximized;
    bool is_focused;
    bool is_dragging;
    int32_t drag_offset_x;
    int32_t drag_offset_y;
    uint32_t* buffer;
    uint32_t* client_buffer;
    void (*render_content)(window_t* win);
    window_paint_fn on_paint;
    window_event_fn on_event;
    void* user_data;
};

void wm_init(void);
window_t* wm_create_window(const char* title, int32_t x, int32_t y, uint32_t width, uint32_t height, uint32_t flags);
void wm_destroy_window(window_t* win);
void wm_bring_to_front(window_t* win);
void wm_focus_window(window_t* win);
window_t* wm_get_window_at(uint32_t index);
bool wm_handle_mouse(int32_t mouse_x, int32_t mouse_y, uint8_t buttons);
window_t* wm_get_focused_window(void);
window_t* wm_get_focused(void);
void wm_render_window(window_t* win);
void wm_render_all(void);
void wm_render(void);
void wm_handle_input(const input_event_t* ev);
uint32_t wm_get_window_count(void);
window_t* wm_get_window(uint32_t index);

#endif /* MYOS_WINDOW_H */
