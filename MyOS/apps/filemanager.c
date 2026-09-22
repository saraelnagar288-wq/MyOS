#include <myos/desktop.h>
#include <myos/window.h>
#include <myos/gfx.h>
#include <myos/kernel.h>
#include <myos/kheap.h>
#include <myos/fs.h>

static window_t* s_fm_window = NULL;
static char s_current_dir[VFS_MAX_PATH] = "/";

static void fm_render_content(window_t* win) {
    int32_t cx = win->x;
    int32_t cy = win->y + 28;

    /* Background */
    gfx_fill_rect(cx, cy, win->width, win->height, 0xFFF5F7FA);

    /* Breadcrumbs Toolbar */
    gfx_fill_rect(cx, cy, win->width, 36, 0xFFE2E8F0);
    gfx_draw_line(cx, cy + 36, cx + win->width, cy + 36, COLOR_GRAY);

    gfx_draw_string(cx + 12, cy + 10, "Location:", COLOR_DARK_GRAY, 0xFFE2E8F0);
    gfx_fill_rect(cx + 90, cy + 6, win->width - 110, 24, COLOR_WHITE);
    gfx_draw_rect(cx + 90, cy + 6, win->width - 110, 24, COLOR_GRAY);
    gfx_draw_string(cx + 98, cy + 10, s_current_dir, COLOR_BLACK, COLOR_WHITE);

    /* Directory table header */
    int32_t th_y = cy + 40;
    gfx_fill_rect(cx, th_y, win->width, 24, 0xFFEDF2F7);
    gfx_draw_string(cx + 16, th_y + 4, "Name", COLOR_DARK_GRAY, 0xFFEDF2F7);
    gfx_draw_string(cx + 260, th_y + 4, "Type", COLOR_DARK_GRAY, 0xFFEDF2F7);
    gfx_draw_string(cx + 380, th_y + 4, "Size", COLOR_DARK_GRAY, 0xFFEDF2F7);
    gfx_draw_line(cx, th_y + 24, cx + win->width, th_y + 24, COLOR_GRAY);

    /* List directory entries */
    vfs_node_t* node = vfs_lookup(s_current_dir);
    if (!node) return;

    int32_t row_y = th_y + 28;
    for (uint32_t i = 0; i < 12; i++) {
        vfs_dirent_t* ent = vfs_readdir(node, i);
        if (!ent) break;

        /* Row zebra striping */
        if (i % 2 == 1) {
            gfx_fill_rect(cx, row_y, win->width, 22, 0xFFF8FAFC);
        }

        bool is_dir = (ent->type == FS_DIRECTORY);

        /* Icon tag */
        if (is_dir) {
            gfx_fill_rect(cx + 16, row_y + 2, 18, 16, 0xFFF6AD55); /* Folder gold */
            gfx_draw_char(cx + 20, row_y + 2, 'D', COLOR_WHITE, 0xFFF6AD55);
        } else {
            gfx_fill_rect(cx + 16, row_y + 2, 18, 16, 0xFF4299E1); /* File blue */
            gfx_draw_char(cx + 20, row_y + 2, 'F', COLOR_WHITE, 0xFF4299E1);
        }

        gfx_draw_string(cx + 42, row_y + 3, ent->name, COLOR_BLACK, 0);
        gfx_draw_string(cx + 260, row_y + 3, is_dir ? "Directory" : "Document", COLOR_DARK_GRAY, 0);

        char sz_buf[32];
        if (is_dir) {
            ksnprintf(sz_buf, sizeof(sz_buf), "-");
        } else {
            ksnprintf(sz_buf, sizeof(sz_buf), "%u B", ent->size);
        }
        gfx_draw_string(cx + 380, row_y + 3, sz_buf, COLOR_DARK_GRAY, 0);

        row_y += 24;
    }
}

void app_filemanager_open(void) {
    if (s_fm_window != NULL) {
        wm_bring_to_front(s_fm_window);
        return;
    }

    s_fm_window = wm_create_window("MyFiles - File Manager", 120, 100, 500, 360, 0);
    if (s_fm_window) {
        s_fm_window->render_content = fm_render_content;
    }
}
