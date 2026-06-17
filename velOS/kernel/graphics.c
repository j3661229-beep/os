#include "graphics.h"
#include "font8x8.h"

#define VGA_WIDTH 320
#define VGA_HEIGHT 200
#define VGA_MEMORY 0xA0000

void vga_put_pixel(int x, int y, uint8_t color) {
    if (x >= 0 && x < VGA_WIDTH && y >= 0 && y < VGA_HEIGHT) {
        uint8_t* vga = (uint8_t*)VGA_MEMORY;
        vga[y * VGA_WIDTH + x] = color;
    }
}

void vga_clear_screen(uint8_t color) {
    uint8_t* vga = (uint8_t*)VGA_MEMORY;
    /* 320 * 200 = 64000 bytes */
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        vga[i] = color;
    }
}

void vga_draw_char(char c, int x, int y, uint8_t fg, uint8_t bg) {
    if (c < 0 || c >= 128) c = '?';
    
    for (int row = 0; row < 8; row++) {
        uint8_t row_data = font8x8_basic[(int)c][row];
        for (int col = 0; col < 8; col++) {
            if (row_data & (1 << col)) {
                vga_put_pixel(x + col, y + row, fg);
            } else {
                vga_put_pixel(x + col, y + row, bg);
            }
        }
    }
}

void vga_draw_cursor(int col, int row, uint8_t color) {
    int x = col * 8;
    int y = row * 8;
    /* Draw underscore at bottom 2 pixels */
    for (int i = 0; i < 8; i++) {
        vga_put_pixel(x + i, y + 6, color);
        vga_put_pixel(x + i, y + 7, color);
    }
}

void vga_print_at(int x, int y, const char* str, uint8_t color) {
    int curr_x = x;
    while (*str) {
        if (*str == '\n') {
            y += 8;
            curr_x = x;
        } else {
            vga_draw_char(*str, curr_x, y, color, 0); // Background 0 (black)
            curr_x += 8;
        }
        str++;
    }
}
