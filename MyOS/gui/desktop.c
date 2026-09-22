#include <myos/desktop.h>
#include <myos/window.h>
#include <myos/gfx.h>
#include <myos/input.h>
#include <myos/hal.h>
#include <myos/cpu.h>
#include <myos/pmm.h>
#include <myos/kheap.h>
#include <myos/pci.h>
#include <myos/process.h>
#include <myos/scheduler.h>
#include <myos/fs.h>
#include <myos/kernel.h>

/* Desktop state */
static bool g_desktop_running = false;
static bool g_start_menu_open = false;
static int32_t g_drag_offset_x = 0;
static int32_t g_drag_offset_y = 0;
static window_t* g_drag_window = NULL;

/* Desktop Icons */
#define DESKTOP_ICON_COUNT 5
static desktop_icon_t g_desktop_icons[DESKTOP_ICON_COUNT] = {
    {"MyTerminal",    30,  30, 48, 48, APP_TERMINAL,    COLOR_RGB(40, 44, 52)},
    {"MyFiles",       30, 110, 48, 48, APP_FILE_MANAGER,COLOR_RGB(33, 150, 243)},
    {"MyTaskManager", 30, 190, 48, 48, APP_TASK_MANAGER,COLOR_RGB(76, 175, 80)},
    {"MySettings",    30, 270, 48, 48, APP_SETTINGS,    COLOR_RGB(156, 39, 176)},
    {"MySystem",      30, 350, 48, 48, APP_SYSTEM_INFO, COLOR_RGB(255, 152, 0)},
};

/* Terminal buffer in window */
#define TERM_ROWS 16
#define TERM_COLS 50
static char g_term_lines[TERM_ROWS][TERM_COLS];
static char g_term_input[TERM_COLS];
static uint32_t g_term_input_len = 0;

static void term_add_line(const char* line) {
    for (int r = 0; r < TERM_ROWS - 1; r++) {
        kstrcpy(g_term_lines[r], g_term_lines[r + 1]);
    }
    kstrncpy(g_term_lines[TERM_ROWS - 1], line, TERM_COLS - 1);
    g_term_lines[TERM_ROWS - 1][TERM_COLS - 1] = '\0';
}

void app_terminal_launch(void) {
    window_t* win = wm_create_window("MyTerminal - MyShell", 120, 80, 520, 360, WINDOW_FLAG_RESIZABLE);
    if (!win) return;

    for (int r = 0; r < TERM_ROWS; r++) {
        g_term_lines[r][0] = '\0';
    }
    term_add_line("MyOS MyShell v0.1.0 (x86_64)");
    term_add_line("Type 'help' for available commands.");
    term_add_line("");
    g_term_input_len = 0;
    g_term_input[0] = '\0';
}

static void term_render_content(window_t* win) {
    if (!win) return;
    int base_x = win->x + 10;
    int base_y = win->y + TITLEBAR_HEIGHT + 10;

    gfx_fill_rect(win->x + 1, win->y + TITLEBAR_HEIGHT + 1, win->width - 2, win->height - TITLEBAR_HEIGHT - 2, COLOR_RGB(18, 22, 28));

    for (int r = 0; r < TERM_ROWS; r++) {
        if (g_term_lines[r][0] != '\0') {
            gfx_draw_string(base_x, base_y + (r * 18), g_term_lines[r], COLOR_RGB(220, 220, 220), COLOR_RGB(18, 22, 28));
        }
    }

    /* Prompt and current input line */
    int prompt_y = base_y + ((TERM_ROWS - 1) * 18);
    gfx_draw_string(base_x, prompt_y, "$ ", COLOR_RGB(76, 175, 80), COLOR_RGB(18, 22, 28));
    gfx_draw_string(base_x + 16, prompt_y, g_term_input, COLOR_WHITE, COLOR_RGB(18, 22, 28));

    /* Cursor */
    gfx_fill_rect(base_x + 16 + (g_term_input_len * 8), prompt_y, 8, 16, COLOR_RGB(100, 181, 246));
}

void app_file_manager_launch(void) {
    window_t* win = wm_create_window("MyFiles - File Manager", 180, 120, 560, 380, WINDOW_FLAG_RESIZABLE);
    if (!win) return;
}

