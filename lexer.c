/* =========================================================
 *  lexer.c  —  Vel Language Lexer Implementation  (C99)
 *
 *  Compilation:
 *      gcc -Wall -std=c99 -o vel main.c lexer.c
 *
 *  Design overview
 *  ---------------
 *  The lexer is a hand-written, single-pass scanner.  It
 *  works directly on the raw source string without copying
 *  or buffering it.  All state lives in the Lexer struct:
 *
 *      source  ─ pointer to the original string (read-only)
 *      pos     ─ the index of the NEXT character to examine
 *      line    ─ 1-based line counter for error messages
 *
 *  Token values that need to be stored (numbers, strings,
 *  identifiers) are malloc-allocated copies so the caller
 *  (AST parser, test harness, etc.) can free them
 *  independently, even after the Lexer itself is freed.
 * ========================================================= */

#include "lexer.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>


/* =========================================================
 *  §1  LEXER LIFECYCLE
 * ========================================================= */

/**
 * lexer_init — allocate a Lexer and point it at `source`.
 *
 * We heap-allocate the Lexer struct itself so callers can
 * pass a pointer around freely.  `source` is never copied;
 * it must stay valid for the lifetime of this Lexer.
 *
 * Returns a pointer the caller must eventually free().
 */
Lexer *lexer_init(const char *source)
{
    Lexer *l = (Lexer *)malloc(sizeof(Lexer));
    if (!l) {
        fprintf(stderr, "lexer_init: out of memory\n");
        exit(EXIT_FAILURE);
    }
    l->source = source;
    l->pos    = 0;      /* start before the first character */
    l->line   = 1;      /* source lines are conventionally 1-based */
    return l;
}


/* =========================================================
 *  §2  LOW-LEVEL CHARACTER HELPERS
 *
 *  These are the only four functions that touch l->pos and
 *  l->source directly.  Everything else is built on them.
 * ========================================================= */

/**
 * advance — return the current character AND move past it.
 *
 * Pointer arithmetic note:
 *   l->source is a `const char *` that points to the first
 *   byte of the source string.  Adding l->pos gives us the
 *   address of the character at index pos.  Dereferencing
 *   that address (via the [] operator, which is sugar for
 *   *(l->source + l->pos)) gives the char value.
 *
 *   After saving it, we increment l->pos so the NEXT call
 *   to advance() or peek() sees the following character.
 *
 * Returns '\0' when the source is exhausted.
 */
static char advance(Lexer *l)
{
    /* '\0' terminates the string — do not move past it */
    if (l->source[l->pos] == '\0')
        return '\0';

    return l->source[l->pos++];   /* post-increment: read first, then advance */
}

/**
 * peek — return the current character WITHOUT consuming it.
 *
 * "Look but don't touch."  Used to decide what kind of token
 * is coming up without committing to reading it yet.
 */
static char peek(Lexer *l)
{
    return l->source[l->pos];     /* no change to l->pos */
}

/**
 * peek_next — return the character ONE position AHEAD of pos.
 *
 * Used for two-character operators like `->` and `|>`.
 * Example: when we see `-`, we peek_next() to decide whether
 * it's a TOKEN_MINUS or the start of TOKEN_ARROW.
 *
 * We must guard against reading past the NUL terminator;
 * if pos already sits on '\0', pos+1 would be undefined.
 */
static char peek_next(Lexer *l)
{
    if (l->source[l->pos] == '\0')
        return '\0';

    /*
     * l->source[l->pos]   is the CURRENT character (not yet consumed).
     * l->source[l->pos+1] is the character AFTER that.
     */
    return l->source[l->pos + 1];
}


/* =========================================================
 *  §3  WHITESPACE & COMMENT SKIPPING
 * ========================================================= */

/**
 * skip_whitespace — consume spaces, tabs, carriage returns,
 *                   and newlines, updating the line counter.
 *
 * We treat '\r\n' (Windows) the same as '\n' (Unix): only
 * the '\n' increments the line counter so we don't double-
 * count on Windows line endings.
 */
static void skip_whitespace(Lexer *l)
{
    while (true) {
        char c = peek(l);
        switch (c) {
            case ' ':
            case '\t':
            case '\r':
                advance(l);         /* consume, keep going */
                break;

            case '\n':
                advance(l);
                l->line++;          /* track source line for diagnostics */
                break;

            default:
                return;             /* non-whitespace: stop */
        }
    }
}

/**
 * skip_comment — consume a `--` line comment to end-of-line.
 *
 * Called AFTER the caller has already verified that the next
 * two characters are `--`.  We advance past both dashes and
 * then eat everything up to (but not including) the '\n'.
 * The newline itself is left for skip_whitespace() to handle
 * so the line counter is updated correctly.
 */
