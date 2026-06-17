#ifndef PMM_H
#define PMM_H

#include <stdint.h>

#define PAGE_SIZE     4096          /* 4KB per frame */
#define TOTAL_MEMORY  0x01000000    /* Assume 16MB RAM */
#define TOTAL_FRAMES  (TOTAL_MEMORY / PAGE_SIZE)  /* 4096 frames */
#define BITMAP_SIZE   (TOTAL_FRAMES / 32)         /* 128 uint32s */

void     pmm_init(void);
uint32_t pmm_alloc_frame(void);
void     pmm_free_frame(uint32_t phys_addr);
uint32_t pmm_free_frame_count(void);

#endif /* PMM_H */