static void file_manager_render_content(window_t* win) {
    if (!win) return;
    int base_x = win->x + 10;
    int base_y = win->y + TITLEBAR_HEIGHT + 10;

    gfx_fill_rect(win->x + 1, win->y + TITLEBAR_HEIGHT + 1, win->width - 2, win->height - TITLEBAR_HEIGHT - 2, COLOR_RGB(245, 247, 250));

    /* Top navigation breadcrumb bar */
    gfx_fill_rect(win->x + 2, win->y + TITLEBAR_HEIGHT + 2, win->width - 4, 30, COLOR_RGB(230, 235, 242));
    gfx_draw_string(base_x, win->y + TITLEBAR_HEIGHT + 8, "Path: /", COLOR_RGB(40, 50, 60), COLOR_RGB(230, 235, 242));

    /* Left Sidebar: Places */
    gfx_fill_rect(win->x + 2, win->y + TITLEBAR_HEIGHT + 32, 130, win->height - TITLEBAR_HEIGHT - 34, COLOR_RGB(238, 241, 246));
    gfx_draw_string(base_x, base_y + 36, "Places", COLOR_RGB(100, 110, 125), COLOR_RGB(238, 241, 246));
    gfx_draw_string(base_x, base_y + 60, "> / (Root)", COLOR_RGB(20, 20, 20), COLOR_RGB(238, 241, 246));
    gfx_draw_string(base_x, base_y + 80, "  /system", COLOR_RGB(80, 90, 100), COLOR_RGB(238, 241, 246));
    gfx_draw_string(base_x, base_y + 100,"  /bin", COLOR_RGB(80, 90, 100), COLOR_RGB(238, 241, 246));
    gfx_draw_string(base_x, base_y + 120,"  /etc", COLOR_RGB(80, 90, 100), COLOR_RGB(238, 241, 246));
    gfx_draw_string(base_x, base_y + 140,"  /home", COLOR_RGB(80, 90, 100), COLOR_RGB(238, 241, 246));

    /* Divider */
    gfx_fill_rect(win->x + 132, win->y + TITLEBAR_HEIGHT + 32, 1, win->height - TITLEBAR_HEIGHT - 34, COLOR_RGB(215, 220, 230));

    /* File List header */
    int list_x = win->x + 144;
    gfx_draw_string(list_x, base_y + 36, "Name", COLOR_RGB(120, 130, 140), COLOR_RGB(245, 247, 250));
    gfx_draw_string(list_x + 180, base_y + 36, "Type", COLOR_RGB(120, 130, 140), COLOR_RGB(245, 247, 250));
    gfx_draw_string(list_x + 280, base_y + 36, "Size", COLOR_RGB(120, 130, 140), COLOR_RGB(245, 247, 250));

    /* Populate items from root VFS */
    vfs_node_t* root = vfs_get_root();
    if (root) {
        uint32_t idx = 0;
        vfs_dirent_t* ent;
        int row_y = base_y + 60;
        while ((ent = vfs_readdir(root, idx++)) != NULL && row_y < (int)(win->y + win->height - 20)) {
            bool is_d = (ent->type == FS_DIRECTORY);
            uint32_t icon_c = is_d ? COLOR_RGB(255, 193, 7) : COLOR_RGB(66, 165, 245);
            gfx_fill_rect(list_x, row_y + 2, 12, 12, icon_c);
            gfx_draw_string(list_x + 18, row_y, ent->name, COLOR_RGB(30, 30, 30), COLOR_RGB(245, 247, 250));
            gfx_draw_string(list_x + 180, row_y, is_d ? "Folder" : "File", COLOR_RGB(100, 100, 100), COLOR_RGB(245, 247, 250));

            char sz[32];
            ksprintf(sz, "%lu B", (unsigned long)ent->size);
            gfx_draw_string(list_x + 280, row_y, is_d ? "-" : sz, COLOR_RGB(100, 100, 100), COLOR_RGB(245, 247, 250));

            row_y += 22;
        }
    }
}

void app_task_manager_launch(void) {
    window_t* win = wm_create_window("MyTaskManager - System Monitor", 240, 100, 520, 380, WINDOW_FLAG_RESIZABLE);
    if (!win) return;
}

