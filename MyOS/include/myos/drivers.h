#ifndef MYOS_DRIVERS_H
#define MYOS_DRIVERS_H

#include <myos/types.h>

/* Serial UART 16550 */
#define COM1_PORT 0x3F8

void serial_init(void);
void serial_write_char(char c);
void serial_write_string(const char* str);
bool serial_received(void);
char serial_read(void);

/* PIT (Programmable Interval Timer) */
void pit_init(uint32_t frequency_hz);
uint64_t pit_get_ticks(void);

/* RTC (Real Time Clock) */
void rtc_read_datetime(uint8_t* sec, uint8_t* min, uint8_t* hour, uint8_t* day, uint8_t* month, uint32_t* year);

/* PS/2 Keyboard & Mouse */
void ps2_init(void);

#endif /* MYOS_DRIVERS_H */
