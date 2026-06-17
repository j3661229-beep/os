/* ==============================================================================
 * shell.c - Basic Command Line Shell for VelOS
 * ============================================================================== */

#include "shell.h"
#include <stdint.h>
#include "heap.h"
#include "vel/libc_compat.h"
#include "vel/lexer.h"
#include "vel/parser.h"
#include "vel/interpreter.h"
#include "fs.h"
#include "editor.h"

extern void vga_print(const char* str, char color);
extern void vga_print_hex(unsigned int n);
extern void vga_clear(void);
extern uint32_t pmm_free_frame_count(void);

#define CMD_BUFFER_SIZE 256
static char cmd_buffer[CMD_BUFFER_SIZE];
static int cmd_index = 0;

/* String functions are provided by libc_compat.h */

/**
 * shell_init - Reset shell state and print prompt
 */
void shell_init() {
    cmd_index = 0;
    for (int i = 0; i < CMD_BUFFER_SIZE; i++) cmd_buffer[i] = 0;
    vga_print("\nvelOS> ", 0x0A);
}

static Interpreter* vel_interp = NULL;

/**
 * shell_execute - Parse and run the current command buffer
 */
static void shell_execute() {
    vga_print("\n", 0x0F);
    
    if (cmd_index == 0) {
        shell_init();
        return;
    }

    /* Basic Commands */
    if (strcmp(cmd_buffer, "help") == 0) {
        vga_print("Available commands:\n", 0x0F);
        vga_print("  help  - Show this message\n", 0x07);
        vga_print("  clear - Clear the screen\n", 0x07);
        vga_print("  free  - Show free physical memory\n", 0x07);
        vga_print("  vel   - Run a Vel statement (e.g. 'vel let x = 5')\n", 0x07);
        vga_print("  echo  - Print text to screen\n", 0x07);
        vga_print("  ls    - List saved files on disk\n", 0x07);
        vga_print("  save  - Save script (e.g. 'save main.vel let x=1')\n", 0x07);
        vga_print("  run   - Load and execute a saved Vel script\n", 0x07);
        vga_print("  edit  - Open VelEdit text editor (e.g. 'edit main.vel')\n", 0x07);
    } 
    else if (strcmp(cmd_buffer, "clear") == 0) {
        vga_clear();
    } 
    else if (strcmp(cmd_buffer, "free") == 0 || strcmp(cmd_buffer, "mem") == 0) {
        uint32_t free_frames = pmm_free_frame_count();
        vga_print("Free physical frames: ", 0x0E);
        vga_print_hex(free_frames);
        vga_print("\nTotal Usable RAM: ~", 0x07);
        vga_print_hex((free_frames * 4096) / 1024 / 1024);
        vga_print(" MB\n", 0x07);
    }
    else if (strncmp(cmd_buffer, "echo ", 5) == 0) {
        vga_print(&cmd_buffer[5], 0x0F);
        vga_print("\n", 0x0F);
    }
    else if (strncmp(cmd_buffer, "vel ", 4) == 0) {
        char* source = &cmd_buffer[4];
        
        /* Initialize persistent interpreter if not already running */
        if (!vel_interp) {
            vel_interp = interpreter_init();
        }
        
        Lexer* lexer = lexer_init(source);
        Parser* parser = parser_init(lexer);
        ASTNode* ast = parse(parser);
        
        if (ast) {
            extern int is_script_running;
            is_script_running = 1;
            ReturnSignal ret_sig = {0, NULL};
            VelValue* result = eval(ast, vel_interp->global, &ret_sig);
            is_script_running = 0;
            
            if (result) {
                if (result->type != VAL_NULL) {
                    vga_print("=> ", 0x0A);
                    /* Basic output formatter since we don't have full printf %g yet */
                    if (result->type == VAL_NUMBER) {
                        int int_val = (int)result->number;
                        vga_print_hex(int_val); /* We'll just print hex for now */
                        vga_print("\n", 0x0F);
                    } else if (result->type == VAL_STRING) {
                        vga_print("\"", 0x0E);
                        vga_print(result->string, 0x0E);
                        vga_print("\"\n", 0x0E);
                    } else if (result->type == VAL_BOOL) {
                        vga_print(result->boolean ? "true\n" : "false\n", 0x0A);
                    }
                }
                free_value(result);
            }
            /* ONLY free the AST, not the environment! The environment is persistent. */
            free_ast(ast);
        } else {
            vga_print("Vel Parse Error: Invalid syntax\n", 0x0C);
        }
        
        /* Cleanup parser and lexer memory for this run */
        kfree(parser);
        kfree(lexer);
    }
    else if (strcmp(cmd_buffer, "ls") == 0) {
        fs_list_files();
    }
    else if (strncmp(cmd_buffer, "save ", 5) == 0) {
        /* Format: save filename data */
        char* args = &cmd_buffer[5];
        char* space = strchr(args, ' ');
        if (space) {
            *space = '\0';
            char* filename = args;
            char* data = space + 1;
            if (fs_write_file(filename, data) == 0) {
                vga_print("File saved successfully.\n", 0x0A);
            }
        } else {
            vga_print("Usage: save <filename> <vel code>\n", 0x0C);
        }
    }
    else if (strncmp(cmd_buffer, "run ", 4) == 0) {
        char* filename = &cmd_buffer[4];
        char* file_data = (char*)kmalloc(4096); /* Max 4KB file for now */
        int bytes = fs_read_file(filename, file_data);
        
        if (bytes > 0) {
            if (!vel_interp) vel_interp = interpreter_init();
            
            Lexer* lexer = lexer_init(file_data);
            Parser* parser = parser_init(lexer);
            ASTNode* ast = parse(parser);
            
            if (ast) {
                extern int is_script_running;
                is_script_running = 1;
                ReturnSignal ret_sig = {0, NULL};
                VelValue* result = eval(ast, vel_interp->global, &ret_sig);
                is_script_running = 0;
                if (result) free_value(result);
                free_ast(ast);
            } else {
                vga_print("Vel Parse Error in file\n", 0x0C);
            }
            kfree(parser);
            kfree(lexer);
        } else {
            vga_print("File not found or empty.\n", 0x0C);
        }
        kfree(file_data);
    }
    else if (strncmp(cmd_buffer, "edit ", 5) == 0) {
        editor_start(&cmd_buffer[5]);
        return; /* Do not call shell_init, as editor is now active */
    }
    else {
        vga_print("Unknown command: ", 0x0C);
        vga_print(cmd_buffer, 0x0C);
        vga_print("\n", 0x0F);
    }

    shell_init();
}

/**
 * shell_handle_char - Called by keyboard driver when a key is typed
 */
void shell_handle_char(char c) {
    if (c == '\n') {
        shell_execute();
    } 
    else if (c == '\b') {
        if (cmd_index > 0) {
            cmd_index--;
            cmd_buffer[cmd_index] = '\0';
        }
    } 
    else {
        if (cmd_index < CMD_BUFFER_SIZE - 1) {
            cmd_buffer[cmd_index++] = c;
            cmd_buffer[cmd_index] = '\0';
        }
    }
}
