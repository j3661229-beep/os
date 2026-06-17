#include "timer.h"
#include "io.h"

volatile uint32_t system_ticks = 0;
static uint32_t timer_freq = 1000;

void timer_init(uint32_t frequency) {
    timer_freq = frequency;
    /* The value we send to the PIT is the value to divide it's input clock
       (1193180 Hz) by, to get our required frequency. */
    uint32_t divisor = 1193180 / frequency;
    
    /* Send the command byte. 0x36 sets the PIT to repeating mode. */
    outb(0x43, 0x36);
    
    /* Divisor has to be sent byte-wise, so split here into upper/lower bytes. */
    outb(0x40, (uint8_t)(divisor & 0xFF));
    outb(0x40, (uint8_t)((divisor >> 8) & 0xFF));
}

void timer_handler(void) {
    system_ticks++;
}

void sleep(uint32_t ms) {
    uint32_t end_ticks = system_ticks + (ms * timer_freq / 1000);
    while (system_ticks < end_ticks) {
        /* Halt the CPU until the next interrupt arrives to save power/cycles */
        __asm__ volatile("hlt");
    }
}
