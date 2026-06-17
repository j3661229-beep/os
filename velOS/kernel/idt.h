#ifndef IDT_H
#define IDT_H

#include <stdint.h>

/* ==============================================================================
 *  IDT Structures
 * ============================================================================== */

/* An entry in the Interrupt Descriptor Table */
typedef struct {
    uint16_t base_low;    /* Lower 16 bits of handler address */
    uint16_t selector;    /* Kernel code segment selector (0x08) */
    uint8_t  zero;        /* Always zero */
    uint8_t  flags;       /* 0x8E for Present, Ring0, 32-bit Interrupt Gate */
    uint16_t base_high;   /* Upper 16 bits of handler address */
} __attribute__((packed)) IDTEntry;

/* The pointer passed to the 'lidt' instruction */
typedef struct {
    uint16_t limit;       /* Size of the IDT in bytes - 1 */
    uint32_t base;        /* Address of the first element in our IDT array */
} __attribute__((packed)) IDTPointer;

/* Functions */
void idt_init(void);
void idt_set_gate(uint8_t num, uint32_t base);

#endif /* IDT_H */