static void skip_comment(Lexer *l)
{
    advance(l); /* consume first  '-' */
    advance(l); /* consume second '-' */

    /* eat the rest of the line */
    while (peek(l) != '\n' && peek(l) != '\0')
        advance(l);
    /* we stop *before* '\n'; next call to skip_whitespace
       will consume it and increment l->line */
}


/* =========================================================
 *  §4  KEYWORD TABLE
 * ========================================================= */

/**
 * check_keyword — map an identifier string to its keyword
 *                 TokenType, or TOKEN_IDENT if not a keyword.
 *
 * This is a straightforward linear search through a table of
 * {string, TokenType} pairs.  For a small keyword set the
 * O(n) cost is negligible; a hash map would be an easy
 * optimisation later.
 */
static TokenType check_keyword(const char *word)
{
    /* Each entry maps the source spelling to a token type. */
    static const struct { const char *kw; TokenType type; } table[] = {
        { "let",    TOKEN_LET    },
        { "const",  TOKEN_CONST  },
        { "func",   TOKEN_FUNC   },
        { "return", TOKEN_RETURN },
        { "end",    TOKEN_END    },
        { "if",     TOKEN_IF     },
        { "elif",   TOKEN_ELIF   },
        { "else",   TOKEN_ELSE   },
        { "match",  TOKEN_MATCH  },
        { "loop",   TOKEN_LOOP   },
        { "repeat", TOKEN_REPEAT },
        { "while",  TOKEN_WHILE  },
        { "from",   TOKEN_FROM   },
        { "to",     TOKEN_TO     },
        { "show",   TOKEN_SHOW   },
        { "true",   TOKEN_BOOL   },
        { "false",  TOKEN_BOOL   },
    };

    /* Calculate the number of entries at compile time.
     * sizeof(table) / sizeof(table[0]) is a classic C idiom
     * that gives the element count of a stack-allocated array
     * without needing a separate length constant. */
    static const size_t TABLE_SIZE = sizeof(table) / sizeof(table[0]);

    for (size_t i = 0; i < TABLE_SIZE; i++) {
        if (strcmp(word, table[i].kw) == 0)
            return table[i].type;
    }
    return TOKEN_IDENT;
}


/* =========================================================
 *  §5  TOKEN CONSTRUCTORS  (internal helpers)
 *
 *  Two small helpers that construct Token values to keep
 *  next_token() clean.
 * ========================================================= */

/**
 * make_token — build a token with no heap-allocated value.
 * Used for operators and delimiters whose text is implicit.
 */
static Token make_token(TokenType type, int line)
{
    Token t;
    t.type  = type;
    t.value = NULL;   /* caller must NOT free() this */
    t.line  = line;
    return t;
}

/**
 * make_value_token — build a token and COPY `len` bytes
 *                    starting at `start` into a new heap buffer.
 *
 * Memory layout:
 *
 *   source buffer (read-only):
 *   [ ... | a | d | d | ... ]
 *                 ^           ^
 *               start      start+len
 *
 *   new heap buffer (owned by the Token):
 *   [ a | d | d | \0 ]
 *     ^
 *   returned as t.value
 *
 * We allocate len+1 bytes: `len` for the characters plus one
 * extra for the NUL terminator that makes it a valid C string.
 *
 * strncpy copies exactly `len` bytes; the explicit NUL write
 * at position [len] is not redundant — strncpy does NOT
 * guarantee a NUL terminator when `len` equals the buffer size.
 */
static Token make_value_token(TokenType type, const char *start,
                               int len, int line)
{
    Token t;
    t.type  = type;
    t.line  = line;

    t.value = (char *)malloc((size_t)len + 1);
    if (!t.value) {
        fprintf(stderr, "make_value_token: out of memory\n");
        exit(EXIT_FAILURE);
    }
    strncpy(t.value, start, (size_t)len);
    t.value[len] = '\0';   /* explicit NUL termination */
    return t;
}


/* =========================================================
 *  §6  MAIN SCANNING ENGINE
 * ========================================================= */

/**
 * next_token — consume the next token from the source and
 *              return it.
 *
 * Call this in a loop until it returns TOKEN_EOF.
 * The returned Token.value (when non-NULL) is heap-allocated;
 * the caller is responsible for free()-ing it.
 */
