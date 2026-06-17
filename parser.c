#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* =========================================================
 *  §1  AST NODE ALLOCATION
 * ========================================================= */

static ASTNode* make_node(NodeType type, int line) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    if (!node) {
        fprintf(stderr, "Out of memory creating ASTNode\n");
        exit(EXIT_FAILURE);
    }
    node->type = type;
    node->value = NULL;
    node->left = NULL;
    node->right = NULL;
    node->children = NULL;
    node->child_count = 0;
    node->line = line;
    return node;
}

static void add_child(ASTNode* parent, ASTNode* child) {
    if (!child) return;
    parent->child_count++;
    parent->children = (ASTNode**)realloc(parent->children, parent->child_count * sizeof(ASTNode*));
    if (!parent->children) {
        fprintf(stderr, "Out of memory reallocating children\n");
        exit(EXIT_FAILURE);
    }
    parent->children[parent->child_count - 1] = child;
}

/* =========================================================
 *  §2  PARSER CORE
 * ========================================================= */

Parser* parser_init(Lexer* lexer) {
    Parser* p = (Parser*)malloc(sizeof(Parser));
    if (!p) {
        fprintf(stderr, "Out of memory creating Parser\n");
        exit(EXIT_FAILURE);
    }
    p->lexer = lexer;
    p->previous.type = TOKEN_EOF;
    p->previous.value = NULL;
    p->current = next_token(lexer);
    return p;
}

static Token advance(Parser* p) {
    p->previous = p->current;
    p->current = next_token(p->lexer);
    return p->previous;
}

static int check(Parser* p, TokenType t) {
    return p->current.type == t;
}

static int match(Parser* p, TokenType t) {
    if (check(p, t)) {
        advance(p);
        return 1;
    }
    return 0;
}

static Token expect(Parser* p, TokenType t, const char* err) {
    if (check(p, t)) {
        return advance(p);
    }
    fprintf(stderr, "Error at line %d: %s\n", p->current.line, err);
    exit(EXIT_FAILURE);
}

static char* dup_value(const char* val) {
    if (!val) return NULL;
    char* copy = (char*)malloc(strlen(val) + 1);
    if (!copy) exit(EXIT_FAILURE);
    strcpy(copy, val);
    return copy;
}

/* =========================================================
 *  §3  FORWARD DECLARATIONS
 * ========================================================= */

static ASTNode* parse_expression(Parser* p);
static ASTNode* parse_statement(Parser* p);
static ASTNode* parse_block(Parser* p);

/* =========================================================
 *  §4  EXPRESSION PARSING (PRECEDENCE)
 * ========================================================= */

/* Level 7: primary literals, (expr), func calls */
static ASTNode* parse_primary(Parser* p) {
    int line = p->current.line;
    ASTNode* n = NULL;
    if (match(p, TOKEN_NUMBER)) {
        n = make_node(NODE_NUMBER, line);
        n->value = dup_value(p->previous.value);
    } else if (match(p, TOKEN_STRING)) {
        n = make_node(NODE_STRING, line);
        n->value = dup_value(p->previous.value);
    } else if (match(p, TOKEN_BOOL)) {
        n = make_node(NODE_BOOL, line);
        n->value = dup_value(p->previous.value);
    } else if (match(p, TOKEN_IDENT)) {
        n = make_node(NODE_IDENT, line);
        n->value = dup_value(p->previous.value);
    } else if (match(p, TOKEN_LBRACKET)) {
        n = make_node(NODE_ARRAY, line);
        if (!check(p, TOKEN_RBRACKET)) {
            do {
                add_child(n, parse_expression(p));
            } while (match(p, TOKEN_COMMA));
        }
        expect(p, TOKEN_RBRACKET, "Expected ']' after array elements");
    } else if (match(p, TOKEN_LPAREN)) {
        n = parse_expression(p);
        expect(p, TOKEN_RPAREN, "Expected ')' after expression");
    } else {
        fprintf(stderr, "Error at line %d: Expected expression\n", line);
        exit(EXIT_FAILURE);
    }

    while (1) {
        if (match(p, TOKEN_LPAREN)) {
            if (n->type != NODE_IDENT) {
                fprintf(stderr, "Error at line %d: Only identifiers can be called\n", p->previous.line);
                exit(EXIT_FAILURE);
            }
            ASTNode* call_node = make_node(NODE_CALL, p->previous.line);
            call_node->value = n->value;
            free(n);
            if (!check(p, TOKEN_RPAREN)) {
                do {
                    add_child(call_node, parse_expression(p));
                } while (match(p, TOKEN_COMMA));
            }
            expect(p, TOKEN_RPAREN, "Expected ')' after arguments");
            n = call_node;
        } else if (match(p, TOKEN_LBRACKET)) {
            ASTNode* index_node = make_node(NODE_INDEX, p->previous.line);
            index_node->left = n;
            index_node->right = parse_expression(p);
            expect(p, TOKEN_RBRACKET, "Expected ']' after index");
            n = index_node;
        } else {
            break;
        }
    }
    return n;
}

