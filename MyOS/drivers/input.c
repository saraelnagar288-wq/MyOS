#include <myos/input.h>
#include <myos/kernel.h>

#define INPUT_QUEUE_SIZE 128

static input_event_t g_input_queue[INPUT_QUEUE_SIZE];
static volatile uint32_t g_queue_head = 0;
static volatile uint32_t g_queue_tail = 0;

void input_init(void) {
    g_queue_head = 0;
    g_queue_tail = 0;
    kmemset(g_input_queue, 0, sizeof(g_input_queue));
    LOG_INFO("INPUT", "Unified input event queue initialized");
}

void input_push_event(const input_event_t* ev) {
    if (!ev) return;

    uint32_t next = (g_queue_head + 1) % INPUT_QUEUE_SIZE;
    if (next != g_queue_tail) {
        g_input_queue[g_queue_head] = *ev;
        g_queue_head = next;
    }
}

bool input_poll_event(input_event_t* out_ev) {
    if (!out_ev || g_queue_head == g_queue_tail) {
        return false;
    }

    *out_ev = g_input_queue[g_queue_tail];
    g_queue_tail = (g_queue_tail + 1) % INPUT_QUEUE_SIZE;
    return true;
}
