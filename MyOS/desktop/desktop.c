#include <myos/desktop.h>
#include <myos/window.h>
#include <myos/gfx.h>
#include <myos/input.h>
#include <myos/kernel.h>
#include <myos/hal.h>
#include <myos/pmm.h>
#include <myos/cpu.h>

extern void app_terminal_open(void);
extern void app_terminal_handle_key(char c);
extern void app_filemanager_open(void);
extern void app_taskmanager_open(void);
extern void app_sysinfo_open(void);
extern void app_settings_open(void);

static bool s_start_menu_open = false;
static color_t s_wallpaper_color = 0xFF1A2634; /* Elegant Slate Charcoal */
static int32_t s_mouse_x = 512;
static int32_t s_mouse_y = 384;
static uint8_t s_mouse_btns = 0;

typedef struct {
    const char* label;
    int32_t x;
    int32_t y;
    color_t icon_bg;
    char icon_char;
    void (*open_func)(void);
} desktop_icon_t;

static desktop_icon_t s_desktop_icons[] = {
    { "MyTerminal",    24,  24, 0xFF2D3748, '>', app_terminal_open },
    { "MyFiles",       24, 114, 0xFFDD6B20, 'F', app_filemanager_open },
    { "MyTaskManager", 24, 204, 0xFF319795, 'T', app_taskmanager_open },
    { "MySystem",      24, 294, 0xFF3182CE, 'S', app_sysinfo_open },
    { "MySettings",    24, 384, 0xFF805AD5, '*', app_settings_open },
};

static void draw_desktop_icons(void) {
    size_t count = sizeof(s_desktop_icons) / sizeof(s_desktop_icons[0]);
    for (size_t i = 0; i < count; i++) {
        desktop_icon_t* ico = &s_desktop_icons[i];

        /* Icon tile */
        gfx_fill_rect(ico->x + 8, ico->y, 48, 48, ico->icon_bg);
        gfx_draw_rect(ico->x + 8, ico->y, 48, 48, COLOR_WHITE);
        gfx_draw_char(ico->x + 28, ico->y + 16, ico->icon_char, COLOR_WHITE, ico->icon_bg);

        /* Icon label text with dark background for legibility */
        size_t len = kstrlen(ico->label);
        int32_t text_x = ico->x + 32 - ((int32_t)len * 4);
        if (text_x < 0) text_x = 0;
        gfx_fill_rect(text_x - 4, ico->y + 54, (uint32_t)(len * 8 + 8), 18, 0x88000000);
        gfx_draw_string(text_x, ico->y + 55, ico->label, COLOR_WHITE, 0);
    }
}

