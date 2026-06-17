/* ==============================================================================
 * web_main.c - Emscripten Entry Point for the Vel Web Playground
 *
 * This file provides the bridge between JavaScript and the C-based Vel
 * interpreter. When compiled with Emscripten, the `run_vel_code` function
 * is exported and callable from JS via `Module.ccall()`.
 *
 * CRUCIAL: Emscripten automatically captures all printf/fprintf output and
 * routes it through Module.print (stdout) and Module.printErr (stderr).
 * We override those in app.js to redirect output to our HTML output box.
 * ============================================================================== */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <emscripten.h>

/* Include the Vel engine headers */
#include "../kernel/vel/lexer.h"
#include "../kernel/vel/parser.h"
#include "../kernel/vel/interpreter.h"

/* Dummy globals that the interpreter may reference */
int is_script_running = 0;

/* Keep a persistent interpreter instance so variables survive between runs */
static Interpreter* vel_interp = NULL;

/**
 * run_vel_code - Execute a Vel script string.
 * 
 * Called from JavaScript via Module.ccall("run_vel_code", ...).
 * All printf output from the C interpreter is captured by Emscripten's
 * Module.print/Module.printErr hooks and displayed in the web UI.
 */
EMSCRIPTEN_KEEPALIVE
void run_vel_code(const char* code) {
    /* Initialize the interpreter on first run */
    if (!vel_interp) {
        vel_interp = interpreter_init();
    }
    
    /* Run the Vel pipeline: Lex -> Parse -> Evaluate */
    Lexer* lexer = lexer_init(code);
    Parser* parser = parser_init(lexer);
    ASTNode* ast = parse(parser);
    
    if (ast) {
        is_script_running = 1;
        ReturnSignal ret_sig = {0, NULL};
        VelValue* result = eval(ast, vel_interp->global, &ret_sig);
        is_script_running = 0;
        if (result) free_value(result);
        free_ast(ast);
    } else {
        printf("[Parse Error] Could not parse the script.\n");
    }
    
    free(parser);
    free(lexer);
}

/**
 * reset_vel_env - Reset the interpreter environment.
 * Useful for a "Clear & Reset" button in the web UI.
 */
EMSCRIPTEN_KEEPALIVE
void reset_vel_env(void) {
    if (vel_interp) {
        interpreter_free(vel_interp);
        vel_interp = NULL;
    }
    vel_interp = interpreter_init();
    printf("Environment reset.\n");
}