static void task_manager_render_content(window_t* win) {
    if (!win) return;
    int base_x = win->x + 14;
    int base_y = win->y + TITLEBAR_HEIGHT + 14;

    gfx_fill_rect(win->x + 1, win->y + TITLEBAR_HEIGHT + 1, win->width - 2, win->height - TITLEBAR_HEIGHT - 2, COLOR_RGB(250, 250, 252));

    uint32_t cpu_load = scheduler_get_cpu_usage();
    const pmm_stats_t* pmm = pmm_get_stats();

    /* CPU Usage Meter */
    gfx_draw_string(base_x, base_y, "CPU Utilization:", COLOR_RGB(30, 30, 30), COLOR_RGB(250, 250, 252));
    char cpu_str[32];
    ksprintf(cpu_str, "%u%%", cpu_load);
    gfx_draw_string(base_x + 150, base_y, cpu_str, COLOR_RGB(33, 150, 243), COLOR_RGB(250, 250, 252));

    /* CPU Progress Bar */
    gfx_draw_rect(base_x, base_y + 20, 480, 16, COLOR_RGB(200, 205, 215));
    uint32_t cpu_bar_w = (476 * cpu_load) / 100;
    gfx_fill_rect(base_x + 2, base_y + 22, cpu_bar_w, 12, COLOR_RGB(33, 150, 243));

    /* Memory Usage Meter */
    uint32_t ram_percent = (pmm->total_memory > 0) ? (uint32_t)((pmm->used_memory * 100) / pmm->total_memory) : 0;
    gfx_draw_string(base_x, base_y + 45, "Physical RAM:", COLOR_RGB(30, 30, 30), COLOR_RGB(250, 250, 252));
    char ram_str[48];
    ksprintf(ram_str, "%lu MiB / %lu MiB (%u%%)", pmm->used_memory / (1024 * 1024), pmm->total_memory / (1024 * 1024), ram_percent);
    gfx_draw_string(base_x + 150, base_y + 45, ram_str, COLOR_RGB(76, 175, 80), COLOR_RGB(250, 250, 252));

    /* RAM Progress Bar */
    gfx_draw_rect(base_x, base_y + 65, 480, 16, COLOR_RGB(200, 205, 215));
    uint32_t ram_bar_w = (476 * ram_percent) / 100;
    gfx_fill_rect(base_x + 2, base_y + 67, ram_bar_w, 12, COLOR_RGB(76, 175, 80));

    /* Process Table Header */
    int table_y = base_y + 95;
    gfx_fill_rect(base_x, table_y, 480, 24, COLOR_RGB(230, 235, 242));
    gfx_draw_string(base_x + 8, table_y + 5, "PID", COLOR_RGB(60, 70, 80), COLOR_RGB(230, 235, 242));
    gfx_draw_string(base_x + 60, table_y + 5, "Process Name", COLOR_RGB(60, 70, 80), COLOR_RGB(230, 235, 242));
    gfx_draw_string(base_x + 240, table_y + 5, "Status", COLOR_RGB(60, 70, 80), COLOR_RGB(230, 235, 242));
    gfx_draw_string(base_x + 360, table_y + 5, "Priority", COLOR_RGB(60, 70, 80), COLOR_RGB(230, 235, 242));

    /* Process Entries */
    uint32_t total = 0;
    const process_t* table = process_get_table(&total);
    int row_y = table_y + 28;
    for (uint32_t i = 0; i < total && row_y < (int)(win->y + win->height - 20); i++) {
        if (table[i].pid != 0 || i == 0) {
            char pid_buf[16];
            ksprintf(pid_buf, "%u", table[i].pid);
            gfx_draw_string(base_x + 8, row_y, pid_buf, COLOR_RGB(40, 40, 40), COLOR_RGB(250, 250, 252));
            gfx_draw_string(base_x + 60, row_y, table[i].name, COLOR_RGB(20, 20, 20), COLOR_RGB(250, 250, 252));

            const char* st = "READY";
            if (table[i].state == PROCESS_STATE_RUNNING) st = "RUNNING";
            gfx_draw_string(base_x + 240, row_y, st, COLOR_RGB(33, 150, 243), COLOR_RGB(250, 250, 252));

            char pri_buf[16];
            ksprintf(pri_buf, "%u", table[i].threads ? table[i].threads->priority : 0);
            gfx_draw_string(base_x + 360, row_y, pri_buf, COLOR_RGB(100, 100, 100), COLOR_RGB(250, 250, 252));

            row_y += 20;
        }
    }
}

