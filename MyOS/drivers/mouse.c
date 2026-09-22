#include <myos/input.h>
#include <myos/idt.h>
#include <myos/cpu.h>
#include <myos/kernel.h>

#define PS2_DATA_PORT    0x60
#define PS2_STATUS_PORT  0x64
#define PS2_COMMAND_PORT 0x64

static uint32_t g_screen_width  = 1024;
static uint32_t g_screen_height = 768;

static int32_t  g_mouse_x = 512;
static int32_t  g_mouse_y = 384;
static uint8_t  g_mouse_buttons = 0;

static uint8_t  g_mouse_cycle = 0;
static uint8_t  g_mouse_bytes[3];

static inline void mouse_wait_write(void) {
    uint32_t timeout = 100000;
    while (timeout--) {
        if ((inb(PS2_STATUS_PORT) & 0x02) == 0) return;
    }
}

static inline void mouse_wait_read(void) {
    uint32_t timeout = 100000;
    while (timeout--) {
        if ((inb(PS2_STATUS_PORT) & 0x01) == 1) return;
    }
}

static void mouse_write(uint8_t write) {
    mouse_wait_write();
    outb(PS2_COMMAND_PORT, 0xD4);
    mouse_wait_write();
    outb(PS2_DATA_PORT, write);
}

static uint8_t mouse_read(void) {
    mouse_wait_read();
    return inb(PS2_DATA_PORT);
}

static void mouse_interrupt_handler(struct interrupt_registers* regs) {
    (void)regs;
    uint8_t status = inb(PS2_STATUS_PORT);
    if (!(status & 0x20)) {
        return; /* Not from mouse */
    }

    uint8_t b = inb(PS2_DATA_PORT);
    g_mouse_bytes[g_mouse_cycle++] = b;

    if (g_mouse_cycle == 3) {
        g_mouse_cycle = 0;

        uint8_t flags = g_mouse_bytes[0];
        if (!(flags & 0x08)) {
            return; /* Discard bad packet (bit 3 must be 1) */
        }

        int32_t dx = (int32_t)g_mouse_bytes[1];
        int32_t dy = (int32_t)g_mouse_bytes[2];

        if (flags & 0x10) dx |= 0xFFFFFF00; /* X sign extend */
        if (flags & 0x20) dy |= 0xFFFFFF00; /* Y sign extend */

        /* PS/2 Y delta is positive upwards, screen Y is positive downwards */
        g_mouse_x += dx;
        g_mouse_y -= dy;

        /* Clamp to screen boundaries */
        if (g_mouse_x < 0) g_mouse_x = 0;
        if (g_mouse_x >= (int32_t)g_screen_width) g_mouse_x = g_screen_width - 1;
        if (g_mouse_y < 0) g_mouse_y = 0;
        if (g_mouse_y >= (int32_t)g_screen_height) g_mouse_y = g_screen_height - 1;

        bool left   = (flags & 0x01) != 0;
        bool right  = (flags & 0x02) != 0;
        bool middle = (flags & 0x04) != 0;

        uint8_t prev_buttons = g_mouse_buttons;
        g_mouse_buttons = (left ? 1 : 0) | (right ? 2 : 0) | (middle ? 4 : 0);

        input_event_t ev;
        if (dx != 0 || dy != 0) {
            ev.type = INPUT_EVENT_MOUSE_MOVE;
            ev.mouse.x = g_mouse_x;
            ev.mouse.y = g_mouse_y;
            ev.mouse.delta_x = dx;
            ev.mouse.delta_y = -dy;
            ev.mouse.left_button   = left;
            ev.mouse.right_button  = right;
            ev.mouse.middle_button = middle;
            ev.mouse.scroll = 0;
            input_push_event(&ev);
        }

        if (left != ((prev_buttons & 1) != 0)) {
            ev.type = left ? INPUT_EVENT_MOUSE_DOWN : INPUT_EVENT_MOUSE_UP;
            ev.mouse.x = g_mouse_x;
            ev.mouse.y = g_mouse_y;
            ev.mouse.left_button   = left;
            ev.mouse.right_button  = right;
            ev.mouse.middle_button = middle;
            input_push_event(&ev);
        }
    }
}

void mouse_init(uint32_t screen_w, uint32_t screen_h) {
    g_screen_width  = screen_w;
    g_screen_height = screen_h;
    g_mouse_x = screen_w / 2;
    g_mouse_y = screen_h / 2;

    /* Enable auxiliary mouse device */
    mouse_wait_write();
    outb(PS2_COMMAND_PORT, 0xA8);

    /* Enable interrupts in PS/2 controller config byte */
    mouse_wait_write();
    outb(PS2_COMMAND_PORT, 0x20);
    uint8_t status = mouse_read() | 0x02; /* Enable mouse IRQ12 */
    mouse_wait_write();
    outb(PS2_COMMAND_PORT, 0x60);
    mouse_wait_write();
    outb(PS2_DATA_PORT, status);

    /* Use default mouse settings */
    mouse_write(0xF6);
    mouse_read(); /* ACK */

    /* Enable data reporting */
    mouse_write(0xF4);
    mouse_read(); /* ACK */

    register_interrupt_handler(44, mouse_interrupt_handler); /* IRQ12 -> Vector 44 */
    pic_unmask_irq(2);  /* Unmask cascade line */
    pic_unmask_irq(12); /* Unmask mouse IRQ12 */

    LOG_INFO("MOUSE", "PS/2 Mouse initialized (screen bounds: %ux%u)", screen_w, screen_h);
}

void mouse_get_position(int32_t* out_x, int32_t* out_y, uint8_t* out_buttons) {
    if (out_x) *out_x = g_mouse_x;
    if (out_y) *out_y = g_mouse_y;
    if (out_buttons) *out_buttons = g_mouse_buttons;
}
