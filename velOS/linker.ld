/* ==============================================================================
 * linker.ld - Kernel Linker Script
 *
 * Role: Tells the GCC Linker how to combine our object files (.o) into a 
 * single raw binary executable. It dictates the exact memory layout of the 
 * kernel, ensuring the entry point is at the very beginning and everything 
 * is loaded at the correct physical address.
 * ============================================================================== */

/* The entry point of our OS is the _start label in kernel_entry.asm */
ENTRY(_start)

SECTIONS
{
    /* The bootloader loads the kernel at physical address 0x10000 */
    /* (Segment 0x1000, Offset 0x0000 in Real Mode)               */
    . = 0x10000;

    /* 1. Code Section (.text) */
    .text : ALIGN(512)
    {
        /* Ensure kernel_entry.asm is placed at the very front of the binary */
        *(.entry)
        
        /* Followed by all other code */
        *(.text)
    }

    /* 2. Read-Only Data Section (.rodata) - Constants, strings */
    .rodata : ALIGN(512)
    {
        *(.rodata)
    }

    /* 3. Initialized Data Section (.data) - Global variables */
    .data : ALIGN(512)
    {
        *(.data)
    }

    /* 4. Uninitialized Data Section (.bss) - Zero-initialized variables */
    .bss : ALIGN(512)
    {
        __bss_start = .;
        *(COMMON)
        *(.bss)
        __bss_end = .;
    }
}