Token next_token(Lexer *l)
{
    /* ── 6a  Skip trivia (whitespace and comments) ───────── */
    /*
     * We loop rather than recurse so that sequences like
     *   "   -- comment\n   -- another\n   42"
     * are handled without stack growth.
     */
    while (true) {
        skip_whitespace(l);

        /*
         * A `--` comment is detected by peeking two characters
         * ahead.  We use peek() for the current char and
         * peek_next() for the one after, consuming neither
         * until we are sure it's actually a comment.
         */
        if (peek(l) == '-' && peek_next(l) == '-') {
            skip_comment(l);
            /* after the comment loop back to skip the '\n' */
        } else {
            break;
        }
    }

    int line = l->line;   /* snapshot the line for the token we're about to build */

    /* ── 6b  End of source ───────────────────────────────── */
    if (peek(l) == '\0')
        return make_token(TOKEN_EOF, line);

    char c = advance(l);  /* consume the first character of the new token */

    /* ── 6c  Integer literals ────────────────────────────── */
    /*
     * We record `start` as the address of the first digit.
     * Then we keep calling advance() while the upcoming char
     * is still a digit.  When we stop, `l->pos - start_pos`
     * is the exact length of the digit sequence.
     *
     * Why pointer arithmetic instead of a separate counter?
     * Because `start` already points into the source buffer;
     * subtracting it from the current position gives the
     * length without needing an extra variable to increment.
     */
    if (isdigit((unsigned char)c)) {
        int start_pos = l->pos - 1;            /* index of `c` in source */
        while (isdigit((unsigned char)peek(l)))
            advance(l);
        int len = l->pos - start_pos;
        return make_value_token(TOKEN_NUMBER,
                                l->source + start_pos, len, line);
    }

    /* ── 6d  String literals ─────────────────────────────── */
    /*
     * Vel strings are delimited by double quotes.  We have
     * already consumed the opening `"` via advance() above.
     *
     * We record the position AFTER the opening quote as the
     * content start, then scan forward until we hit the
     * closing `"` or run out of source.  The resulting token
     * value contains only the inner text (quotes stripped).
     *
     * Interpolation syntax  {name}  is intentionally left
     * raw inside the string value; a later AST pass will
     * parse it.
     */
    if (c == '"') {
        int start_pos = l->pos;                 /* first char INSIDE the quotes */
        while (peek(l) != '"' && peek(l) != '\0') {
            if (peek(l) == '\n') l->line++;     /* multi-line strings: track lines */
            advance(l);
        }
        int len = l->pos - start_pos;
        Token t = make_value_token(TOKEN_STRING,
                                   l->source + start_pos, len, line);
        if (peek(l) == '"')
            advance(l);                         /* consume closing quote */
        return t;
    }

    /* ── 6e  Identifiers and keywords ───────────────────── */
    /*
     * Vel identifiers start with a letter or underscore and
     * continue with letters, digits, or underscores.
     *
     * We use the same "record start, scan forward" pattern
     * as for numbers.  After collecting the raw text we run
     * it through check_keyword() to distinguish reserved
     * words from user-defined names.
     */
    if (isalpha((unsigned char)c) || c == '_') {
        int start_pos = l->pos - 1;
        while (isalnum((unsigned char)peek(l)) || peek(l) == '_')
            advance(l);
        int len = l->pos - start_pos;

        /*
         * We need a temporary NUL-terminated copy to pass to
         * check_keyword(), because the source buffer is not
         * guaranteed to have a NUL right after the identifier.
         *
         * Stack-allocate a VLA for small identifiers to avoid
         * an extra malloc/free pair.  C99 allows VLAs.
         */
        char word[len + 1];
        strncpy(word, l->source + start_pos, (size_t)len);
        word[len] = '\0';

        TokenType kw = check_keyword(word);

        /*
         * For keywords, the text is redundant (we can always
         * reconstruct it from the type), so value stays NULL.
         * For identifiers and booleans we keep the text
         * so that later stages can read the name / value.
         */
        if (kw == TOKEN_IDENT || kw == TOKEN_BOOL)
            return make_value_token(kw, l->source + start_pos, len, line);
        else
            return make_token(kw, line);
    }

    /* ── 6f  Operators and delimiters ───────────────────── */
    /*
     * Multi-character operators are handled by peeking at
     * the character AFTER the one we already consumed (`c`).
     * If it matches, we call advance() to swallow it too.
     * If it doesn't, we fall through to the single-char case.
     *
     * Example: we consumed `-`.
     *   peek(l) == '>'  →  advance(), return TOKEN_ARROW
     *   peek(l) != '>'  →  return TOKEN_MINUS
     */
    switch (c) {
        /* ── Arithmetic ──────────────────────────── */
        case '+': return make_token(TOKEN_PLUS,  line);
        case '*': return make_token(TOKEN_STAR,  line);
        case '/': return make_token(TOKEN_SLASH, line);

        /* ── Minus OR arrow (->) ─────────────────── */
        case '-':
            if (peek(l) == '>') {
                advance(l);            /* consume '>' */
                return make_token(TOKEN_ARROW, line);
            }
            return make_token(TOKEN_MINUS, line);

        /* ── Pipe-forward (|>) ───────────────────── */
        case '|':
            if (peek(l) == '>') {
                advance(l);            /* consume '>' */
                return make_token(TOKEN_PIPE, line);
            }
            /* bare `|` is not in Vel; fall through to UNKNOWN */
            break;

        /* ── Equality (==) vs. assignment (=) ────── */
        case '=':
            if (peek(l) == '=') {
                advance(l);
                return make_token(TOKEN_EQ, line);
            }
            return make_token(TOKEN_ASSIGN, line);

        /* ── Not-equal (!=) ──────────────────────── */
        case '!':
            if (peek(l) == '=') {
                advance(l);
                return make_token(TOKEN_NEQ, line);
            }
            break;   /* bare `!` → UNKNOWN */

        /* ── Comparison ──────────────────────────── */
        case '>': return make_token(TOKEN_GT, line);
        case '<': return make_token(TOKEN_LT, line);

        /* ── Delimiters ───────────────────────────── */
        case '(': return make_token(TOKEN_LPAREN,   line);
        case ')': return make_token(TOKEN_RPAREN,   line);
        case '[': return make_token(TOKEN_LBRACKET, line);
        case ']': return make_token(TOKEN_RBRACKET, line);
        case ':': return make_token(TOKEN_COLON,    line);
        case ',': return make_token(TOKEN_COMMA,    line);
        case '.': return make_token(TOKEN_DOT,      line);

        default:
            break;
    }

    /* ── 6g  Unknown character ───────────────────────────── */
    /*
     * We store the unknown character as a one-byte string so
     * error reporting can show the offending character.
     */
    return make_value_token(TOKEN_UNKNOWN, &c, 1, line);
}


