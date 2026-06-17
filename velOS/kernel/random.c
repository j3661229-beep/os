#include "random.h"

static uint32_t current_seed = 1;

void seed_random(uint32_t seed) {
    if (seed == 0) seed = 1;
    current_seed = seed;
}

uint32_t get_random(void) {
    /* Simple Linear Congruential Generator (LCG) */
    /* Constants from POSIX/glibc */
    current_seed = current_seed * 1103515245 + 12345;
    return (uint32_t)(current_seed / 65536) % 32768;
}

uint32_t get_random_range(uint32_t min, uint32_t max) {
    if (min >= max) return min;
    uint32_t r = get_random();
    return min + (r % (max - min + 1));
}
