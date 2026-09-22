#include <myos/desktop.h>
#include <myos/window.h>
#include <myos/gfx.h>
#include <myos/kernel.h>
#include <myos/kheap.h>
#include <myos/fs.h>
#include <myos/cpu.h>
#include <myos/pmm.h>
#include <myos/pci.h>
#include <myos/process.h>
#include <myos/hal.h>

#define TERM_ROWS 24
#define TERM_COLS 60

typedef struct {
    char lines[TERM_ROWS][TERM_COLS + 1];
    char input_line[TERM_COLS + 1];
    uint32_t input_len;
    uint32_t current_row;
} terminal_state_t;

static terminal_state_t s_term;
static window_t* s_term_window = NULL;

static void term_scroll(void) {
    for (int r = 0; r < TERM_ROWS - 1; r++) {
        kstrcpy(s_term.lines[r], s_term.lines[r + 1]);
    }
    s_term.lines[TERM_ROWS - 1][0] = '\0';
    s_term.current_row = TERM_ROWS - 1;
}

static void term_print(const char* str) {
    while (*str) {
        if (*str == '\n') {
            if (s_term.current_row >= TERM_ROWS - 1) {
                term_scroll();
            } else {
                s_term.current_row++;
            }
            s_term.lines[s_term.current_row][0] = '\0';
        } else {
            size_t len = kstrlen(s_term.lines[s_term.current_row]);
            if (len < TERM_COLS) {
                s_term.lines[s_term.current_row][len] = *str;
                s_term.lines[s_term.current_row][len + 1] = '\0';
            } else {
                if (s_term.current_row >= TERM_ROWS - 1) {
                    term_scroll();
                } else {
                    s_term.current_row++;
                }
                s_term.lines[s_term.current_row][0] = *str;
                s_term.lines[s_term.current_row][1] = '\0';
            }
        }
        str++;
    }
}

static void term_execute(const char* cmd) {
    if (kstrlen(cmd) == 0) return;

    if (kstrcmp(cmd, "help") == 0) {
        term_print("MyOS MyShell Commands:\n");
        term_print("  help      - List available commands\n");
        term_print("  clear     - Clear terminal screen\n");
        term_print("  uname     - Display OS system release\n");
        term_print("  cpuinfo   - Inspect CPU features & vendor\n");
        term_print("  meminfo   - Display RAM usage statistics\n");
        term_print("  pci       - Enumerate detected PCI devices\n");
        term_print("  ps        - List running kernel processes\n");
        term_print("  uptime    - Display system uptime\n");
        term_print("  date      - Current RTC date & time\n");
        term_print("  ls        - List root files and directories\n");
        term_print("  cat <file>- Print file contents (e.g. cat /etc/os-release)\n");
        term_print("  reboot    - Reboot the system\n");
        term_print("  shutdown  - Power off computer\n");
    } else if (kstrcmp(cmd, "clear") == 0) {
        for (int r = 0; r < TERM_ROWS; r++) {
            s_term.lines[r][0] = '\0';
        }
        s_term.current_row = 0;
    } else if (kstrcmp(cmd, "uname") == 0) {
        term_print("MyOS 0.1.0-alpha x86_64 SMP (MyOS Kernel monolithic)\n");
    } else if (kstrcmp(cmd, "cpuinfo") == 0) {
        const cpu_info_t* c = cpu_get_info();
        term_print("Vendor: "); term_print(c->vendor); term_print("\n");
        term_print("Model:  "); term_print(c->brand); term_print("\n");
        term_print("Cores:  1 Detected, x86-64 Long Mode Active\n");
    } else if (kstrcmp(cmd, "meminfo") == 0) {
        const pmm_stats_t* p = pmm_get_stats();
        const kheap_stats_t* h = kheap_get_stats();
        char buf[64];
        ksnprintf(buf, sizeof(buf), "RAM: Total=%lluMB, Free=%lluMB, Used=%lluMB\n",
                  p->total_memory / (1024*1024), p->free_memory / (1024*1024), p->used_memory / (1024*1024));
        term_print(buf);
        ksnprintf(buf, sizeof(buf), "Heap: Total=%lluKB, Used=%lluKB\n",
                  h->total_heap / 1024, h->used_heap / 1024);
        term_print(buf);
    } else if (kstrcmp(cmd, "uptime") == 0) {
        uint64_t sec = hal_get_uptime_ms() / 1000;
        char buf[64];
        ksnprintf(buf, sizeof(buf), "Uptime: %llu seconds (%llu minutes)\n", sec, sec / 60);
        term_print(buf);
    } else if (kstrcmp(cmd, "date") == 0) {
        rtc_time_t t;
        hal_get_time(&t);
        char buf[64];
        ksnprintf(buf, sizeof(buf), "Date: %04u-%02u-%02u %02u:%02u:%02u UTC\n",
                  t.year, t.month, t.day, t.hour, t.minute, t.second);
        term_print(buf);
    } else if (kstrcmp(cmd, "pci") == 0) {
        uint32_t count = pci_get_device_count();
        char buf[80];
        ksnprintf(buf, sizeof(buf), "PCI Bus Scan: %u devices detected\n", count);
        term_print(buf);
        for (uint32_t i = 0; i < count && i < 6; i++) {
            const pci_device_t* d = pci_get_device(i);
            ksnprintf(buf, sizeof(buf), "[%02x:%02x] %s [0x%04x:0x%04x]\n",
                      d->bus, d->device, pci_class_to_string(d->class_id, d->subclass_id),
                      d->vendor_id, d->device_id);
            term_print(buf);
        }
    } else if (kstrcmp(cmd, "ps") == 0) {
        uint32_t count = process_get_count();
        char buf[80];
        ksnprintf(buf, sizeof(buf), "PID  STATE    NAME\n");
        term_print(buf);
        for (uint32_t i = 0; i < MAX_PROCESSES; i++) {
            const process_t* pr = process_get_at(i);
            if (pr && pr->state != PROCESS_STATE_UNUSED) {
                const char* st = (pr->state == PROCESS_STATE_RUNNING) ? "RUNNING" :
                                 (pr->state == PROCESS_STATE_READY)   ? "READY  " : "WAIT   ";
                ksnprintf(buf, sizeof(buf), "%-4u %s %s\n", pr->pid, st, pr->name);
                term_print(buf);
            }
        }
    } else if (kstrcmp(cmd, "ls") == 0) {
        vfs_node_t* root = vfs_get_root();
        char buf[80];
        for (uint32_t i = 0; ; i++) {
            vfs_dirent_t* d = vfs_readdir(root, i);
            if (!d) break;
            ksnprintf(buf, sizeof(buf), "%s%s  ", d->name, (d->type == FS_DIRECTORY) ? "/" : "");
            term_print(buf);
        }
        term_print("\n");
    } else if (kstrncmp(cmd, "cat ", 4) == 0) {
        const char* path = cmd + 4;
        vfs_node_t* file = vfs_lookup(path);
        if (file && !(file->flags & FS_DIRECTORY)) {
            uint8_t buffer[256];
            uint32_t bytes = vfs_read(file, 0, sizeof(buffer) - 1, buffer);
            buffer[bytes] = '\0';
            term_print((const char*)buffer);
            if (bytes > 0 && buffer[bytes-1] != '\n') term_print("\n");
        } else {
            term_print("File not found: "); term_print(path); term_print("\n");
        }
    } else if (kstrcmp(cmd, "reboot") == 0) {
        hal_reboot();
    } else if (kstrcmp(cmd, "shutdown") == 0) {
        hal_shutdown();
    } else {
        term_print("mysh: command not found: ");
        term_print(cmd);
        term_print(" (type 'help' for commands)\n");
    }
}

