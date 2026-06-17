#ifndef LEXER_H
#define LEXER_H

/* =========================================================
 *  lexer.h  —  Vel Language Lexer Header
 *  Defines all token types, the Token struct, and the
 *  Lexer struct. This header is included by both lexer.c
 *  and main.c so both translation units share one definition.
 * ========================================================= */

/* ---------------------------------------------------------
 *  Token Types
 *  Each variant represents one recognisable unit of the
 *  Vel grammar. The ordering is purely for readability.
 * --------------------------------------------------------- */
typedef enum {
    /* Literals */
    TOKEN_NUMBER,       /* 42, 0, 999 */
    TOKEN_STRING,       /* "hello {name}" — raw content, not yet interpolated */
    TOKEN_BOOL,         /* true, false */
    TOKEN_IDENT,        /* any user-defined name */

    /* Keywords */
    TOKEN_LET,          /* let   */
    TOKEN_CONST,        /* const */
    TOKEN_FUNC,         /* func  */
    TOKEN_RETURN,       /* return */
    TOKEN_END,          /* end   */
    TOKEN_IF,           /* if    */
    TOKEN_ELIF,         /* elif  */
    TOKEN_ELSE,         /* else  */
    TOKEN_MATCH,        /* match */
    TOKEN_LOOP,         /* loop  */
    TOKEN_REPEAT,       /* repeat */
    TOKEN_WHILE,        /* while */
    TOKEN_FROM,         /* from  */
    TOKEN_TO,           /* to    */
    TOKEN_SHOW,         /* show  */

    /* Arithmetic operators */
    TOKEN_PLUS,         /* +  */
    TOKEN_MINUS,        /* -  */
    TOKEN_STAR,         /* *  */
    TOKEN_SLASH,        /* /  */

    /* Comparison operators */
    TOKEN_EQ,           /* == */
    TOKEN_NEQ,          /* != */
    TOKEN_GT,           /* >  */
    TOKEN_LT,           /* <  */

    /* Assignment & special operators */
    TOKEN_ASSIGN,       /* =   */
    TOKEN_ARROW,        /* ->  */
    TOKEN_PIPE,         /* |>  */

    /* Delimiters */
    TOKEN_LPAREN,       /* (  */
    TOKEN_RPAREN,       /* )  */
    TOKEN_LBRACKET,     /* [  */
    TOKEN_RBRACKET,     /* ]  */
    TOKEN_COLON,        /* :  */
    TOKEN_COMMA,        /* ,  */
    TOKEN_DOT,          /* .  */
    TOKEN_UNDERSCORE,   /* _  (wildcard in match) */

    /* Sentinel / error */
    TOKEN_EOF,          /* end of source */
    TOKEN_UNKNOWN       /* any character we don't recognise */
} TokenType;

/* ---------------------------------------------------------
 *  Token
 *  Carries the token type, a heap-allocated string value,
 *  and the source line where the token was found.
 *
 *  Memory contract:
 *    • `value` is NULL for tokens whose text is implicit
 *      (operators, delimiters, EOF).
 *    • `value` is malloc-allocated for NUMBER, STRING,
 *      IDENT, BOOL, and UNKNOWN tokens.
 *    • The CALLER is responsible for free()-ing value.
 * --------------------------------------------------------- */
typedef struct {
    TokenType  type;
    char      *value;   /* heap-allocated, or NULL — see above */
    int        line;
} Token;

/* ---------------------------------------------------------
 *  Lexer
 *  Holds the source text as a read-only pointer, the
 *  current byte offset `pos`, and the current line number
 *  for error reporting.
 * --------------------------------------------------------- */
typedef struct {
    const char *source; /* points into caller-owned memory, never written */
    int         pos;    /* index of the next character to be consumed */
    int         line;   /* 1-based line counter, incremented on '\n' */
} Lexer;

/* ---------------------------------------------------------
 *  Public API
 * --------------------------------------------------------- */

/**
 * lexer_init — allocate and return a new Lexer for `source`.
 * The caller must ensure `source` outlives the Lexer.
 */
Lexer *lexer_init(const char *source);

/**
 * next_token — consume and return the next token.
 * The caller owns Token.value and must free() it when done.
 */
Token  next_token(Lexer *l);

/**
 * print_token — write a human-readable description of `t`
 * to stdout (type name, value if any, line number).
 */
void   print_token(Token t);

#endif /* LEXER_H */
