/* ==============================================================================
 * isr.c - Interrupt Service Routines (C Handlers)
 *
 * Role: When the assembly stubs in isr.asm are triggered, they save the state
 * and call these C functions. This file acts as the main router.
 * ============================================================================== */

#include <stdint.h>
#include "pic.h"

/* We must declare these functions manually since we don't have standard headers yet */
extern void vga_print(const char* str, char color);
extern void keyboard_handler(void);
extern void timer_handler(void);

/* Matches the stack layout pushed by isr_common_stub and irq_common_stub */
typedef struct {
    uint32_t ds;                                     /* Data segment pushed by us */
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax; /* Pushed by pusha */
    uint32_t int_no, err_code;                       /* Pushed by our ISR stub */
    uint32_t eip, cs, eflags, useresp, ss;           /* Pushed by CPU automatically */
} Registers;

/* Array of exception messages */
const char* exception_messages[] = {
    "Division By Zero",
    "Debug",
    "Non Maskable Interrupt",
    "Breakpoint",
    "Into Detected Overflow",
    "Out of Bounds",
    "Invalid Opcode",
    "No Coprocessor",
    "Double Fault",
    "Coprocessor Segment Overrun",
    "Bad TSS",
    "Segment Not Present",
    "Stack Fault",
    "General Protection Fault",
    "Page Fault",
    "Unknown Interrupt",
    "Coprocessor Fault",
    "Alignment Check",
    "Machine Check",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved"
};

/**
 * isr_handler - Handles CPU exceptions
 */
void isr_handler(Registers* r) {
    if (r->int_no < 32) {
        vga_print("\nEXCEPTION: ", 0x0C); /* Red text */
        vga_print(exception_messages[r->int_no], 0x0C);
        vga_print("\nSystem Halted!\n", 0x0C);
        
        /* Hang forever on exception */
        while(1) {}
    }
}

/**
 * irq_handler - Handles hardware interrupts from the PIC
 */
void irq_handler(Registers* r) {
    /* Handle specific IRQs */
    if (r->int_no == 32) {
        /* IRQ0 is mapped to 32. This is the PIT timer. */
        timer_handler();
    } else if (r->int_no == 33) {
        /* IRQ1 is mapped to 33. This is the keyboard. */
        keyboard_handler();
    } else if (r->int_no == 36) {
        /* IRQ4 is mapped to 36. This is the COM1 serial port. */
        extern void serial_handler(void);
        serial_handler();
    }

    /* Send End of Interrupt (EOI) to the PIC so it knows we are done */
    /* Hardware IRQs map to int_no 32-47. Subtract 32 to get IRQ number 0-15 */
    pic_send_eoi(r->int_no - 32);
}