/* Array indexing, etc (if needed, adding index as it was in the ASTNode but not strictly in rules, we'll keep it simple if not needed)
   Wait, the rules didn't explicitly say index in precedence, I will omit array indexing to stick exactly to the provided precedence table or include it in primary.
   Let's stick to the prompt's precedence table. */

/* Level 6: unary - (negate) */
static ASTNode* parse_unary(Parser* p) {
    if (match(p, TOKEN_MINUS)) {
        int line = p->previous.line;
        ASTNode* n = make_node(NODE_UNOP, line);
        n->value = dup_value("-");
        n->right = parse_unary(p);
        return n;
    }
    return parse_primary(p);
}

/* Level 5: factor * / */
static ASTNode* parse_factor(Parser* p) {
    ASTNode* expr = parse_unary(p);
    while (check(p, TOKEN_STAR) || check(p, TOKEN_SLASH)) {
        advance(p);
        ASTNode* n = make_node(NODE_BINOP, p->previous.line);
        n->value = dup_value(p->previous.type == TOKEN_STAR ? "*" : "/");
        n->left = expr;
        n->right = parse_unary(p);
        expr = n;
    }
    return expr;
}

/* Level 4: term + - */
static ASTNode* parse_term(Parser* p) {
    ASTNode* expr = parse_factor(p);
    while (check(p, TOKEN_PLUS) || check(p, TOKEN_MINUS)) {
        advance(p);
        ASTNode* n = make_node(NODE_BINOP, p->previous.line);
        n->value = dup_value(p->previous.type == TOKEN_PLUS ? "+" : "-");
        n->left = expr;
        n->right = parse_factor(p);
        expr = n;
    }
    return expr;
}

/* Level 3: comparison > < */
static ASTNode* parse_comparison(Parser* p) {
    ASTNode* expr = parse_term(p);
    while (check(p, TOKEN_GT) || check(p, TOKEN_LT)) {
        advance(p);
        ASTNode* n = make_node(NODE_BINOP, p->previous.line);
        n->value = dup_value(p->previous.type == TOKEN_GT ? ">" : "<");
        n->left = expr;
        n->right = parse_term(p);
        expr = n;
    }
    return expr;
}

/* Level 2: equality == != */
static ASTNode* parse_equality(Parser* p) {
    ASTNode* expr = parse_comparison(p);
    while (check(p, TOKEN_EQ) || check(p, TOKEN_NEQ)) {
        advance(p);
        ASTNode* n = make_node(NODE_BINOP, p->previous.line);
        n->value = dup_value(p->previous.type == TOKEN_EQ ? "==" : "!=");
        n->left = expr;
        n->right = parse_comparison(p);
        expr = n;
    }
    return expr;
}