void app_settings_launch(void) {
    window_t* win = wm_create_window("MySettings - Preferences", 200, 140, 480, 340, WINDOW_FLAG_RESIZABLE);
    if (!win) return;
}

static void settings_render_content(window_t* win) {
    if (!win) return;
    int base_x = win->x + 20;
    int base_y = win->y + TITLEBAR_HEIGHT + 20;

    gfx_fill_rect(win->x + 1, win->y + TITLEBAR_HEIGHT + 1, win->width - 2, win->height - TITLEBAR_HEIGHT - 2, COLOR_RGB(245, 247, 250));

    gfx_draw_string(base_x, base_y, "Display & Resolution:", COLOR_RGB(30, 30, 30), COLOR_RGB(245, 247, 250));
    gfx_draw_string(base_x + 20, base_y + 24, "> Resolution: 1024 x 768 @ 32 bpp (TrueColor)", COLOR_RGB(70, 70, 70), COLOR_RGB(245, 247, 250));
    gfx_draw_string(base_x + 20, base_y + 44, "> Double Buffering: Hardware / Software VESA", COLOR_RGB(70, 70, 70), COLOR_RGB(245, 247, 250));

    gfx_draw_string(base_x, base_y + 80, "Theme & Appearance:", COLOR_RGB(30, 30, 30), COLOR_RGB(245, 247, 250));
    gfx_draw_string(base_x + 20, base_y + 104, "> Modern Neutral Slate (Default)", COLOR_RGB(70, 70, 70), COLOR_RGB(245, 247, 250));
    gfx_draw_string(base_x + 20, base_y + 124, "> Window Borders: Smooth High Contrast", COLOR_RGB(70, 70, 70), COLOR_RGB(245, 247, 250));

    gfx_draw_string(base_x, base_y + 160, "Input Devices:", COLOR_RGB(30, 30, 30), COLOR_RGB(245, 247, 250));
    gfx_draw_string(base_x + 20, base_y + 184, "> Keyboard: Standard PS/2 (US QWERTY)", COLOR_RGB(70, 70, 70), COLOR_RGB(245, 247, 250));
    gfx_draw_string(base_x + 20, base_y + 204, "> Pointer: PS/2 3-Button Mouse with Acceleration", COLOR_RGB(70, 70, 70), COLOR_RGB(245, 247, 250));
}

void app_system_info_launch(void) {
    window_t* win = wm_create_window("MySystem - System Information", 220, 110, 480, 360, WINDOW_FLAG_RESIZABLE);
    if (!win) return;
}