static void draw_taskbar(uint32_t sw, uint32_t sh) {
    int32_t bar_y = sh - 44;

    /* Taskbar base bar */
    gfx_fill_rect(0, bar_y, sw, 44, 0xFF141923);
    gfx_draw_line(0, bar_y, sw, bar_y, 0xFF2D3748);

    /* Start Menu Button */
    color_t start_bg = s_start_menu_open ? 0xFF3182CE : 0xFF2B6CB0;
    gfx_fill_rect(8, bar_y + 6, 92, 32, start_bg);
    gfx_draw_string(20, bar_y + 14, "[ MyOS ]", COLOR_WHITE, start_bg);

    /* Running window tabs */
    uint32_t win_count = wm_get_window_count();
    int32_t tab_x = 112;
    for (uint32_t i = 0; i < win_count; i++) {
        window_t* w = wm_get_window_at(i);
        if (!w) continue;

        color_t tab_bg = w->is_focused ? 0xFF2D3748 : 0xFF1A202C;
        gfx_fill_rect(tab_x, bar_y + 6, 140, 32, tab_bg);
        gfx_draw_rect(tab_x, bar_y + 6, 140, 32, 0xFF4A5568);

        char short_title[16];
        kstrncpy(short_title, w->title, 14);
        short_title[14] = '\0';
        gfx_draw_string(tab_x + 10, bar_y + 14, short_title, COLOR_WHITE, tab_bg);

        tab_x += 148;
    }

    /* System Tray (Right edge): Clock & Status */
    rtc_time_t now;
    hal_get_time(&now);
    char time_str[32];
    ksnprintf(time_str, sizeof(time_str), "%02u:%02u:%02u", now.hour, now.minute, now.second);

    int32_t clock_x = sw - 100;
    gfx_fill_rect(clock_x - 8, bar_y + 6, 96, 32, 0xFF1A202C);
    gfx_draw_rect(clock_x - 8, bar_y + 6, 96, 32, 0xFF2D3748);
    gfx_draw_string(clock_x + 8, bar_y + 14, time_str, COLOR_WHITE, 0xFF1A202C);

    /* RAM usage indicator pill */
    const pmm_stats_t* pmm = pmm_get_stats();
    char ram_str[32];
    ksnprintf(ram_str, sizeof(ram_str), "RAM: %lluMB", pmm->used_memory / (1024*1024));
    int32_t ram_x = clock_x - 110;
    gfx_fill_rect(ram_x - 6, bar_y + 6, 100, 32, 0xFF1A202C);
    gfx_draw_rect(ram_x - 6, bar_y + 6, 100, 32, 0xFF2D3748);
    gfx_draw_string(ram_x + 6, bar_y + 14, ram_str, 0xFF63B3ED, 0xFF1A202C);
}

static void draw_start_menu(uint32_t sh) {
    if (!s_start_menu_open) return;

    int32_t menu_w = 260;
    int32_t menu_h = 280;
    int32_t menu_x = 8;
    int32_t menu_y = sh - 44 - menu_h - 4;

    /* Shadow */
    gfx_fill_rect(menu_x + 4, menu_y + 4, menu_w, menu_h, 0x44000000);

    /* Menu container */
    gfx_fill_rect(menu_x, menu_y, menu_w, menu_h, 0xFF1A202C);
    gfx_draw_rect(menu_x, menu_y, menu_w, menu_h, 0xFF4A5568);

    /* Menu Header */
    gfx_fill_rect(menu_x, menu_y, menu_w, 40, 0xFF2B6CB0);
    gfx_draw_string(menu_x + 16, menu_y + 12, "MyOS System 0.1.0", COLOR_WHITE, 0xFF2B6CB0);

    /* Menu Items */
    const char* items[] = {
        "> MyTerminal",
        "F MyFiles",
        "T MyTaskManager",
        "S MySystem",
        "* MySettings",
        "- - - - - - - - - - - -",
        "! Restart System",
        "X Shut Down"
    };

    int32_t item_y = menu_y + 48;
    for (size_t i = 0; i < sizeof(items)/sizeof(items[0]); i++) {
        gfx_draw_string(menu_x + 16, item_y, items[i], COLOR_WHITE, 0xFF1A202C);
        item_y += 28;
    }
}

static void handle_click(int32_t mx, int32_t my, uint32_t sw, uint32_t sh) {
    int32_t bar_y = sh - 44;

    /* Start Button clicked */
    if (mx >= 8 && mx <= 100 && my >= bar_y + 6 && my <= bar_y + 38) {
        s_start_menu_open = !s_start_menu_open;
        return;
    }

    /* Start Menu items clicked */
    if (s_start_menu_open) {
        int32_t menu_w = 260;
        int32_t menu_h = 280;
        int32_t menu_x = 8;
        int32_t menu_y = sh - 44 - menu_h - 4;

        if (mx >= menu_x && mx <= menu_x + menu_w && my >= menu_y && my <= menu_y + menu_h) {
            int32_t rel_y = my - (menu_y + 48);
            int item_idx = rel_y / 28;
            s_start_menu_open = false;

            if (item_idx == 0) app_terminal_open();
            else if (item_idx == 1) app_filemanager_open();
            else if (item_idx == 2) app_taskmanager_open();
            else if (item_idx == 3) app_sysinfo_open();
            else if (item_idx == 4) app_settings_open();
            else if (item_idx == 6) hal_reboot();
            else if (item_idx == 7) hal_shutdown();
            return;
        } else {
            s_start_menu_open = false;
        }
    }

    /* Window manager mouse handling */
    if (wm_handle_mouse(mx, my, 1)) {
        return;
    }

    /* Desktop Icons clicked */
    size_t count = sizeof(s_desktop_icons) / sizeof(s_desktop_icons[0]);
    for (size_t i = 0; i < count; i++) {
        desktop_icon_t* ico = &s_desktop_icons[i];
        if (mx >= ico->x && mx <= ico->x + 64 && my >= ico->y && my <= ico->y + 72) {
            if (ico->open_func) {
                ico->open_func();
            }
            return;
        }
    }
}