/* Level 1: pipeline |> */
static ASTNode* parse_pipeline(Parser* p) {
    ASTNode* expr = parse_equality(p);
    while (match(p, TOKEN_PIPE)) {
        ASTNode* n = make_node(NODE_PIPELINE, p->previous.line);
        n->left = expr;
        /* The right side should be a function call */
        n->right = parse_equality(p);
        expr = n;
    }
    return expr;
}

static ASTNode* parse_expression(Parser* p) {
    return parse_pipeline(p);
}

/* =========================================================
 *  §5  STATEMENT PARSING
 * ========================================================= */

static ASTNode* parse_block(Parser* p) {
    ASTNode* block = make_node(NODE_BLOCK, p->current.line);
    while (!check(p, TOKEN_END) && !check(p, TOKEN_EOF) && 
           !check(p, TOKEN_ELIF) && !check(p, TOKEN_ELSE)) {
        add_child(block, parse_statement(p));
    }
    return block;
}

static ASTNode* parse_let(Parser* p) {
    int line = p->previous.line;
    expect(p, TOKEN_IDENT, "Expected variable name after 'let'");
    Token name = p->previous;
    expect(p, TOKEN_ASSIGN, "Expected '=' after variable name");
    ASTNode* n = make_node(NODE_LET, line);
    n->value = dup_value(name.value);
    n->right = parse_expression(p);
    return n;
}

static ASTNode* parse_const(Parser* p) {
    int line = p->previous.line;
    expect(p, TOKEN_IDENT, "Expected variable name after 'const'");
    Token name = p->previous;
    expect(p, TOKEN_ASSIGN, "Expected '=' after variable name");
    ASTNode* n = make_node(NODE_CONST, line);
    n->value = dup_value(name.value);
    n->right = parse_expression(p);
    return n;
}

static ASTNode* parse_func(Parser* p) {
    int line = p->previous.line;
    expect(p, TOKEN_IDENT, "Expected function name");
    Token name = p->previous;
    
    ASTNode* n = make_node(NODE_FUNC_DEF, line);
    n->value = dup_value(name.value);
    
    expect(p, TOKEN_LPAREN, "Expected '(' after function name");
    
    /* struct says NODE_FUNC_DEF -> [param0, param1, ..., return_type_node, body_block] */
    if (!check(p, TOKEN_RPAREN)) {
        do {
            expect(p, TOKEN_IDENT, "Expected parameter name");
            ASTNode* param = make_node(NODE_IDENT, p->previous.line);
            param->value = dup_value(p->previous.value);
            add_child(n, param);
        } while (match(p, TOKEN_COMMA));
    }
    expect(p, TOKEN_RPAREN, "Expected ')' after parameters");
    
    expect(p, TOKEN_ARROW, "Expected '->' for return type");
    expect(p, TOKEN_IDENT, "Expected return type");
    ASTNode* ret_type = make_node(NODE_IDENT, p->previous.line);
    ret_type->value = dup_value(p->previous.value);
    add_child(n, ret_type);
    
    expect(p, TOKEN_COLON, "Expected ':' before function body");
    
    add_child(n, parse_block(p));
    expect(p, TOKEN_END, "Expected 'end' after function body");
    
    return n;
}

static ASTNode* parse_if(Parser* p) {
    int line = p->previous.line;
    ASTNode* n = make_node(NODE_IF, line);
    n->left = parse_expression(p);
    expect(p, TOKEN_COLON, "Expected ':' after if condition");
    
    add_child(n, parse_block(p)); /* if_block */
    
    while (match(p, TOKEN_ELIF)) {
        ASTNode* elif_node = make_node(NODE_ELIF, p->previous.line);
        elif_node->left = parse_expression(p);
        expect(p, TOKEN_COLON, "Expected ':' after elif condition");
        add_child(elif_node, parse_block(p)); /* Note: adding elif_node to n's children */
        add_child(n, elif_node);
    }
    
    if (match(p, TOKEN_ELSE)) {
        ASTNode* else_node = make_node(NODE_ELSE, p->previous.line);
        expect(p, TOKEN_COLON, "Expected ':' after 'else'");
        add_child(else_node, parse_block(p));
        add_child(n, else_node);
    }
    
    expect(p, TOKEN_END, "Expected 'end' after if statement");
    return n;
}

