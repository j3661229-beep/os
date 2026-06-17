#ifndef PIC_H
#define PIC_H

#include <stdint.h>

/* PIC Ports */
#define PIC1_COMMAND 0x20
#define PIC1_DATA    0x21
#define PIC2_COMMAND 0xA0
#define PIC2_DATA    0xA1

/* Inline assembly helpers to read/write hardware ports */
static inline void outb(uint16_t port, uint8_t val) {
    /* 'a' forces 'val' into AL register, 'Nd' forces 'port' into DX register */
    asm volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    /* '=a' writes AL register into 'ret', 'Nd' forces 'port' into DX register */
    asm volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/* PIC Functions */
void pic_init(void);
void pic_send_eoi(uint8_t irq);

#endif /* PIC_H */
