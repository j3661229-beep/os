/* ==============================================================================
 * pmm.c - Physical Memory Manager
 *
 * Role: Keeps track of which 4KB physical frames of RAM are free and which 
 * are in use. We use a bitmap where 1 bit represents 1 page frame (4KB).
 * 0 = Free, 1 = Used.
 * ============================================================================== */

#include "pmm.h"

extern void panic(const char* msg);

/* The bitmap array: 128 uint32s = 4096 bits = 4096 frames = 16MB of tracked RAM */
static uint32_t pmm_bitmap[BITMAP_SIZE];

/**
 * bitmap_set - Mark a frame as USED
 */
static void bitmap_set(uint32_t frame) {
    uint32_t idx = frame / 32;
    uint32_t bit = frame % 32;
    pmm_bitmap[idx] |= (1 << bit);
}

/**
 * bitmap_clear - Mark a frame as FREE
 */
static void bitmap_clear(uint32_t frame) {
    uint32_t idx = frame / 32;
    uint32_t bit = frame % 32;
    pmm_bitmap[idx] &= ~(1 << bit);
}

/**
 * bitmap_test - Check if a frame is used
 * Returns non-zero if used, 0 if free
 */
static int bitmap_test(uint32_t frame) {
    uint32_t idx = frame / 32;
    uint32_t bit = frame % 32;
    return (pmm_bitmap[idx] & (1 << bit));
}

/**
 * pmm_init - Initialize the physical memory manager
 */
void pmm_init() {
    /* Initially mark EVERYTHING as used to be safe */
    for (int i = 0; i < BITMAP_SIZE; i++) {
        pmm_bitmap[i] = 0xFFFFFFFF;
    }

    /* Free the usable RAM region: 1MB to 16MB (0x100000 to 0x1000000)
     * Frames: 0x100000 / 4096 = 256
     * To:     0x1000000 / 4096 = 4096 */
    for (uint32_t i = 256; i < TOTAL_FRAMES; i++) {
        bitmap_clear(i);
    }

    /* Note: Low memory (frames 0 to 255) remains marked as USED.
     * This protects the BIOS data area, VGA buffer, and our loaded Kernel.
     * Our kernel loads at 0x1000, so it's fully protected inside this 1MB block. */
}

/**
 * pmm_alloc_frame - Finds the first free physical frame, marks it used, 
 * and returns its physical address.
 */
uint32_t pmm_alloc_frame() {
    for (uint32_t i = 0; i < BITMAP_SIZE; i++) {
        /* If not all bits in this uint32 are 1 */
        if (pmm_bitmap[i] != 0xFFFFFFFF) {
            for (uint32_t j = 0; j < 32; j++) {
                uint32_t frame = (i * 32) + j;
                if (!bitmap_test(frame)) {
                    bitmap_set(frame);
                    return frame * PAGE_SIZE;
                }
            }
        }
    }
    panic("Out of physical memory!");
    return 0; /* Should never be reached */
}

/**
 * pmm_free_frame - Marks a physical frame as free
 * @phys_addr: The physical address of the frame to free
 */
void pmm_free_frame(uint32_t phys_addr) {
    uint32_t frame = phys_addr / PAGE_SIZE;
    bitmap_clear(frame);
}

/**
 * pmm_free_frame_count - Returns the total number of free frames
 */
uint32_t pmm_free_frame_count() {
    uint32_t count = 0;
    for (uint32_t i = 0; i < TOTAL_FRAMES; i++) {
        if (!bitmap_test(i)) {
            count++;
        }
    }
    return count;
}
