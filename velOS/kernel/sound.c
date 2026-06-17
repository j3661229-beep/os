#include "sound.h"
#include "io.h"
#include "timer.h"

// Play sound using built-in speaker
void play_sound(uint32_t frequency) {
    uint32_t div;
    uint8_t tmp;
    
    if (frequency == 0) return;
    
    // Set the PIT to the desired frequency
    div = 1193180 / frequency;
    outb(0x43, 0xB6);
    outb(0x42, (uint8_t) (div) );
    outb(0x42, (uint8_t) (div >> 8));
    
    // And play the sound using the PC speaker
    tmp = inb(0x61);
    if (tmp != (tmp | 3)) {
        outb(0x61, tmp | 3);
    }
}

// Make it shut up
void nosound(void) {
    uint8_t tmp = inb(0x61) & 0xFC;
    outb(0x61, tmp);
}

// Make a beep
void beep(uint32_t frequency, uint32_t ms) {
    play_sound(frequency);
    sleep(ms);
    nosound();
}