void desktop_init(void) {
    wm_init();

    /* Automatically open MyTerminal and MySystem on first boot for convenience */
    app_sysinfo_open();
    app_terminal_open();

    LOG_INFO("DESKTOP", "MyOS Desktop Environment loaded");
}

void desktop_render_frame(void) {
    const framebuffer_t* fb = gfx_get_fb();
    uint32_t sw = fb->width;
    uint32_t sh = fb->height;

    /* 1. Render Wallpaper with subtle grid accent */
    gfx_clear(s_wallpaper_color);

    for (uint32_t gy = 0; gy < sh - 44; gy += 40) {
        for (uint32_t gx = 0; gx < sw; gx += 40) {
            gfx_put_pixel(gx, gy, 0xFF2A3644);
        }
    }

    /* 2. Render Desktop Icons */
    draw_desktop_icons();

    /* 3. Render Windows */
    wm_render_all();

    /* 4. Render Taskbar */
    draw_taskbar(sw, sh);

    /* 5. Render Start Menu */
    draw_start_menu(sh);

    /* 6. Render Mouse Cursor */
    gfx_draw_cursor(s_mouse_x, s_mouse_y);

    /* 7. Swap backbuffer to linear VRAM */
    gfx_swap_buffers();
}

void desktop_run(void) {
    const framebuffer_t* fb = gfx_get_fb();
    mouse_init(fb->width, fb->height);
    keyboard_init();

    LOG_INFO("DESKTOP", "Entering interactive GUI event loop");

    uint8_t prev_buttons = 0;

    while (1) {
        /* Poll and dispatch hardware input events */
        input_event_t ev;
        while (input_poll_event(&ev)) {
            if (ev.type == INPUT_EVENT_MOUSE_MOVE) {
                s_mouse_x = ev.mouse.x;
                s_mouse_y = ev.mouse.y;
                if (ev.mouse.left_button) {
                    wm_handle_mouse(s_mouse_x, s_mouse_y, 1);
                }
            } else if (ev.type == INPUT_EVENT_MOUSE_DOWN) {
                s_mouse_btns |= 1;
                handle_click(s_mouse_x, s_mouse_y, fb->width, fb->height);
            } else if (ev.type == INPUT_EVENT_MOUSE_UP) {
                s_mouse_btns &= ~1;
                wm_handle_mouse(s_mouse_x, s_mouse_y, 0);
            } else if (ev.type == INPUT_EVENT_KEY_DOWN) {
                if (ev.key.character != 0) {
                    app_terminal_handle_key(ev.key.character);
                }
            }
        }

        /* Continuous mouse sampling fallback */
        int32_t mx, my;
        uint8_t btns;
        mouse_get_position(&mx, &my, &btns);
        s_mouse_x = mx;
        s_mouse_y = my;
        if ((btns & 1) && !(prev_buttons & 1)) {
            handle_click(mx, my, fb->width, fb->height);
        } else if (!(btns & 1) && (prev_buttons & 1)) {
            wm_handle_mouse(mx, my, 0);
        }
        prev_buttons = btns;

        /* Render entire composed frame */
        desktop_render_frame();

        /* Yield to other kernel threads or sleep slightly */
        hal_sleep_ms(16); /* ~60 FPS */
    }
}
