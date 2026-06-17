#include "editor.h"
#include "fs.h"
#include "heap.h"
#include "keyboard.h"
#include "vel/libc_compat.h"

extern void vga_clear(void);
extern void vga_print(const char* str, char color);
extern void shell_init(void);
extern void vga_draw_cursor(int col, int row, uint8_t color);

#define VGA_COLS 40
#define VGA_ROWS 25
#define MAX_FILE_SIZE 4096

static char* editor_buffer;
static int buffer_cursor = 0;
static int buffer_length = 0;
static char current_filename[16];

extern int is_editor_active;

/* Simple string/memory movement for insertion/deletion */
static void insert_char(char c) {
    if (buffer_length >= MAX_FILE_SIZE - 1) return;
    
    for (int i = buffer_length; i > buffer_cursor; i--) {
        editor_buffer[i] = editor_buffer[i - 1];
    }
    editor_buffer[buffer_cursor] = c;
    buffer_cursor++;
    buffer_length++;
}

static void delete_char() {
    if (buffer_cursor == 0) return;
    
    for (int i = buffer_cursor - 1; i < buffer_length - 1; i++) {
        editor_buffer[i] = editor_buffer[i + 1];
    }
    buffer_cursor--;
    buffer_length--;
}

static void editor_redraw() {
    vga_clear();
    
    /* Calculate cursor position on screen */
    int cur_r = 0, cur_c = 0;
    
    /* Determine scroll offset if cursor goes out of bounds */
    int target_r = 0;
    for (int i = 0; i < buffer_cursor; i++) {
        if (editor_buffer[i] == '\n') target_r++;
        else if ((i - target_r) % VGA_COLS == VGA_COLS - 1) target_r++;
    }
    
    /* For simplicity, we just print the whole buffer and trust vga_scroll to handle it 
       if the file is too big. However, since we clear the screen, we only print from the 
       point that fits the screen. If the file is small, just print all. */
       
    /* Just print everything */
    editor_buffer[buffer_length] = '\0';
    vga_print(editor_buffer, 0x0F);
    
    /* Now find the exact hardware cursor position */
    cur_r = 0; cur_c = 0;
    for (int i = 0; i < buffer_cursor; i++) {
        if (editor_buffer[i] == '\n') {
            cur_r++;
            cur_c = 0;
        } else {
            cur_c++;
            if (cur_c >= VGA_COLS) {
                cur_c = 0;
                cur_r++;
            }
        }
    }
    
    /* Update global cursor state and move hardware cursor */
    extern int cursor_row;
    extern int cursor_col;
    
    /* Erase the cursor that vga_print left at the end of the text */
    vga_draw_cursor(cursor_col, cursor_row, 0x00);
    
    /* If cursor scrolled past the screen, vga_print handled it, but our raw cur_r is high */
    if (cur_r >= VGA_ROWS) {
        cursor_row = VGA_ROWS - 1;
        cursor_col = cur_c;
    } else {
        cursor_row = cur_r;
        cursor_col = cur_c;
    }
    
    /* Draw the new software cursor */
    vga_draw_cursor(cursor_col, cursor_row, 0x0F);
}

void editor_start(const char* filename) {
    editor_buffer = (char*)kmalloc(MAX_FILE_SIZE);
    memset(editor_buffer, 0, MAX_FILE_SIZE);
    strncpy(current_filename, filename, 15);
    current_filename[15] = '\0';
    
    /* Try to load existing file */
    int bytes = fs_read_file(filename, editor_buffer);
    if (bytes > 0) {
        buffer_length = bytes;
        buffer_cursor = bytes;
    } else {
        buffer_length = 0;
        buffer_cursor = 0;
    }
    
    is_editor_active = 1;
    editor_redraw();
}

void editor_handle_char(char c) {
    if (c == KEY_ESC) {
        /* Save file and exit */
        editor_buffer[buffer_length] = '\0';
        fs_write_file(current_filename, editor_buffer);
        kfree(editor_buffer);
        
        is_editor_active = 0;
        
        vga_clear();
        shell_init();
        return;
    }
    else if (c == '\b') {
        delete_char();
    }
    else if (c == KEY_LEFT) {
        if (buffer_cursor > 0) buffer_cursor--;
    }
    else if (c == KEY_RIGHT) {
        if (buffer_cursor < buffer_length) buffer_cursor++;
    }
    else if (c == KEY_UP) {
        /* Move cursor to previous line (roughly) */
        while (buffer_cursor > 0 && editor_buffer[buffer_cursor - 1] != '\n') {
            buffer_cursor--;
        }
        if (buffer_cursor > 0) buffer_cursor--; /* Skip the newline */
        /* Now we are at the end of the previous line */
        while (buffer_cursor > 0 && editor_buffer[buffer_cursor - 1] != '\n') {
            buffer_cursor--;
        }
    }
    else if (c == KEY_DOWN) {
        while (buffer_cursor < buffer_length && editor_buffer[buffer_cursor] != '\n') {
            buffer_cursor++;
        }
        if (buffer_cursor < buffer_length) buffer_cursor++; /* Skip the newline */
    }
    else {
        insert_char(c);
    }
    
    editor_redraw();
}
