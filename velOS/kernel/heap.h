#ifndef HEAP_H
#define HEAP_H

#include <stdint.h>

#define HEAP_START 0xC0000000   /* 3GB virtual memory mark */
#define HEAP_SIZE  0x00100000   /* 1MB initial size */
#define HEAP_MAGIC 0xDEADBEEF

typedef struct HeapBlock {
    uint32_t          size;      /* Size of THIS block (not including header) */
    uint8_t           is_free;   /* 1 if available, 0 if allocated */
    struct HeapBlock* next;      /* Pointer to next block in list */
    struct HeapBlock* prev;      /* Pointer to previous block */
    uint32_t          magic;     /* 0xDEADBEEF corruption check */
} HeapBlock;

void  heap_init(void);
void* kmalloc(uint32_t size);
void  kfree(void* ptr);
void* krealloc(void* ptr, uint32_t size);
void* kmalloc_aligned(uint32_t size, uint32_t align);
void  heap_dump(void);

#endif /* HEAP_H */
