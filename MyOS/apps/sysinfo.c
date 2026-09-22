#include <myos/desktop.h>
#include <myos/window.h>
#include <myos/gfx.h>
#include <myos/kernel.h>
#include <myos/cpu.h>
#include <myos/pmm.h>
#include <myos/pci.h>

static window_t* s_sys_window = NULL;

static void sys_render_content(window_t* win) {
    int32_t cx = win->x;
    int32_t cy = win->y + 28;

    gfx_fill_rect(cx, cy, win->width, win->height, 0xFFF7FAFC);

    /* Section: OS Information */
    gfx_fill_rect(cx + 12, cy + 12, win->width - 24, 70, COLOR_WHITE);
    gfx_draw_rect(cx + 12, cy + 12, win->width - 24, 70, COLOR_GRAY);
    gfx_draw_string(cx + 20, cy + 18, "MyOS System Release", COLOR_NAVY, COLOR_WHITE);
    gfx_draw_string(cx + 20, cy + 38, "OS Name:      MyOS 0.1.0-alpha (x86_64)", COLOR_BLACK, COLOR_WHITE);
    gfx_draw_string(cx + 20, cy + 56, "Kernel Type:  Monolithic 64-bit Preemptive Kernel", COLOR_DARK_GRAY, COLOR_WHITE);

    /* Section: CPU Information */
    const cpu_info_t* cpu = cpu_get_info();
    gfx_fill_rect(cx + 12, cy + 90, win->width - 24, 90, COLOR_WHITE);
    gfx_draw_rect(cx + 12, cy + 90, win->width - 24, 90, COLOR_GRAY);
    gfx_draw_string(cx + 20, cy + 96, "Processor (CPU)", COLOR_NAVY, COLOR_WHITE);

    char buf[128];
    ksnprintf(buf, sizeof(buf), "Vendor:       %s", cpu->vendor);
    gfx_draw_string(cx + 20, cy + 116, buf, COLOR_BLACK, COLOR_WHITE);

    ksnprintf(buf, sizeof(buf), "Model:        %s", cpu->brand);
    gfx_draw_string(cx + 20, cy + 136, buf, COLOR_BLACK, COLOR_WHITE);

    ksnprintf(buf, sizeof(buf), "Features:     SSE=%d  SSE2=%d  AVX=%d  APIC=%d  NX=%d",
              cpu->has_sse, cpu->has_sse2, cpu->has_avx, cpu->has_apic, cpu->has_nx);
    gfx_draw_string(cx + 20, cy + 156, buf, COLOR_DARK_GRAY, COLOR_WHITE);

    /* Section: Memory Information */
    const pmm_stats_t* pmm = pmm_get_stats();
    gfx_fill_rect(cx + 12, cy + 190, win->width - 24, 90, COLOR_WHITE);
    gfx_draw_rect(cx + 12, cy + 190, win->width - 24, 90, COLOR_GRAY);
    gfx_draw_string(cx + 20, cy + 196, "Physical Memory (RAM)", COLOR_NAVY, COLOR_WHITE);

    ksnprintf(buf, sizeof(buf), "Total: %llu MB    Used: %llu MB    Free: %llu MB",
              pmm->total_memory / (1024*1024), pmm->used_memory / (1024*1024), pmm->free_memory / (1024*1024));
    gfx_draw_string(cx + 20, cy + 216, buf, COLOR_BLACK, COLOR_WHITE);

    /* Memory progress bar */
    int32_t bar_x = cx + 20;
    int32_t bar_y = cy + 242;
    int32_t bar_w = win->width - 64;
    int32_t bar_h = 18;
    gfx_fill_rect(bar_x, bar_y, bar_w, bar_h, 0xFFE2E8F0);
    gfx_draw_rect(bar_x, bar_y, bar_w, bar_h, COLOR_DARK_GRAY);
    uint32_t used_w = (uint32_t)((pmm->used_memory * bar_w) / (pmm->total_memory ? pmm->total_memory : 1));
    if (used_w > (uint32_t)bar_w) used_w = bar_w;
    gfx_fill_rect(bar_x + 1, bar_y + 1, used_w, bar_h - 2, 0xFF3182CE);

    /* Section: PCI Hardware Summary */
    gfx_fill_rect(cx + 12, cy + 290, win->width - 24, 60, COLOR_WHITE);
    gfx_draw_rect(cx + 12, cy + 290, win->width - 24, 60, COLOR_GRAY);
    gfx_draw_string(cx + 20, cy + 296, "PCI Hardware Bus", COLOR_NAVY, COLOR_WHITE);
    ksnprintf(buf, sizeof(buf), "Detected Devices: %u controllers active on bus", pci_get_device_count());
    gfx_draw_string(cx + 20, cy + 318, buf, COLOR_BLACK, COLOR_WHITE);
}

void app_sysinfo_open(void) {
    if (s_sys_window != NULL) {
        wm_bring_to_front(s_sys_window);
        return;
    }

    s_sys_window = wm_create_window("MySystem - System & Hardware Information", 100, 80, 520, 390, 0);
    if (s_sys_window) {
        s_sys_window->render_content = sys_render_content;
    }
}
