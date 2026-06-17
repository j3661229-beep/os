#ifndef RANDOM_H
#define RANDOM_H

#include <stdint.h>

void seed_random(uint32_t seed);
uint32_t get_random(void);
uint32_t get_random_range(uint32_t min, uint32_t max);

#endif
