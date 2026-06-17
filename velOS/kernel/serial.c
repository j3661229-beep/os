#include "serial.h"
#include <stdint.h>

extern int is_script_running;
extern int is_editor_active;
extern void editor_handle_char(char c);
extern void shell_handle_char(char c);
extern void vga_print(const char* str, uint8_t color);
extern void vga_draw_cursor(int col, int row, uint8_t color);
extern void vga_draw_char(char c, int x, int y, uint8_t fg, uint8_t bg);
extern int cursor_col;
extern int cursor_row;

#define VGA_COLS 40
#define PORT 0x3F8          // COM1

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void outb(uint16_t port, uint8_t val) {
    asm volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

void serial_init(void) {
    outb(PORT + 1, 0x00);    // Disable all interrupts
    outb(PORT + 3, 0x80);    // Enable DLAB (set baud rate divisor)
    outb(PORT + 0, 0x01);    // Set divisor to 1 (lo byte) 115200 baud
    outb(PORT + 1, 0x00);    //                  (hi byte)
    outb(PORT + 3, 0x03);    // 8 bits, no parity, one stop bit
    outb(PORT + 2, 0xC7);    // Enable FIFO, clear them, with 14-byte threshold
    outb(PORT + 4, 0x0B);    // IRQs enabled, RTS/DSR set
    outb(PORT + 1, 0x01);    // Enable Data Available Interrupt
}

void serial_handler(void) {
    // Read while data is available
    while (inb(PORT + 5) & 1) {
        char c = inb(PORT);
        
        // Ignore carriage returns to avoid double newlines on Windows
        if (c == '\r') {
            continue;
        }

        if (is_script_running) continue;
        
        if (is_editor_active) {
            editor_handle_char(c);
        } else {
            if (c == '\b' || c == 0x7F) { // Backspace or DEL
                c = '\b';
                vga_draw_cursor(cursor_col, cursor_row, 0x00);
                if (cursor_col > 0) {
                    cursor_col--;
                } else if (cursor_row > 0) {
                    cursor_row--;
                    cursor_col = VGA_COLS - 1;
                }
                vga_draw_char(' ', cursor_col * 8, cursor_row * 8, 0x0F, 0x00);
                vga_draw_cursor(cursor_col, cursor_row, 0x0F);
            } else {
                char str[2] = {c, '\0'};
                vga_print(str, 0x0F);
            }
            shell_handle_char(c);
        }
    }
}
