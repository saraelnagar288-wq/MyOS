#include <myos/kernel.h>
#include <myos/process.h>
#include <myos/desktop.h>
#include <myos/fs.h>
#include <myos/hal.h>

void init_process_main(void) {
    LOG_INFO("INIT", "MyOS Init System (PID 1) started");

    /* Read and display MOTD */
    vfs_node_t* motd_node = vfs_lookup("/system/motd");
    if (motd_node) {
        char buffer[256];
        uint32_t read_bytes = vfs_read(motd_node, 0, sizeof(buffer) - 1, (uint8_t*)buffer);
        buffer[read_bytes] = '\0';
        kprintf("\n%s\n", buffer);
    }

    /* Start Desktop GUI */
    desktop_init();

    LOG_INFO("INIT", "Init process entered main event loop");

    while (1) {
        input_event_t ev;
        if (input_poll_event(&ev)) {
            desktop_handle_input(&ev);
        }

        desktop_render();
        hal_sleep_ms(16); /* ~60 FPS update rate */
    }
}
