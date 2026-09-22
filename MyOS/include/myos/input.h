#ifndef MYOS_INPUT_H
#define MYOS_INPUT_H

#include <myos/types.h>

typedef enum {
    INPUT_EVENT_NONE = 0,
    INPUT_EVENT_KEY_DOWN,
    INPUT_EVENT_KEY_UP,
    INPUT_EVENT_MOUSE_MOVE,
    INPUT_EVENT_MOUSE_DOWN,
    INPUT_EVENT_MOUSE_UP,
    INPUT_EVENT_MOUSE_SCROLL
} input_event_type_t;

typedef struct {
    input_event_type_t type;
    union {
        struct {
            uint8_t scancode;
            char character;
            bool ctrl;
            bool alt;
            bool shift;
        } key;
        struct {
            int32_t x;
            int32_t y;
            int32_t delta_x;
            int32_t delta_y;
            bool left_button;
            bool right_button;
            bool middle_button;
            int8_t scroll;
        } mouse;
    };
} input_event_t;

void input_init(void);
void input_push_event(const input_event_t* ev);
bool input_poll_event(input_event_t* out_ev);

/* Mouse Driver API */
void mouse_init(uint32_t screen_w, uint32_t screen_h);
void mouse_get_position(int32_t* out_x, int32_t* out_y, uint8_t* out_buttons);

/* Keyboard Driver API */
void keyboard_init(void);
bool keyboard_has_key(void);
char keyboard_get_char(void);

#endif /* MYOS_INPUT_H */
