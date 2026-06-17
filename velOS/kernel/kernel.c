/* ==============================================================================
 * kernel.c - VelOS Core Kernel
 *
 * Role: This is the very first piece of C code executed in our OS.
 * Since we have no standard library, we must interact directly with hardware.
 * We write to the VGA text mode buffer at physical address 0xB8000 to display
 * text on the screen.
 * ============================================================================== */

#include <stdint.h>
#include "idt.h"
#include "pmm.h"
#include "paging.h"
#include "heap.h"
#include "shell.h"
#include "fs.h"
#include "io.h"
#include "graphics.h"
#include "timer.h"

/* Memory-mapped I/O address for VGA graphics mode */
#define VGA_ADDRESS 0xA0000

/* VGA Mode 13h with 8x8 font dimensions */
#define VGA_ROWS 25
#define VGA_COLS 40

/* Keep track of current cursor position */
int cursor_row = 0;
int cursor_col = 0;

/**
 * vga_scroll - Scrolls the screen up by one line
 */
void vga_scroll() {
    uint8_t* vga = (uint8_t*)VGA_ADDRESS;
    int row_bytes = 320 * 8;
    int total_bytes = 320 * 200;
    
    /* Move all rows up by one character row (8 pixels) */
    for (int i = 0; i < total_bytes - row_bytes; i++) {
        vga[i] = vga[i + row_bytes];
    }
    
    /* Clear the last character row */
    for (int i = total_bytes - row_bytes; i < total_bytes; i++) {
        vga[i] = 0x00; /* Black */
    }
    
    cursor_row = VGA_ROWS - 1;
}

/**
 * vga_clear - Clears the screen by filling it with spaces
 */
void vga_clear() {
    vga_clear_screen(0x00);
    cursor_row = 0;
    cursor_col = 0;
    vga_draw_cursor(cursor_col, cursor_row, 0x0F);
}

/**
 * vga_print - Prints a string to the screen with a specific color
 */
void vga_print(const char* str, char color) {
    /* Erase old cursor before moving */
    vga_draw_cursor(cursor_col, cursor_row, 0x00);
    
    for (int i = 0; str[i] != '\0'; i++) {
        if (str[i] == '\n') {
            cursor_row++;
            cursor_col = 0;
            if (cursor_row >= VGA_ROWS) {
                vga_scroll();
            }
            continue;
        }
        
        vga_draw_char(str[i], cursor_col * 8, cursor_row * 8, color, 0x00);
        
        cursor_col++;
        if (cursor_col >= VGA_COLS) {
            cursor_col = 0;
            cursor_row++;
        }
        
        if (cursor_row >= VGA_ROWS) {
            vga_scroll();
        }
    }
    /* Draw new cursor */
    vga_draw_cursor(cursor_col, cursor_row, 0x0F);
}

/**
 * vga_print_hex - Prints an integer in hexadecimal format
 */
void vga_print_hex(unsigned int n) {
    char hex_str[11];
    hex_str[0] = '0';
    hex_str[1] = 'x';
    hex_str[10] = '\0';
    
    const char* hex_chars = "0123456789ABCDEF";
    
    for (int i = 9; i >= 2; i--) {
        hex_str[i] = hex_chars[n & 0x0F];
        n >>= 4;
    }
    
    vga_print(hex_str, 0x0F);
}

/**
 * kernel_main - The main entry point for our C kernel
 */
void kernel_main() {
    vga_clear();
    vga_print("VelOS v0.1\n", 0x0F);

    /* Memory init sequence (ORDER MATTERS): */
    pmm_init();
    vga_print("[PMM] Physical Memory Manager ready\n", 0x0A);
    
    paging_init();
    vga_print("[PAGE] Paging enabled\n", 0x0A);
    
    heap_init();
    vga_print("[HEAP] Kernel heap ready\n", 0x0A);

    /* Register page fault handler in IDT slot 14 */
    idt_set_gate(14, (uint32_t)page_fault_handler);

    idt_init();
    vga_print("[IDT] Interrupts enabled\n", 0x0A);
    
    timer_init(1000);
    vga_print("[PIT] Timer initialized at 1000Hz\n", 0x0A);

    /* Test kmalloc */
    char* test = (char*)kmalloc(64);
    if (test) {
        vga_print("[HEAP TEST] kmalloc(64) success\n", 0x0B);
        kfree(test);
        vga_print("[HEAP TEST] kfree success\n", 0x0B);
    }

    uint32_t free_frames = pmm_free_frame_count();
    vga_print("[PMM] Free frames: ", 0x0E);
    vga_print_hex(free_frames);
    vga_print("\n", 0x0F);

    vga_print("\nVelOS Ready. Shell coming next...\n", 0x0D);
    
    /* Initialize VelFS file system */
    fs_init();
    
    /* Initialize COM1 Serial Port for copy/paste support */
    extern void serial_init(void);
    serial_init();
    vga_print("[COM1] Serial port initialized\n", 0x0A);
    
    /* Initialize the interactive shell */
    shell_init();

    /* Hang forever to prevent exiting back to random memory */
    while (1) {}
}
