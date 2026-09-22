#ifndef MYOS_HAL_H
#define MYOS_HAL_H

#include <myos/types.h>

typedef struct {
    uint8_t second;
    uint8_t minute;
    uint8_t hour;
    uint8_t day;
    uint8_t month;
    uint32_t year;
} rtc_time_t;

void hal_init(void);
uint64_t hal_get_uptime_ms(void);
uint64_t hal_get_ticks(void);
void hal_sleep_ms(uint32_t ms);
void hal_get_time(rtc_time_t* t);
void hal_reboot(void);
void hal_shutdown(void);

#endif /* MYOS_HAL_H */
