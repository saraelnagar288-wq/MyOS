#include <myos/desktop.h>
#include <myos/window.h>
#include <myos/gfx.h>
#include <myos/kernel.h>
#include <myos/hal.h>

static window_t* s_settings_window = NULL;

static void settings_render_content(window_t* win) {
    int32_t cx = win->x;
    int32_t cy = win->y + 28;

    gfx_fill_rect(cx, cy, win->width, win->height, 0xFFF7FAFC);

    /* Desktop Appearance Section */
    gfx_fill_rect(cx + 16, cy + 16, win->width - 32, 110, COLOR_WHITE);
    gfx_draw_rect(cx + 16, cy + 16, win->width - 32, 110, COLOR_GRAY);
    gfx_draw_string(cx + 26, cy + 24, "Desktop Appearance", COLOR_NAVY, COLOR_WHITE);
    gfx_draw_string(cx + 26, cy + 46, "Select Wallpaper Color Theme:", COLOR_BLACK, COLOR_WHITE);

    /* Theme buttons */
    gfx_fill_rect(cx + 26, cy + 72, 100, 32, 0xFF1A202C); /* Midnight Slate */
    gfx_draw_string(cx + 34, cy + 80, "Slate", COLOR_WHITE, 0xFF1A202C);

    gfx_fill_rect(cx + 136, cy + 72, 100, 32, 0xFF1A365D); /* Deep Navy */
    gfx_draw_string(cx + 144, cy + 80, "Navy", COLOR_WHITE, 0xFF1A365D);

    gfx_fill_rect(cx + 246, cy + 72, 100, 32, 0xFF1C453B); /* Forest */
    gfx_draw_string(cx + 254, cy + 80, "Forest", COLOR_WHITE, 0xFF1C453B);

    /* Display Resolution Section */
    gfx_fill_rect(cx + 16, cy + 138, win->width - 32, 70, COLOR_WHITE);
    gfx_draw_rect(cx + 16, cy + 138, win->width - 32, 70, COLOR_GRAY);
    gfx_draw_string(cx + 26, cy + 146, "Display & Monitor", COLOR_NAVY, COLOR_WHITE);
    const framebuffer_t* fb = gfx_get_fb();
    char buf[64];
    ksnprintf(buf, sizeof(buf), "Resolution: %u x %u @ %u bpp (VBE LFB)", fb->width, fb->height, fb->bpp);
    gfx_draw_string(cx + 26, cy + 172, buf, COLOR_DARK_GRAY, COLOR_WHITE);

    /* Power Management Section */
    gfx_fill_rect(cx + 16, cy + 220, win->width - 32, 80, COLOR_WHITE);
    gfx_draw_rect(cx + 16, cy + 220, win->width - 32, 80, COLOR_GRAY);
    gfx_draw_string(cx + 26, cy + 228, "Power Actions", COLOR_NAVY, COLOR_WHITE);

    /* Reboot & Shutdown Buttons */
    gfx_fill_rect(cx + 26, cy + 252, 120, 32, 0xFFDD6B20); /* Orange Reboot */
    gfx_draw_string(cx + 42, cy + 260, "Restart", COLOR_WHITE, 0xFFDD6B20);

    gfx_fill_rect(cx + 160, cy + 252, 120, 32, 0xFFE53E3E); /* Red Shutdown */
    gfx_draw_string(cx + 176, cy + 260, "Shut Down", COLOR_WHITE, 0xFFE53E3E);
}

void app_settings_open(void) {
    if (s_settings_window != NULL) {
        wm_bring_to_front(s_settings_window);
        return;
    }

    s_settings_window = wm_create_window("MySettings - Control Panel", 160, 120, 440, 330, 0);
    if (s_settings_window) {
        s_settings_window->render_content = settings_render_content;
    }
}