static ASTNode* parse_match(Parser* p) {
    int line = p->previous.line;
    ASTNode* n = make_node(NODE_MATCH, line);
    n->left = parse_expression(p);
    expect(p, TOKEN_COLON, "Expected ':' after match expression");
    
    while (!check(p, TOKEN_END) && !check(p, TOKEN_EOF)) {
        ASTNode* arm = make_node(NODE_MATCH_ARM, p->current.line);
        
        /* Left side of arm can be literal or wildcard _ */
        if (match(p, TOKEN_NUMBER) || match(p, TOKEN_STRING) || match(p, TOKEN_BOOL) || match(p, TOKEN_IDENT) || match(p, TOKEN_UNDERSCORE)) {
            if (p->previous.type == TOKEN_UNDERSCORE) {
                arm->value = dup_value("_");
            } else {
                arm->value = dup_value(p->previous.value);
            }
        } else {
            fprintf(stderr, "Error at line %d: Expected pattern in match arm\n", p->current.line);
            exit(EXIT_FAILURE);
        }
        
        expect(p, TOKEN_ARROW, "Expected '->' after match pattern");
        arm->right = parse_statement(p);
        add_child(n, arm);
    }
    
    expect(p, TOKEN_END, "Expected 'end' after match block");
    return n;
}

static ASTNode* parse_loop(Parser* p) {
    int line = p->previous.line;
    ASTNode* n = make_node(NODE_LOOP_RANGE, line);
    expect(p, TOKEN_IDENT, "Expected loop variable name");
    n->value = dup_value(p->previous.value);
    
    expect(p, TOKEN_FROM, "Expected 'from' in loop");
    n->left = parse_expression(p);
    
    expect(p, TOKEN_TO, "Expected 'to' in loop");
    n->right = parse_expression(p);
    
    expect(p, TOKEN_COLON, "Expected ':' before loop body");
    add_child(n, parse_block(p));
    expect(p, TOKEN_END, "Expected 'end' after loop body");
    return n;
}

static ASTNode* parse_repeat(Parser* p) {
    int line = p->previous.line;
    ASTNode* n = make_node(NODE_REPEAT_WHILE, line);
    expect(p, TOKEN_WHILE, "Expected 'while' after repeat");
    n->left = parse_expression(p);
    expect(p, TOKEN_COLON, "Expected ':' before repeat body");
    add_child(n, parse_block(p));
    expect(p, TOKEN_END, "Expected 'end' after repeat body");
    return n;
}

static ASTNode* parse_show(Parser* p) {
    int line = p->previous.line;
    ASTNode* n = make_node(NODE_SHOW, line);
    expect(p, TOKEN_LPAREN, "Expected '(' after 'show'");
    n->right = parse_expression(p);
    expect(p, TOKEN_RPAREN, "Expected ')' after show arguments");
    return n;
}

static ASTNode* parse_return(Parser* p) {
    int line = p->previous.line;
    ASTNode* n = make_node(NODE_RETURN, line);
    n->right = parse_expression(p);
    return n;
}

