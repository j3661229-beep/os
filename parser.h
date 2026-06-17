#ifndef PARSER_H
#define PARSER_H

/* =========================================================
 *  parser.h  —  Vel Language Parser & AST Header  (C99)
 *
 *  Declares the NodeType enum, ASTNode struct, Parser struct,
 *  and the public API that main.c (or a future evaluator)
 *  will call.  All implementation lives in parser.c.
 * ========================================================= */

#include "lexer.h"   /* Token, TokenType, Lexer */


/* =========================================================
 *  §1  AST NODE TYPES
 *
 *  Every syntactic construct in Vel maps to one NodeType.
 *  The ordering groups them by category for readability;
 *  the numeric values themselves are never relied upon.
 * ========================================================= */
typedef enum {
    /* Top-level container */
    NODE_PROGRAM,        /* root of every parse tree             */

    /* Variable declarations */
    NODE_LET,            /* let  <name> = <expr>                 */
    NODE_CONST,          /* const <name> = <expr>                */
    NODE_ASSIGN,         /* <name> = <expr>  (re-assignment)     */

    /* Functions */
    NODE_FUNC_DEF,       /* func <name>(<params>) -> <type>: ... end */
    NODE_RETURN,         /* return <expr>                        */

    /* Conditionals */
    NODE_IF,             /* if <cond>: <block> [elif…] [else] end */
    NODE_ELIF,           /* elif <cond>: <block>                 */
    NODE_ELSE,           /* else: <block>                        */

    /* Pattern matching */
    NODE_MATCH,          /* match <expr>: <arms> end             */
    NODE_MATCH_ARM,      /* <pattern> -> <expr>                  */

    /* Loops */
    NODE_LOOP_RANGE,     /* loop <var> from <start> to <end>: .. end */
    NODE_REPEAT_WHILE,   /* repeat while <cond>: <block> end     */

    /* Expressions */
    NODE_BINOP,          /* left <op> right                      */
    NODE_UNOP,           /* -<expr>                              */
    NODE_CALL,           /* <callee>(<args…>)                    */
    NODE_PIPELINE,       /* <expr> |> <call>                     */
    NODE_INDEX,          /* <expr>[<index>]                      */

    /* Literals & references */
    NODE_IDENT,          /* variable / function name             */
    NODE_NUMBER,         /* integer literal                      */
    NODE_STRING,         /* string literal (may contain {…})     */
    NODE_BOOL,           /* true / false                         */
    NODE_ARRAY,          /* [elem, elem, …]                      */

    /* Misc */
    NODE_SHOW,           /* show(<expr>)                         */
    NODE_BLOCK           /* sequence of statements               */
} NodeType;


/* =========================================================
 *  §2  AST NODE STRUCT
 *
 *  A single, uniform struct that represents every node in
 *  the tree.  Not all fields are used by every node type;
 *  the comments below document which fields each type uses.
 *
 *  Memory contract
 *  ───────────────
 *  • `value`    — heap-allocated string (strdup), or NULL.
 *  • `left`     — heap-allocated child node, or NULL.
 *  • `right`    — heap-allocated child node, or NULL.
 *  • `children` — heap-allocated array of child pointers.
 *                 Each pointer in the array is also heap-
 *                 allocated.  Length is stored in child_count.
 *
 *  free_ast() recursively releases the entire subtree.
 * ========================================================= */
typedef struct ASTNode {
    NodeType         type;

    /*
     * value — textual payload for this node:
     *   NODE_IDENT       → identifier name  ("x", "add")
     *   NODE_NUMBER      → digit string     ("42")
     *   NODE_STRING      → raw string body  ("Hello, {name}!")
     *   NODE_BOOL        → "true" or "false"
     *   NODE_BINOP       → operator symbol  ("+", "-", "==", …)
     *   NODE_UNOP        → operator symbol  ("-")
     *   NODE_FUNC_DEF    → function name
     *   NODE_LET/CONST   → variable name
     *   NODE_ASSIGN      → variable name
     *   NODE_LOOP_RANGE  → loop variable name
     *   NODE_MATCH_ARM   → pattern literal  ("1", "_", "42")
     *   NODE_CALL        → callee name
     *   (all others)     → NULL
     */
    char            *value;

    /*
     * Binary / unary expression operands.
     *   NODE_BINOP    left  = LHS,  right = RHS
     *   NODE_UNOP     right = operand (left unused)
     *   NODE_LET      right = initialiser expression
     *   NODE_CONST    right = initialiser expression
     *   NODE_ASSIGN   right = new-value expression
     *   NODE_RETURN   right = returned expression
     *   NODE_SHOW     right = argument expression
     *   NODE_IF       left  = condition
     *   NODE_ELIF     left  = condition
     *   NODE_MATCH    left  = scrutinee expression
     *   NODE_MATCH_ARM right = result expression
     *   NODE_LOOP_RANGE   left = from-expr, right = to-expr
     *   NODE_REPEAT_WHILE left = condition
     *   NODE_PIPELINE left  = LHS expr, right = RHS call
     *   NODE_INDEX    left  = array expr, right = index expr
     */
    struct ASTNode  *left;
    struct ASTNode  *right;

    /*
     * children — variable-length list of child nodes.
     *   NODE_PROGRAM    → top-level statements
     *   NODE_BLOCK      → block statements
     *   NODE_FUNC_DEF   → [param₀, param₁, …, return_type_node, body_block]
     *   NODE_CALL       → argument expressions
     *   NODE_ARRAY      → element expressions
     *   NODE_IF         → [if_block, elif₀, elif₁, …, else_node]
     *   NODE_MATCH      → match arm nodes
     */
    struct ASTNode **children;
    int              child_count;

    int              line;    /* source line, for error messages */
} ASTNode;


/* =========================================================
 *  §3  PARSER STRUCT
 *
 *  The parser is a single-token look-ahead (LL(1)) recursive-
 *  descent parser.  It holds:
 *
 *    lexer    — pointer to the already-initialised Lexer.
 *    current  — the token we are examining RIGHT NOW but have
 *               NOT yet consumed (i.e. our one look-ahead).
 *    previous — the last token we DID consume (useful for
 *               grabbing values after an expect() call).
 * ========================================================= */
typedef struct {
    Lexer *lexer;
    Token  current;    /* look-ahead token — not yet consumed  */
    Token  previous;   /* most recently consumed token         */
} Parser;


/* =========================================================
 *  §4  PUBLIC API
 * ========================================================= */

/**
 * parser_init — allocate and return a Parser primed with the
 * first token from `lexer`.  The caller owns the returned
 * pointer and must free() it when done.
 */
Parser  *parser_init(Lexer *lexer);

/**
 * parse — run the full parse and return the NODE_PROGRAM root.
 * The caller owns the returned tree and must call free_ast()
 * when it is no longer needed.
 */
ASTNode *parse(Parser *p);

/**
 * free_ast — recursively free an entire AST subtree.
 * Passing NULL is safe (no-op).
 */
void     free_ast(ASTNode *node);

/**
 * print_ast — pretty-print the AST to stdout with indentation.
 * `indent` is the initial indentation level (pass 0 at root).
 */
void     print_ast(ASTNode *node, int indent);

#endif /* PARSER_H */
