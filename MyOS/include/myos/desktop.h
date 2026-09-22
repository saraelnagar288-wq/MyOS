#ifndef MYOS_DESKTOP_H
#define MYOS_DESKTOP_H

#include <myos/types.h>
#include <myos/gfx.h>
#include <myos/input.h>

void desktop_init(void);
void desktop_render_frame(void);
void desktop_run(void);

void app_terminal_open(void);
void app_terminal_handle_key(char c);
void app_filemanager_open(void);
void app_taskmanager_open(void);
void app_sysinfo_open(void);
void app_settings_open(void);

#endif /* MYOS_DESKTOP_H */