static ASTNode* parse_statement(Parser* p) {
    if (match(p, TOKEN_LET)) return parse_let(p);
    if (match(p, TOKEN_CONST)) return parse_const(p);
    if (match(p, TOKEN_FUNC)) return parse_func(p);
    if (match(p, TOKEN_IF)) return parse_if(p);
    if (match(p, TOKEN_MATCH)) return parse_match(p);
    if (match(p, TOKEN_LOOP)) return parse_loop(p);
    if (match(p, TOKEN_REPEAT)) return parse_repeat(p);
    if (match(p, TOKEN_SHOW)) return parse_show(p);
    if (match(p, TOKEN_RETURN)) return parse_return(p);
    
    /* Expression statement or Assignment */
    ASTNode* expr = parse_expression(p);
    if (expr->type == NODE_IDENT && match(p, TOKEN_ASSIGN)) {
        ASTNode* assign = make_node(NODE_ASSIGN, p->previous.line);
        assign->value = dup_value(expr->value);
        assign->right = parse_expression(p);
        free_ast(expr);
        return assign;
    }
    return expr;
}

/* =========================================================
 *  §6  ENTRY POINT
 * ========================================================= */

ASTNode* parse(Parser* p) {
    ASTNode* prog = make_node(NODE_PROGRAM, 1);
    while (!check(p, TOKEN_EOF)) {
        add_child(prog, parse_statement(p));
    }
    return prog;
}

/* =========================================================
 *  §7  AST UTILS
 * ========================================================= */

void free_ast(ASTNode* node) {
    if (!node) return;
    free(node->value);
    free_ast(node->left);
    free_ast(node->right);
    for (int i = 0; i < node->child_count; i++) {
        free_ast(node->children[i]);
    }
    free(node->children);
    free(node);
}

static const char* node_type_name(NodeType t) {
    switch(t) {
        case NODE_PROGRAM: return "PROGRAM";
        case NODE_LET: return "LET";
        case NODE_CONST: return "CONST";
        case NODE_ASSIGN: return "ASSIGN";
        case NODE_FUNC_DEF: return "FUNC_DEF";
        case NODE_RETURN: return "RETURN";
        case NODE_IF: return "IF";
        case NODE_ELIF: return "ELIF";
        case NODE_ELSE: return "ELSE";
        case NODE_MATCH: return "MATCH";
        case NODE_MATCH_ARM: return "MATCH_ARM";
        case NODE_LOOP_RANGE: return "LOOP_RANGE";
        case NODE_REPEAT_WHILE: return "REPEAT_WHILE";
        case NODE_BINOP: return "BINOP";
        case NODE_UNOP: return "UNOP";
        case NODE_CALL: return "CALL";
        case NODE_PIPELINE: return "PIPELINE";
        case NODE_INDEX: return "INDEX";
        case NODE_IDENT: return "IDENT";
        case NODE_NUMBER: return "NUMBER";
        case NODE_STRING: return "STRING";
        case NODE_BOOL: return "BOOL";
        case NODE_ARRAY: return "ARRAY";
        case NODE_SHOW: return "SHOW";
        case NODE_BLOCK: return "BLOCK";
        default: return "UNKNOWN";
    }
}

void print_ast(ASTNode* node, int indent) {
    if (!node) return;
    for (int i = 0; i < indent; i++) printf("  ");
    printf("%s", node_type_name(node->type));
    if (node->value) printf(" %s", node->value);
    printf("\n");
    
    if (node->type == NODE_FUNC_DEF) {
        /* Special printing for FUNC_DEF to match the expected output */
        if (node->child_count >= 2) {
            for (int i = 0; i < indent + 1; i++) printf("  ");
            printf("PARAMS [");
            for (int i = 0; i < node->child_count - 2; i++) {
                printf("%s%s", node->children[i]->value, (i < node->child_count - 3) ? ", " : "");
            }
            printf("]\n");
            
            for (int i = 0; i < indent + 1; i++) printf("  ");
            printf("RETURN_TYPE %s\n", node->children[node->child_count - 2]->value);
            
            print_ast(node->children[node->child_count - 1], indent + 1);
        }
        return; /* custom handling done */
    }

    print_ast(node->left, indent + 1);
    print_ast(node->right, indent + 1);
    
    if (node->type != NODE_FUNC_DEF) { /* already handled above */
        for (int i = 0; i < node->child_count; i++) {
            print_ast(node->children[i], indent + 1);
        }
    }
}
