#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "parser.h"

/* =========================================================
 *  §1  VALUE SYSTEM
 * ========================================================= */
typedef enum {
    VAL_NUMBER,   /* double */
    VAL_STRING,   /* char* */
    VAL_BOOL,     /* int (0 or 1) */
    VAL_ARRAY,    /* VelValue** + length */
    VAL_FUNC,     /* pointer to NODE_FUNC_DEF ASTNode */
    VAL_NULL      /* no value / void return */
} ValueType;

typedef struct VelValue {
    ValueType type;
    union {
        double      number;
        char*       string;
        int         boolean;
        struct {
            struct VelValue** items;
            int               length;
        } array;
        ASTNode*    func_node;
    };
} VelValue;

/* =========================================================
 *  §2  ENVIRONMENT (SCOPE SYSTEM)
 * ========================================================= */
typedef struct Env {
    char**      keys;
    VelValue**  values;
    int         count;
    int         capacity;
    struct Env* parent;    /* NULL for global scope */
} Env;

Env*      env_create(Env* parent);
void      env_set(Env* e, const char* key, VelValue* val);
VelValue* env_get(Env* e, const char* key);  /* walks up parent chain */
void      env_free(Env* e);

/* =========================================================
 *  §3  INTERPRETER SETUP & CORE
 * ========================================================= */
typedef struct {
    Env* global;
} Interpreter;

/* Return Signal for handling function returns */
typedef struct {
    int       triggered;
    VelValue* value;
} ReturnSignal;

Interpreter* interpreter_init(void);
void         interpreter_free(Interpreter* interp);

VelValue* eval(ASTNode* node, Env* env, ReturnSignal* ret_sig);
void      free_value(VelValue* v);

#endif /* INTERPRETER_H */