static void system_info_render_content(window_t* win) {
    if (!win) return;
    int base_x = win->x + 20;
    int base_y = win->y + TITLEBAR_HEIGHT + 20;

    gfx_fill_rect(win->x + 1, win->y + TITLEBAR_HEIGHT + 1, win->width - 2, win->height - TITLEBAR_HEIGHT - 2, COLOR_RGB(250, 252, 255));

    /* Logo Banner */
    gfx_fill_rect(base_x, base_y, 440, 45, COLOR_RGB(33, 150, 243));
    gfx_draw_string(base_x + 16, base_y + 14, "MyOS Operating System 0.1.0", COLOR_WHITE, COLOR_RGB(33, 150, 243));

    const cpu_info_t* cpu = cpu_get_info();
    const pmm_stats_t* pmm = pmm_get_stats();

    int dy = base_y + 60;
    gfx_draw_string(base_x, dy, "OS Version:          MyOS 0.1.0 x86-64 Release", COLOR_RGB(30, 30, 30), COLOR_RGB(250, 252, 255));
    dy += 24;
    gfx_draw_string(base_x, dy, "Architecture:        x86-64 (Long Mode)", COLOR_RGB(30, 30, 30), COLOR_RGB(250, 252, 255));
    dy += 24;
    gfx_draw_string(base_x, dy, "Processor Vendor:    ", COLOR_RGB(30, 30, 30), COLOR_RGB(250, 252, 255));
    gfx_draw_string(base_x + 180, dy, cpu->vendor, COLOR_RGB(33, 150, 243), COLOR_RGB(250, 252, 255));
    dy += 24;
    gfx_draw_string(base_x, dy, "Processor Model:     ", COLOR_RGB(30, 30, 30), COLOR_RGB(250, 252, 255));
    gfx_draw_string(base_x + 180, dy, cpu->brand, COLOR_RGB(30, 30, 30), COLOR_RGB(250, 252, 255));
    dy += 24;
    char cores_s[32];
    ksprintf(cores_s, "%u Logical Core(s)", cpu->cores_detected);
    gfx_draw_string(base_x, dy, "CPU Cores:           ", COLOR_RGB(30, 30, 30), COLOR_RGB(250, 252, 255));
    gfx_draw_string(base_x + 180, dy, cores_s, COLOR_RGB(30, 30, 30), COLOR_RGB(250, 252, 255));
    dy += 24;
    char mem_s[32];
    ksprintf(mem_s, "%lu MiB Physical RAM", pmm->total_memory / (1024 * 1024));
    gfx_draw_string(base_x, dy, "Installed Memory:    ", COLOR_RGB(30, 30, 30), COLOR_RGB(250, 252, 255));
    gfx_draw_string(base_x + 180, dy, mem_s, COLOR_RGB(76, 175, 80), COLOR_RGB(250, 252, 255));
    dy += 24;
    char pci_s[32];
    ksprintf(pci_s, "%u Devices Discovered", pci_get_device_count());
    gfx_draw_string(base_x, dy, "PCI Devices:         ", COLOR_RGB(30, 30, 30), COLOR_RGB(250, 252, 255));
    gfx_draw_string(base_x + 180, dy, pci_s, COLOR_RGB(30, 30, 30), COLOR_RGB(250, 252, 255));
}

void desktop_init(void) {
    wm_init();

    /* Automatically launch MyTerminal and MySystem windows at startup */
    app_terminal_launch();
    app_system_info_launch();

    g_desktop_running = true;
    LOG_INFO("DESKTOP", "MyOS Desktop Environment loaded");
}

static void draw_taskbar(uint32_t screen_w, uint32_t screen_h) {
    uint32_t bar_y = screen_h - TASKBAR_HEIGHT;

    /* Taskbar background with modern dark neutral tone */
    gfx_fill_rect(0, bar_y, screen_w, TASKBAR_HEIGHT, COLOR_TASKBAR_BG);
    gfx_draw_line(0, bar_y, screen_w, bar_y, COLOR_RGB(50, 56, 68));

    /* Start Button */
    uint32_t start_c = g_start_menu_open ? COLOR_RGB(33, 150, 243) : COLOR_START_BTN;
    gfx_fill_rect(6, bar_y + 4, 80, TASKBAR_HEIGHT - 8, start_c);
    gfx_draw_string(18, bar_y + 10, "MyOS", COLOR_WHITE, start_c);

    /* Clock on the right side */
    rtc_time_t t;
    hal_get_time(&t);
    char time_str[32];
    ksprintf(time_str, "%02u:%02u:%02u", t.hour, t.minute, t.second);
    gfx_draw_string(screen_w - 90, bar_y + 10, time_str, COLOR_WHITE, COLOR_TASKBAR_BG);

    /* System Status indicator */
    gfx_draw_string(screen_w - 180, bar_y + 10, "[Online]", COLOR_RGB(76, 175, 80), COLOR_TASKBAR_BG);
}

