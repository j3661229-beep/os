/* ==============================================================================
 * pic.c - Programmable Interrupt Controller
 *
 * Role: The 8259 PIC handles hardware interrupts (IRQs) and routes them to the CPU.
 * By default, BIOS maps IRQs 0-7 to CPU interrupts 8-15, which collide with 
 * CPU exceptions (like Page Fault). We must remap the PIC to route IRQs to 
 * interrupts 32-47.
 * ============================================================================== */

#include "pic.h"

/* ICW (Initialization Command Word) constants */
#define ICW1_INIT 0x11
#define ICW4_8086 0x01

/**
 * pic_init - Remaps the PIC to offsets 32 and 40, and unmasks IRQ1.
 */
void pic_init() {
    /* Start the initialization sequence (in cascade mode) */
    outb(PIC1_COMMAND, ICW1_INIT);
    outb(PIC2_COMMAND, ICW1_INIT);

    /* ICW2: Vector offset */
    outb(PIC1_DATA, 0x20); /* PIC1 maps to INT 32-39 */
    outb(PIC2_DATA, 0x28); /* PIC2 maps to INT 40-47 */

    /* ICW3: Cascade setup */
    outb(PIC1_DATA, 0x04); /* Tell PIC1 there is a slave PIC at IRQ2 (0000 0100) */
    outb(PIC2_DATA, 0x02); /* Tell PIC2 its cascade identity (0000 0010) */

    /* ICW4: 8086/88 mode */
    outb(PIC1_DATA, ICW4_8086);
    outb(PIC2_DATA, ICW4_8086);

    /* Masking: Unmask IRQ0 (Timer), IRQ1 (Keyboard), and IRQ4 (COM1) on PIC1, mask everything else */
    /* 0xEC = 1110 1100 (Bits 0, 1, and 4 are 0, meaning unmasked) */
    outb(PIC1_DATA, 0xEC);
    /* 0xFF = 1111 1111 (All masked on PIC2) */
    outb(PIC2_DATA, 0xFF);
}

/**
 * pic_send_eoi - Sends End-Of-Interrupt signal to the PICs
 * @irq: The hardware IRQ number (0-15)
 */
void pic_send_eoi(uint8_t irq) {
    /* If IRQ comes from the slave PIC, we must send EOI to both */
    if (irq >= 8) {
        outb(PIC2_COMMAND, 0x20);
    }
    /* Always send EOI to the master PIC */
    outb(PIC1_COMMAND, 0x20);
}
