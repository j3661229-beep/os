/* ==============================================================================
 * panic.c - Kernel Panic handler
 *
 * Role: When the kernel encounters an unrecoverable error (like running out of
 * memory or a corrupt heap), it calls panic(). This function prints an error
 * message in red, disables interrupts, and halts the CPU forever.
 * ============================================================================== */

extern void vga_print(const char* str, char color);

/**
 * panic - Halt the system on critical failure
 * @msg: The error message to display
 */
void panic(const char* msg) {
    /* 0x04 is Red on Black */
    vga_print("\n\nKERNEL PANIC: ", 0x04);
    vga_print(msg, 0x04);
    vga_print("\nSystem Halted.", 0x04);

    /* Disable interrupts so we aren't disturbed */
    asm volatile("cli");

    /* Halt the CPU */
    asm volatile("hlt");

    /* Infinite loop as a fallback safety measure */
    while(1) {}
}