static void draw_start_menu(uint32_t screen_h) {
    if (!g_start_menu_open) return;

    uint32_t menu_w = 220;
    uint32_t menu_h = 240;
    uint32_t menu_x = 6;
    uint32_t menu_y = screen_h - TASKBAR_HEIGHT - menu_h - 6;

    /* Shadow & Border */
    gfx_fill_rect(menu_x + 3, menu_y + 3, menu_w, menu_h, COLOR_RGB(15, 20, 30));
    gfx_fill_rect(menu_x, menu_y, menu_w, menu_h, COLOR_START_MENU_BG);
    gfx_draw_rect(menu_x, menu_y, menu_w, menu_h, COLOR_RGB(50, 60, 75));

    /* Header */
    gfx_fill_rect(menu_x + 1, menu_y + 1, menu_w - 2, 36, COLOR_RGB(33, 150, 243));
    gfx_draw_string(menu_x + 14, menu_y + 10, "MyOS Applications", COLOR_WHITE, COLOR_RGB(33, 150, 243));

    /* Menu Items */
    const char* items[] = {
        "1. MyTerminal",
        "2. MyFiles",
        "3. MyTaskManager",
        "4. MySettings",
        "5. MySystem",
        "6. Restart System",
        "7. Power Off"
    };

    for (int i = 0; i < 7; i++) {
        int iy = menu_y + 45 + (i * 26);
        gfx_draw_string(menu_x + 16, iy, items[i], COLOR_WHITE, COLOR_START_MENU_BG);
    }
}

static void draw_desktop_icons(void) {
    for (int i = 0; i < DESKTOP_ICON_COUNT; i++) {
        desktop_icon_t* icon = &g_desktop_icons[i];

        /* Icon tile */
        gfx_fill_rect(icon->x, icon->y, icon->w, icon->h, icon->color);
        gfx_draw_rect(icon->x, icon->y, icon->w, icon->h, COLOR_RGB(255, 255, 255));

        /* Inner graphic accent */
        gfx_fill_rect(icon->x + 8, icon->y + 8, icon->w - 16, icon->h - 16, COLOR_RGB(255, 255, 255));
        gfx_fill_rect(icon->x + 12, icon->y + 12, icon->w - 24, icon->h - 24, icon->color);

        /* Icon label */
        gfx_draw_string(icon->x - 6, icon->y + icon->h + 6, icon->label, COLOR_WHITE, 0);
    }
}

static void draw_mouse_cursor(int32_t x, int32_t y) {
    /* 12x18 Arrow mouse cursor */
    static const uint8_t cursor_mask[18] = {
        0b10000000,
        0b11000000,
        0b11100000,
        0b11110000,
        0b11111000,
        0b11111100,
        0b11111110,
        0b11111111,
        0b11111110,
        0b11111000,
        0b11011000,
        0b10001100,
        0b00001100,
        0b00000110,
        0b00000110,
        0b00000000,
        0b00000000,
        0b00000000
    };

    for (int cy = 0; cy < 18; cy++) {
        uint8_t row = cursor_mask[cy];
        for (int cx = 0; cx < 8; cx++) {
            if (row & (0x80 >> cx)) {
                gfx_putpixel(x + cx, y + cy, COLOR_WHITE);
            }
        }
    }
}

void desktop_render(void) {
    framebuffer_t* fb = gfx_get_framebuffer();
    if (!fb || !fb->width) return;

    /* 1. Desktop Background */
    gfx_clear(COLOR_DESKTOP_BG);

    /* 2. Desktop Icons */
    draw_desktop_icons();

    /* 3. Render Windows */
    wm_render_all();

    /* 4. Render Active Window Specialized Content */
    window_t* focused = wm_get_focused_window();
    if (focused) {
        if (kstrcmp(focused->title, "MyTerminal - MyShell") == 0) {
            term_render_content(focused);
        } else if (kstrcmp(focused->title, "MyFiles - File Manager") == 0) {
            file_manager_render_content(focused);
        } else if (kstrcmp(focused->title, "MyTaskManager - System Monitor") == 0) {
            task_manager_render_content(focused);
        } else if (kstrcmp(focused->title, "MySettings - Preferences") == 0) {
            settings_render_content(focused);
        } else if (kstrcmp(focused->title, "MySystem - System Information") == 0) {
            system_info_render_content(focused);
        }
    }

    /* 5. Taskbar */
    draw_taskbar(fb->width, fb->height);

    /* 6. Start Menu */
    draw_start_menu(fb->height);

    /* 7. Mouse Cursor */
    int32_t mx, my;
    uint8_t mb;
    mouse_get_position(&mx, &my, &mb);
    draw_mouse_cursor(mx, my);

    /* 8. Blit to hardware screen */
    gfx_swap_buffers();
}

