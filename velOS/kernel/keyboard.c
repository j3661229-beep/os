/* ==============================================================================
 * keyboard.c - PS/2 Keyboard Driver
 *
 * Role: Translates raw hardware scancodes from the keyboard into printable
 * ASCII characters, and prints them to the screen.
 * ============================================================================== */

#include "keyboard.h"
#include "pic.h"
#include "shell.h"

/* Extern our VGA print function to display typed characters */
extern void vga_print(const char* str, char color);

int is_editor_active = 0;
int is_script_running = 0;
volatile char last_pressed_key = 0;

char get_last_key(void) {
    char k = last_pressed_key;
    last_pressed_key = 0;
    return k;
}

/* We need access to the cursor state to handle backspace properly */
extern int cursor_row;
extern int cursor_col;

/* Memory-mapped I/O address for VGA text mode */
#define VGA_ADDRESS 0xA0000
#define VGA_COLS 40

/* Basic US QWERTY Scancode Table (Set 1) for 'Make' codes (key pressed) */
const char scancodes[128] = {
    0, KEY_ESC, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b', /* Backspace */
  '\t', /* Tab */
  'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n', /* Enter */
    0, /* Control */
  'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',   0, /* Left Shift */
  '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',   0, /* Right Shift */
  '*',
    0,  /* Alt */
  ' ',  /* Space bar */
    0,  /* Caps lock */
    0,  /* F1 */
    0,  /* F2 */
    0,  /* F3 */
    0,  /* F4 */
    0,  /* F5 */
    0,  /* F6 */
    0,  /* F7 */
    0,  /* F8 */
    0,  /* F9 */
    0,  /* F10 */
    0,  /* Num lock*/
    0,  /* Scroll Lock */
    0,  /* Home key */
    KEY_UP,  /* Up Arrow */
    0,  /* Page Up */
  '-',
    KEY_LEFT,  /* Left Arrow */
    0,
    KEY_RIGHT,  /* Right Arrow */
  '+',
    0,  /* End key*/
    KEY_DOWN,  /* Down Arrow */
    0,  /* Page Down */
    0,  /* Insert Key */
    0,  /* Delete Key */
    0,   0,   0,
    0,  /* F11 Key */
    0,  /* F12 Key */
    0, /* All other keys are undefined */
};

const char shifted_scancodes[128] = {
    0, KEY_ESC, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
  '\t',
  'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0,
  'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',   0,
  '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?',   0,
  '*',
    0,  /* Alt */
  ' ',  /* Space bar */
    0,  /* Caps lock */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* F1-F10 */
    0,  /* Num lock*/
    0,  /* Scroll Lock */
    0,  /* Home key */
    KEY_UP,  /* Up Arrow */
    0,  /* Page Up */
  '-',
    KEY_LEFT,  /* Left Arrow */
    0,
    KEY_RIGHT,  /* Right Arrow */
  '+',
    0,  /* End key*/
    KEY_DOWN,  /* Down Arrow */
    0,  /* Page Down */
    0,  /* Insert Key */
    0,  /* Delete Key */
    0,   0,   0,
    0,  /* F11 Key */
    0,  /* F12 Key */
    0, /* All other keys are undefined */
};

int shift_pressed = 0;

/**
 * keyboard_handler - Read a scancode from the keyboard and print the char
 */
void keyboard_handler() {
    /* Read from keyboard data port */
    uint8_t scancode = inb(0x60);

    /* If the top bit is set, it's a key release (Break code). */
    if (scancode & 0x80) {
        if (scancode == 0xAA || scancode == 0xB6) { /* Left Shift or Right Shift release */
            shift_pressed = 0;
        }
        return;
    }

    if (scancode == 0x2A || scancode == 0x36) { /* Left Shift or Right Shift pressed */
        shift_pressed = 1;
        return;
    }

    /* Translate scancode to ASCII */
    char c = shift_pressed ? shifted_scancodes[scancode] : scancodes[scancode];

    /* If it is a valid, printable character */
    if (c != 0) {
        last_pressed_key = c;
        if (is_script_running) return;
        
        if (is_editor_active) {
            extern void editor_handle_char(char c);
            editor_handle_char(c);
        } else {
            if (c == '\b') {
                extern void vga_draw_cursor(int col, int row, uint8_t color);
                extern void vga_draw_char(char c, int x, int y, uint8_t fg, uint8_t bg);
                
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
                /* Normal character printing */
                char str[2] = {c, '\0'};
                vga_print(str, 0x0F);
            }
            /* Pass the character to the shell buffer */
            shell_handle_char(c);
        }
    }
}
