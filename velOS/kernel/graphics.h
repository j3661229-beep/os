#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <stdint.h>

void vga_put_pixel(int x, int y, uint8_t color);
void vga_clear_screen(uint8_t color);
void vga_draw_char(char c, int x, int y, uint8_t fg, uint8_t bg);
void vga_draw_cursor(int col, int row, uint8_t color);
void vga_print_at(int x, int y, const char* str, uint8_t color);

#endif