void desktop_handle_input(input_event_t* ev) {
    if (!ev) return;
    framebuffer_t* fb = gfx_get_framebuffer();
    uint32_t screen_h = fb ? fb->height : 768;

    if (ev->type == INPUT_EVENT_MOUSE_DOWN) {
        int32_t mx = ev->mouse.x;
        int32_t my = ev->mouse.y;

        /* Check Start Button */
        if (my >= (int32_t)(screen_h - TASKBAR_HEIGHT) && mx <= 90) {
            g_start_menu_open = !g_start_menu_open;
            return;
        }

        /* Check Start Menu Item clicks */
        if (g_start_menu_open && mx >= 6 && mx <= 226) {
            int menu_y = screen_h - TASKBAR_HEIGHT - 240 - 6;
            if (my >= menu_y + 40 && my <= menu_y + 240) {
                int item = (my - (menu_y + 40)) / 26;
                switch (item) {
                    case 0: app_terminal_launch(); break;
                    case 1: app_file_manager_launch(); break;
                    case 2: app_task_manager_launch(); break;
                    case 3: app_settings_launch(); break;
                    case 4: app_system_info_launch(); break;
                    case 5: hal_reboot(); break;
                    case 6: hal_shutdown(); break;
                }
                g_start_menu_open = false;
                return;
            }
        }

        /* Check Desktop Icon clicks */
        for (int i = 0; i < DESKTOP_ICON_COUNT; i++) {
            desktop_icon_t* icon = &g_desktop_icons[i];
            if (mx >= icon->x && mx <= (int32_t)(icon->x + icon->w) &&
                my >= icon->y && my <= (int32_t)(icon->y + icon->h + 20)) {
                switch (icon->app_id) {
                    case APP_TERMINAL:     app_terminal_launch(); break;
                    case APP_FILE_MANAGER: app_file_manager_launch(); break;
                    case APP_TASK_MANAGER: app_task_manager_launch(); break;
                    case APP_SETTINGS:     app_settings_launch(); break;
                    case APP_SYSTEM_INFO:  app_system_info_launch(); break;
                }
                g_start_menu_open = false;
                return;
            }
        }

        /* Check Window clicks */
        window_t* clicked_win = wm_get_window_at(mx, my);
        if (clicked_win) {
            wm_focus_window(clicked_win);

            /* Check close button */
            int btn_close_x = clicked_win->x + clicked_win->width - 24;
            if (mx >= btn_close_x && mx <= btn_close_x + 16 &&
                my >= clicked_win->y + 6 && my <= clicked_win->y + 22) {
                wm_destroy_window(clicked_win);
                return;
            }

            /* Title bar drag */
            if (my >= clicked_win->y && my <= clicked_win->y + TITLEBAR_HEIGHT) {
                g_drag_window = clicked_win;
                g_drag_offset_x = mx - clicked_win->x;
                g_drag_offset_y = my - clicked_win->y;
            }
        } else {
            g_start_menu_open = false;
        }
    } else if (ev->type == INPUT_EVENT_MOUSE_UP) {
        g_drag_window = NULL;
    } else if (ev->type == INPUT_EVENT_MOUSE_MOVE) {
        if (g_drag_window) {
            g_drag_window->x = ev->mouse.x - g_drag_offset_x;
            g_drag_window->y = ev->mouse.y - g_drag_offset_y;
        }
    } else if (ev->type == INPUT_EVENT_KEY_DOWN) {
        /* Forward keyboard input to focused window (e.g. MyTerminal) */
        window_t* win = wm_get_focused_window();
        if (win && kstrcmp(win->title, "MyTerminal - MyShell") == 0) {
            char ch = ev->key.character;
            if (ch == '\n' || ch == '\r') {
                /* Enter command */
                char prompt_line[64];
                ksprintf(prompt_line, "$ %s", g_term_input);
                term_add_line(prompt_line);

                if (g_term_input_len > 0) {
                    shell_execute(g_term_input);
                }

                g_term_input_len = 0;
                g_term_input[0] = '\0';
            } else if (ch == '\b') {
                if (g_term_input_len > 0) {
                    g_term_input[--g_term_input_len] = '\0';
                }
            } else if (ch >= ' ' && ch <= '~') {
                if (g_term_input_len < TERM_COLS - 4) {
                    g_term_input[g_term_input_len++] = ch;
                    g_term_input[g_term_input_len] = '\0';
                }
            }
        }
    }
}
