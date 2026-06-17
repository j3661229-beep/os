#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>

extern volatile uint32_t system_ticks;

void timer_init(uint32_t frequency);
void timer_handler(void);
void sleep(uint32_t ms);

#endif
