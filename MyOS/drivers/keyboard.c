#include <myos/input.h>
#include <myos/idt.h>
#include <myos/cpu.h>
#include <myos/kernel.h>

#define PS2_DATA_PORT    0x60
#define PS2_STATUS_PORT  0x64

static bool g_shift_pressed = false;
static bool g_ctrl_pressed  = false;
static bool g_alt_pressed   = false;

/* US QWERTY Scan Code Set 1 lookup */
static const char kbd_us_ascii[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, /* Ctrl */
    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0, /* Left Shift */
    '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',
    0, /* Right Shift */
    '*',
    0, /* Alt */
    ' ', /* Space */
    0, /* Caps lock */
};

static const char kbd_us_ascii_shift[128] = {
    0,  27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0, /* Ctrl */
    'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
    0, /* Left Shift */
    '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?',
    0, /* Right Shift */
    '*',
    0, /* Alt */
    ' ', /* Space */
    0, /* Caps lock */
};

static void keyboard_interrupt_handler(struct interrupt_registers* regs) {
    (void)regs;
    uint8_t scancode = inb(PS2_DATA_PORT);

    bool is_release = (scancode & 0x80) != 0;
    uint8_t key_code = scancode & 0x7F;

    if (key_code == 0x2A || key_code == 0x36) {
        g_shift_pressed = !is_release;
    } else if (key_code == 0x1D) {
        g_ctrl_pressed = !is_release;
    } else if (key_code == 0x38) {
        g_alt_pressed = !is_release;
    }

    char ch = 0;
    if (key_code < sizeof(kbd_us_ascii)) {
        ch = g_shift_pressed ? kbd_us_ascii_shift[key_code] : kbd_us_ascii[key_code];
    }

    input_event_t ev;
    ev.type = is_release ? INPUT_EVENT_KEY_UP : INPUT_EVENT_KEY_DOWN;
    ev.key.scancode  = key_code;
    ev.key.character = ch;
    ev.key.ctrl      = g_ctrl_pressed;
    ev.key.alt       = g_alt_pressed;
    ev.key.shift     = g_shift_pressed;

    input_push_event(&ev);
}

void keyboard_init(void) {
    register_interrupt_handler(33, keyboard_interrupt_handler); /* IRQ1 -> Vector 33 */
    pic_unmask_irq(1);

    LOG_INFO("KBD", "PS/2 Keyboard driver registered on IRQ1");
}