static void term_render_content(window_t* win) {
    int32_t cx = win->x;
    int32_t cy = win->y + 28;
    gfx_fill_rect(cx, cy, win->width, win->height, 0xFF1E1E1E);

    /* Render terminal text lines */
    for (int r = 0; r < TERM_ROWS; r++) {
        if (s_term.lines[r][0] != '\0') {
            gfx_draw_string(cx + 8, cy + 6 + (r * 16), s_term.lines[r], COLOR_WHITE, 0xFF1E1E1E);
        }
    }

    /* Render current prompt */
    int32_t prompt_y = cy + 6 + (s_term.current_row * 16);
    gfx_draw_string(cx + 8, prompt_y, "root@myos:~# ", COLOR_GREEN, 0xFF1E1E1E);
    gfx_draw_string(cx + 8 + (13 * 8), prompt_y, s_term.input_line, COLOR_WHITE, 0xFF1E1E1E);

    /* Blinking cursor */
    int32_t cur_x = cx + 8 + ((13 + s_term.input_len) * 8);
    if ((hal_get_uptime_ms() / 500) % 2 == 0) {
        gfx_fill_rect(cur_x, prompt_y, 8, 14, COLOR_WHITE);
    }
}

void app_terminal_open(void) {
    if (s_term_window != NULL) {
        wm_bring_to_front(s_term_window);
        return;
    }

    kmemset(&s_term, 0, sizeof(s_term));
    term_print("MyOS Interactive Terminal (MyShell v0.1)\nType 'help' to see list of commands.\n\n");

    s_term_window = wm_create_window("MyTerminal - /bin/mysh", 60, 60, 520, 420, 0);
    if (s_term_window) {
        s_term_window->render_content = term_render_content;
    }
}

void app_terminal_handle_key(char c) {
    if (!s_term_window || wm_get_focused() != s_term_window) return;

    if (c == '\n' || c == '\r') {
        s_term.input_line[s_term.input_len] = '\0';
        char cmd[TERM_COLS + 1];
        kstrcpy(cmd, s_term.input_line);

        /* Print prompt + command to line buffer */
        char full_line[TERM_COLS + 32];
        ksnprintf(full_line, sizeof(full_line), "root@myos:~# %s\n", cmd);
        term_print(full_line);

        s_term.input_len = 0;
        s_term.input_line[0] = '\0';

        term_execute(cmd);
    } else if (c == '\b') {
        if (s_term.input_len > 0) {
            s_term.input_len--;
            s_term.input_line[s_term.input_len] = '\0';
        }
    } else if (c >= ' ' && c <= '~') {
        if (s_term.input_len < TERM_COLS - 15) {
            s_term.input_line[s_term.input_len++] = c;
            s_term.input_line[s_term.input_len] = '\0';
        }
    }
}
