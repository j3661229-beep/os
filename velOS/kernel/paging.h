#ifndef PAGING_H
#define PAGING_H

#include <stdint.h>

#define PAGE_PRESENT  0x1
#define PAGE_WRITABLE 0x2
#define PAGE_USER     0x4

typedef uint32_t PageEntry;

/* A Page Table contains 1024 entries. Each entry points to a 4KB physical frame.
 * One Page Table covers exactly 4MB of memory (1024 * 4KB). */
typedef struct {
    PageEntry entries[1024];
} __attribute__((aligned(4096))) PageTable;

/* The Page Directory contains 1024 entries. Each entry points to a Page Table.
 * It covers the entire 4GB virtual address space. */
typedef struct {
    PageEntry entries[1024];
} __attribute__((aligned(4096))) PageDirectory;

/* A struct reflecting the CPU registers pushed during a page fault */
typedef struct {
    uint32_t ds;
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    uint32_t int_no, err_code;
    uint32_t eip, cs, eflags, useresp, ss;
} Registers;

void     paging_init(void);
void     paging_map(uint32_t virt, uint32_t phys, uint32_t flags);
void     paging_unmap(uint32_t virt);
uint32_t virt_to_phys(uint32_t virt);
void     page_fault_handler(Registers* r);

#endif /* PAGING_H */