/* =========================================================
 *  §7  DIAGNOSTIC PRINTING
 * ========================================================= */

/**
 * token_type_name — map a TokenType to a readable string.
 *
 * Kept as a file-scope helper so print_token() stays clean.
 * The returned pointer is into a string literal (static
 * storage) and must NOT be freed.
 */
static const char *token_type_name(TokenType t)
{
    switch (t) {
        case TOKEN_NUMBER:    return "NUMBER";
        case TOKEN_STRING:    return "STRING";
        case TOKEN_BOOL:      return "BOOL";
        case TOKEN_IDENT:     return "IDENT";
        case TOKEN_LET:       return "LET";
        case TOKEN_CONST:     return "CONST";
        case TOKEN_FUNC:      return "FUNC";
        case TOKEN_RETURN:    return "RETURN";
        case TOKEN_END:       return "END";
        case TOKEN_IF:        return "IF";
        case TOKEN_ELIF:      return "ELIF";
        case TOKEN_ELSE:      return "ELSE";
        case TOKEN_MATCH:     return "MATCH";
        case TOKEN_LOOP:      return "LOOP";
        case TOKEN_REPEAT:    return "REPEAT";
        case TOKEN_WHILE:     return "WHILE";
        case TOKEN_FROM:      return "FROM";
        case TOKEN_TO:        return "TO";
        case TOKEN_SHOW:      return "SHOW";
        case TOKEN_PLUS:      return "PLUS";
        case TOKEN_MINUS:     return "MINUS";
        case TOKEN_STAR:      return "STAR";
        case TOKEN_SLASH:     return "SLASH";
        case TOKEN_EQ:        return "EQ";
        case TOKEN_NEQ:       return "NEQ";
        case TOKEN_GT:        return "GT";
        case TOKEN_LT:        return "LT";
        case TOKEN_ASSIGN:    return "ASSIGN";
        case TOKEN_ARROW:     return "ARROW";
        case TOKEN_PIPE:      return "PIPE";
        case TOKEN_LPAREN:    return "LPAREN";
        case TOKEN_RPAREN:    return "RPAREN";
        case TOKEN_LBRACKET:  return "LBRACKET";
        case TOKEN_RBRACKET:  return "RBRACKET";
        case TOKEN_COLON:     return "COLON";
        case TOKEN_COMMA:     return "COMMA";
        case TOKEN_DOT:       return "DOT";
        case TOKEN_UNDERSCORE:return "UNDERSCORE";
        case TOKEN_EOF:       return "EOF";
        case TOKEN_UNKNOWN:   return "UNKNOWN";
        default:              return "???";
    }
}

/**
 * print_token — print one token to stdout in the format:
 *   [LINE:  3] ARROW         ->
 *   [LINE:  4] IDENT         add
 */
void print_token(Token t)
{
    if (t.value)
        printf("[LINE:%3d] %-14s  \"%s\"\n",
               t.line, token_type_name(t.type), t.value);
    else
        printf("[LINE:%3d] %s\n",
               t.line, token_type_name(t.type));
}
