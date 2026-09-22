#include <myos/desktop.h>
#include <myos/window.h>
#include <myos/gfx.h>
#include <myos/kernel.h>
#include <myos/process.h>
#include <myos/pmm.h>
#include <myos/kheap.h>

static window_t* s_tm_window = NULL;

static void tm_render_content(window_t* win) {
    int32_t cx = win->x;
    int32_t cy = win->y + 28;

    /* Background */
    gfx_fill_rect(cx, cy, win->width, win->height, COLOR_WHITE);

    /* Header summary stats */
    gfx_fill_rect(cx, cy, win->width, 42, 0xFFEDF2F7);
    const pmm_stats_t* pmm = pmm_get_stats();
    const kheap_stats_t* heap = kheap_get_stats();

    char stat_buf[128];
    ksnprintf(stat_buf, sizeof(stat_buf), "Processes: %u   RAM Used: %llu MB / %llu MB   Heap: %llu KB",
              process_get_count(), pmm->used_memory / (1024*1024), pmm->total_memory / (1024*1024),
              heap->used_heap / 1024);
    gfx_draw_string(cx + 12, cy + 12, stat_buf, COLOR_NAVY, 0xFFEDF2F7);

    /* Process Table Header */
    int32_t th_y = cy + 42;
    gfx_fill_rect(cx, th_y, win->width, 24, 0xFFCBD5E0);
    gfx_draw_string(cx + 12, th_y + 4, "PID", COLOR_BLACK, 0xFFCBD5E0);
    gfx_draw_string(cx + 64, th_y + 4, "Process Name", COLOR_BLACK, 0xFFCBD5E0);
    gfx_draw_string(cx + 260, th_y + 4, "State", COLOR_BLACK, 0xFFCBD5E0);
    gfx_draw_string(cx + 360, th_y + 4, "CPU Ticks", COLOR_BLACK, 0xFFCBD5E0);

    /* Process List */
    int32_t row_y = th_y + 26;
    for (uint32_t i = 0; i < MAX_PROCESSES; i++) {
        const process_t* pr = process_get_at(i);
        if (!pr || pr->state == PROCESS_STATE_UNUSED) continue;

        if (i % 2 == 1) {
            gfx_fill_rect(cx, row_y, win->width, 22, 0xFFF7FAFC);
        }

        char pid_buf[16];
        ksnprintf(pid_buf, sizeof(pid_buf), "%u", pr->pid);
        gfx_draw_string(cx + 12, row_y + 3, pid_buf, COLOR_BLACK, 0);
        gfx_draw_string(cx + 64, row_y + 3, pr->name, COLOR_BLACK, 0);

        const char* st = (pr->state == PROCESS_STATE_RUNNING) ? "Running" :
                         (pr->state == PROCESS_STATE_READY)   ? "Ready" : "Waiting";
        color_t st_color = (pr->state == PROCESS_STATE_RUNNING) ? COLOR_GREEN : COLOR_DARK_GRAY;
        gfx_draw_string(cx + 260, row_y + 3, st, st_color, 0);

        char ticks_buf[32];
        uint64_t ticks = pr->main_thread ? pr->main_thread->ticks_executed : 0;
        ksnprintf(ticks_buf, sizeof(ticks_buf), "%llu", ticks);
        gfx_draw_string(cx + 360, row_y + 3, ticks_buf, COLOR_DARK_GRAY, 0);

        row_y += 24;
    }
}

void app_taskmanager_open(void) {
    if (s_tm_window != NULL) {
        wm_bring_to_front(s_tm_window);
        return;
    }

    s_tm_window = wm_create_window("MyTaskManager - Resource Monitor", 180, 140, 480, 320, 0);
    if (s_tm_window) {
        s_tm_window->render_content = tm_render_content;
    }
}
