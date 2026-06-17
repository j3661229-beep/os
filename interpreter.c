#include "interpreter.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* =========================================================
 *  HELPERS & MEMORY
 * ========================================================= */

static void runtime_error(int line, const char* msg) {
    fprintf(stderr, "RUNTIME ERROR (line %d): %s\n", line, msg);
    exit(1);
}

static char* dup_string(const char* s) {
    if (!s) return NULL;
    char* copy = (char*)malloc(strlen(s) + 1);
    if (!copy) {
        fprintf(stderr, "Out of memory\n");
        exit(1);
    }
    strcpy(copy, s);
    return copy;
}

static VelValue* make_null(void) {
    VelValue* v = (VelValue*)malloc(sizeof(VelValue));
    v->type = VAL_NULL;
    return v;
}

static VelValue* make_number(double n) {
    VelValue* v = (VelValue*)malloc(sizeof(VelValue));
    v->type = VAL_NUMBER;
    v->number = n;
    return v;
}

static VelValue* make_string(const char* s) {
    VelValue* v = (VelValue*)malloc(sizeof(VelValue));
    v->type = VAL_STRING;
    v->string = dup_string(s);
    return v;
}

static VelValue* make_bool(int b) {
    VelValue* v = (VelValue*)malloc(sizeof(VelValue));
    v->type = VAL_BOOL;
    v->boolean = b ? 1 : 0;
    return v;
}

static VelValue* make_func(ASTNode* node) {
    VelValue* v = (VelValue*)malloc(sizeof(VelValue));
    v->type = VAL_FUNC;
    v->func_node = node;
    return v;
}

void free_value(VelValue* v) {
    if (!v) return;
    if (v->type == VAL_STRING) {
        free(v->string);
    } else if (v->type == VAL_ARRAY) {
        for (int i = 0; i < v->array.length; i++) {
            free_value(v->array.items[i]);
        }
        free(v->array.items);
    }
    free(v);
}

static VelValue* copy_value(VelValue* v) {
    if (!v) return make_null();
    switch (v->type) {
        case VAL_NUMBER: return make_number(v->number);
        case VAL_STRING: return make_string(v->string);
        case VAL_BOOL: return make_bool(v->boolean);
        case VAL_FUNC: return make_func(v->func_node);
        case VAL_ARRAY: {
            VelValue* arr = (VelValue*)malloc(sizeof(VelValue));
            arr->type = VAL_ARRAY;
            arr->array.length = v->array.length;
            arr->array.items = (VelValue**)malloc(arr->array.length * sizeof(VelValue*));
            for (int i = 0; i < arr->array.length; i++) {
                arr->array.items[i] = copy_value(v->array.items[i]);
            }
            return arr;
        }
        case VAL_NULL: return make_null();
    }
    return make_null();
}

static int is_truthy(VelValue* v) {
    if (!v || v->type == VAL_NULL) return 0;
    if (v->type == VAL_BOOL) return v->boolean;
    if (v->type == VAL_NUMBER) return v->number != 0.0;
    if (v->type == VAL_STRING) return strlen(v->string) > 0;
    return 1;
}

static char* value_to_string(VelValue* v) {
    char buf[256];
    if (!v || v->type == VAL_NULL) return dup_string("null");
    if (v->type == VAL_NUMBER) {
        snprintf(buf, sizeof(buf), "%g", v->number);
        return dup_string(buf);
    }
    if (v->type == VAL_STRING) return dup_string(v->string);
    if (v->type == VAL_BOOL) return dup_string(v->boolean ? "true" : "false");
    if (v->type == VAL_ARRAY) return dup_string("[Array]");
    if (v->type == VAL_FUNC) return dup_string("[Function]");
    return dup_string("unknown");
}

/* =========================================================
 *  ENVIRONMENT (SCOPE)
 * ========================================================= */

Env* env_create(Env* parent) {
    Env* e = (Env*)malloc(sizeof(Env));
    e->keys = NULL;
    e->values = NULL;
    e->count = 0;
    e->capacity = 0;
    e->parent = parent;
    return e;
}

void env_set(Env* e, const char* key, VelValue* val) {
    for (int i = 0; i < e->count; i++) {
        if (strcmp(e->keys[i], key) == 0) {
            free_value(e->values[i]);
            e->values[i] = copy_value(val);
            return;
        }
    }
    if (e->count == e->capacity) {
        e->capacity = e->capacity == 0 ? 8 : e->capacity * 2;
        e->keys = (char**)realloc(e->keys, e->capacity * sizeof(char*));
        e->values = (VelValue**)realloc(e->values, e->capacity * sizeof(VelValue*));
    }
    e->keys[e->count] = dup_string(key);
    e->values[e->count] = copy_value(val);
    e->count++;
}

VelValue* env_get(Env* e, const char* key) {
    if (!e) return NULL;
    for (int i = 0; i < e->count; i++) {
        if (strcmp(e->keys[i], key) == 0) {
            return e->values[i];
        }
    }
    return env_get(e->parent, key);
}

void env_free(Env* e) {
    if (!e) return;
    for (int i = 0; i < e->count; i++) {
        free(e->keys[i]);
        free_value(e->values[i]);
    }
    free(e->keys);
    free(e->values);
    free(e);
}

/* =========================================================
 *  STRING INTERPOLATION
 * ========================================================= */
static char* interpolate(const char* tmpl, Env* env) {
    int cap = 256;
    char* res = (char*)malloc(cap);
    int len = 0;
    res[0] = '\0';
    
    int i = 0;
    while (tmpl[i] != '\0') {
        if (tmpl[i] == '{') {
            int start = i + 1;
            while (tmpl[i] != '}' && tmpl[i] != '\0') i++;
            if (tmpl[i] == '}') {
                int var_len = i - start;
                char varname[256];
                strncpy(varname, &tmpl[start], var_len);
                varname[var_len] = '\0';
                
                VelValue* val = env_get(env, varname);
                char* val_str = value_to_string(val);
                int add_len = strlen(val_str);
                
                if (len + add_len + 1 > cap) {
                    cap = cap * 2 + add_len;
                    res = (char*)realloc(res, cap);
                }
                strcpy(res + len, val_str);
                len += add_len;
                free(val_str);
            }
        } else {
            if (len + 2 > cap) {
                cap *= 2;
                res = (char*)realloc(res, cap);
            }
            res[len++] = tmpl[i];
            res[len] = '\0';
        }
        i++;
    }
    return res;
}

/* =========================================================
 *  EVALUATORS
 * ========================================================= */

Interpreter* interpreter_init(void) {
    Interpreter* interp = (Interpreter*)malloc(sizeof(Interpreter));
    interp->global = env_create(NULL);
    return interp;
}

void interpreter_free(Interpreter* interp) {
    env_free(interp->global);
    free(interp);
}

static VelValue* eval_ident(ASTNode* node, Env* env) {
    VelValue* val = env_get(env, node->value);
    if (!val) runtime_error(node->line, "Undefined variable");
    return copy_value(val);
}

static VelValue* eval_let(ASTNode* node, Env* env) {
    VelValue* val = eval(node->right, env, NULL);
    env_set(env, node->value, val);
    VelValue* ret = copy_value(val);
    free_value(val);
    return ret;
}

static VelValue* eval_assign(ASTNode* node, Env* env) {
    VelValue* val = eval(node->right, env, NULL);
    /* Set only looks at current scope in env_set, so assignment in Vel
       could technically just shadow if we used env_set. Let's make a set_existing. */
    Env* curr = env;
    while (curr) {
        for (int i = 0; i < curr->count; i++) {
            if (strcmp(curr->keys[i], node->value) == 0) {
                free_value(curr->values[i]);
                curr->values[i] = copy_value(val);
                VelValue* ret = copy_value(val);
                free_value(val);
                return ret;
            }
        }
        curr = curr->parent;
    }
    runtime_error(node->line, "Cannot assign to undefined variable");
    return NULL;
}

static VelValue* eval_binop(ASTNode* node, Env* env) {
    VelValue* left = eval(node->left, env, NULL);
    VelValue* right = eval(node->right, env, NULL);
    
    char* op = node->value;
    VelValue* res = NULL;

    if (strcmp(op, "+") == 0) {
        if (left->type == VAL_NUMBER && right->type == VAL_NUMBER) {
            res = make_number(left->number + right->number);
        } else if (left->type == VAL_STRING || right->type == VAL_STRING) {
            char* lstr = value_to_string(left);
            char* rstr = value_to_string(right);
            char* combined = (char*)malloc(strlen(lstr) + strlen(rstr) + 1);
            strcpy(combined, lstr);
            strcat(combined, rstr);
            res = make_string(combined);
            free(lstr); free(rstr); free(combined);
        } else {
            runtime_error(node->line, "Wrong type for + operator");
        }
    } else if (strcmp(op, "-") == 0) {
        if (left->type == VAL_NUMBER && right->type == VAL_NUMBER) res = make_number(left->number - right->number);
        else runtime_error(node->line, "Wrong type for - operator");
    } else if (strcmp(op, "*") == 0) {
        if (left->type == VAL_NUMBER && right->type == VAL_NUMBER) res = make_number(left->number * right->number);
        else runtime_error(node->line, "Wrong type for * operator");
    } else if (strcmp(op, "/") == 0) {
        if (left->type == VAL_NUMBER && right->type == VAL_NUMBER) {
            if (right->number == 0.0) runtime_error(node->line, "Divide by zero");
            res = make_number(left->number / right->number);
        } else runtime_error(node->line, "Wrong type for / operator");
    } else if (strcmp(op, ">") == 0) {
        if (left->type == VAL_NUMBER && right->type == VAL_NUMBER) res = make_bool(left->number > right->number);
        else runtime_error(node->line, "Wrong type for > operator");
    } else if (strcmp(op, "<") == 0) {
        if (left->type == VAL_NUMBER && right->type == VAL_NUMBER) res = make_bool(left->number < right->number);
        else runtime_error(node->line, "Wrong type for < operator");
    } else if (strcmp(op, "==") == 0) {
        if (left->type == VAL_NUMBER && right->type == VAL_NUMBER) res = make_bool(left->number == right->number);
        else if (left->type == VAL_STRING && right->type == VAL_STRING) res = make_bool(strcmp(left->string, right->string) == 0);
        else res = make_bool(0);
    } else if (strcmp(op, "!=") == 0) {
        if (left->type == VAL_NUMBER && right->type == VAL_NUMBER) res = make_bool(left->number != right->number);
        else if (left->type == VAL_STRING && right->type == VAL_STRING) res = make_bool(strcmp(left->string, right->string) != 0);
        else res = make_bool(1);
    } else {
        runtime_error(node->line, "Unknown binary operator");
    }

    free_value(left);
    free_value(right);
    return res;
}

static VelValue* eval_unop(ASTNode* node, Env* env) {
    VelValue* right = eval(node->right, env, NULL);
    if (strcmp(node->value, "-") == 0 && right->type == VAL_NUMBER) {
        VelValue* res = make_number(-right->number);
        free_value(right);
        return res;
    }
    runtime_error(node->line, "Wrong type for unary operator");
    return NULL;
}

static VelValue* eval_block(ASTNode* node, Env* env, ReturnSignal* ret_sig) {
    VelValue* last_val = make_null();
    for (int i = 0; i < node->child_count; i++) {
        free_value(last_val);
        last_val = eval(node->children[i], env, ret_sig);
        if (ret_sig && ret_sig->triggered) {
            return last_val; /* last_val is now the return value */
        }
    }
    return last_val;
}

static VelValue* eval_if(ASTNode* node, Env* env, ReturnSignal* ret_sig) {
    VelValue* cond = eval(node->left, env, ret_sig);
    int truthy = is_truthy(cond);
    free_value(cond);
    
    if (truthy) {
        return eval_block(node->children[0], env, ret_sig);
    }
    
    /* Elif / Else nodes */
    for (int i = 1; i < node->child_count; i++) {
        ASTNode* branch = node->children[i];
        if (branch->type == NODE_ELIF) {
            VelValue* elif_cond = eval(branch->left, env, ret_sig);
            int elif_truthy = is_truthy(elif_cond);
            free_value(elif_cond);
            if (elif_truthy) {
                return eval_block(branch->children[0], env, ret_sig);
            }
        } else if (branch->type == NODE_ELSE) {
            return eval_block(branch->children[0], env, ret_sig);
        }
    }
    
    return make_null();
}

static VelValue* eval_match(ASTNode* node, Env* env, ReturnSignal* ret_sig) {
    VelValue* match_val = eval(node->left, env, ret_sig);
    
    for (int i = 0; i < node->child_count; i++) {
        ASTNode* arm = node->children[i];
        int matched = 0;
        if (strcmp(arm->value, "_") == 0) {
            matched = 1;
        } else if (match_val->type == VAL_NUMBER) {
            matched = (match_val->number == atof(arm->value));
        } else if (match_val->type == VAL_STRING) {
            matched = (strcmp(match_val->string, arm->value) == 0);
        }
        
        if (matched) {
            free_value(match_val);
            return eval(arm->right, env, ret_sig);
        }
    }
    
    free_value(match_val);
    return make_null();
}

static VelValue* eval_loop_range(ASTNode* node, Env* env, ReturnSignal* ret_sig) {
    VelValue* start_val = eval(node->left, env, ret_sig);
    VelValue* end_val = eval(node->right, env, ret_sig);
    
    if (start_val->type != VAL_NUMBER || end_val->type != VAL_NUMBER) {
        runtime_error(node->line, "Loop bounds must be numbers");
    }
    
    int start = (int)start_val->number;
    int end = (int)end_val->number;
    free_value(start_val);
    free_value(end_val);
    
    Env* loop_env = env_create(env);
    VelValue* res = make_null();
    
    for (int i = start; i <= end; i++) {
        VelValue* ival = make_number(i);
        env_set(loop_env, node->value, ival);
        free_value(ival);
        
        free_value(res);
        res = eval_block(node->children[0], loop_env, ret_sig);
        
        if (ret_sig && ret_sig->triggered) break;
    }
    
    env_free(loop_env);
    return res;
}

static VelValue* eval_call(ASTNode* node, Env* env, ReturnSignal* ret_sig) {
    VelValue* func_val = env_get(env, node->value);
    if (!func_val || func_val->type != VAL_FUNC) {
        runtime_error(node->line, "Attempted to call non-function");
    }
    ASTNode* func_def = func_val->func_node;
    int param_count = func_def->child_count - 2; /* last two are ret_type, block */
    
    if (node->child_count != param_count) {
        runtime_error(node->line, "Wrong arg count");
    }
    
    Env* call_env = env_create(env); /* Functions should really capture definition environment, simplified here to dynamic scope or global if global. Let's use current env */
    
    for (int i = 0; i < param_count; i++) {
        VelValue* arg_val = eval(node->children[i], env, ret_sig);
        env_set(call_env, func_def->children[i]->value, arg_val);
        free_value(arg_val);
    }
    
    ReturnSignal call_ret_sig = {0, NULL};
    VelValue* res = eval_block(func_def->children[func_def->child_count - 1], call_env, &call_ret_sig);
    
    env_free(call_env);
    
    if (call_ret_sig.triggered) {
        free_value(res);
        return call_ret_sig.value;
    }
    return res;
}

static VelValue* eval_array(ASTNode* node, Env* env, ReturnSignal* ret_sig) {
    VelValue* arr = (VelValue*)malloc(sizeof(VelValue));
    arr->type = VAL_ARRAY;
    arr->array.length = node->child_count;
    arr->array.items = (VelValue**)malloc(arr->array.length * sizeof(VelValue*));
    for (int i = 0; i < arr->array.length; i++) {
        arr->array.items[i] = eval(node->children[i], env, ret_sig);
    }
    return arr;
}

static VelValue* eval_index(ASTNode* node, Env* env, ReturnSignal* ret_sig) {
    VelValue* arr = eval(node->left, env, ret_sig);
    VelValue* idx = eval(node->right, env, ret_sig);
    
    if (arr->type != VAL_ARRAY || idx->type != VAL_NUMBER) {
        runtime_error(node->line, "Invalid array indexing");
    }
    
    int i = (int)idx->number;
    if (i < 0 || i >= arr->array.length) {
        runtime_error(node->line, "Index out of bounds");
    }
    
    VelValue* res = copy_value(arr->array.items[i]);
    free_value(arr);
    free_value(idx);
    return res;
}

static VelValue* eval_show(ASTNode* node, Env* env, ReturnSignal* ret_sig) {
    VelValue* val = eval(node->right, env, ret_sig);
    if (val->type == VAL_NUMBER) {
        printf("%g\n", val->number);
    } else if (val->type == VAL_STRING) {
        char* interp = interpolate(val->string, env);
        printf("%s\n", interp);
        free(interp);
    } else if (val->type == VAL_BOOL) {
        printf("%s\n", val->boolean ? "true" : "false");
    } else if (val->type == VAL_ARRAY) {
        printf("[Array of len %d]\n", val->array.length);
    } else if (val->type == VAL_NULL) {
        printf("null\n");
    } else {
        printf("[Function]\n");
    }
    return val;
}

static VelValue* eval_return(ASTNode* node, Env* env, ReturnSignal* ret_sig) {
    VelValue* val = eval(node->right, env, ret_sig);
    if (ret_sig) {
        ret_sig->triggered = 1;
        ret_sig->value = val;
        return copy_value(val);
    }
    return val;
}

/* Pipeline operator `a |> f(b)` */
static VelValue* eval_pipeline(ASTNode* node, Env* env, ReturnSignal* ret_sig) {
    VelValue* left_val = eval(node->left, env, ret_sig);
    ASTNode* call_node = node->right;
    
    if (call_node->type != NODE_CALL) {
        runtime_error(node->line, "Right side of pipeline must be a call");
    }
    
    /* We temporarily modify the AST to prepend the argument */
    call_node->child_count++;
    call_node->children = (ASTNode**)realloc(call_node->children, call_node->child_count * sizeof(ASTNode*));
    
    /* Shift existing arguments to the right */
    for (int i = call_node->child_count - 1; i > 0; i--) {
        call_node->children[i] = call_node->children[i-1];
    }
    
    /* Create a mock AST node that evaluates to left_val directly */
    /* But since we just want to execute it, evaluating the AST is easier.
       Wait, let's just do it cleanly by modifying the env or injecting. 
       Actually, since we already have left_val, maybe we evaluate the call manually here? */
    
    VelValue* func_val = env_get(env, call_node->value);
    if (!func_val || func_val->type != VAL_FUNC) runtime_error(node->line, "Attempted to call non-function in pipeline");
    ASTNode* func_def = func_val->func_node;
    int param_count = func_def->child_count - 2;
    if (call_node->child_count != param_count) runtime_error(node->line, "Wrong arg count in pipeline");
    
    Env* call_env = env_create(env);
    
    /* First param is left_val */
    env_set(call_env, func_def->children[0]->value, left_val);
    
    /* Rest of the params are the original arguments in call_node */
    for (int i = 1; i < param_count; i++) {
        VelValue* arg_val = eval(call_node->children[i], env, ret_sig);
        env_set(call_env, func_def->children[i]->value, arg_val);
        free_value(arg_val);
    }
    
    ReturnSignal call_ret_sig = {0, NULL};
    VelValue* res = eval_block(func_def->children[func_def->child_count - 1], call_env, &call_ret_sig);
    env_free(call_env);
    free_value(left_val);
    
    /* Restore the AST so free_ast doesn't mess up */
    for (int i = 0; i < call_node->child_count - 1; i++) {
        call_node->children[i] = call_node->children[i+1];
    }
    call_node->child_count--;
    
    if (call_ret_sig.triggered) {
        free_value(res);
        return call_ret_sig.value;
    }
    return res;
}

VelValue* eval(ASTNode* node, Env* env, ReturnSignal* ret_sig) {
    if (!node) return make_null();
    switch (node->type) {
        case NODE_PROGRAM: {
            VelValue* last = make_null();
            for (int i = 0; i < node->child_count; i++) {
                free_value(last);
                last = eval(node->children[i], env, ret_sig);
            }
            return last;
        }
        case NODE_LET:
        case NODE_CONST:
            return eval_let(node, env);
        case NODE_ASSIGN:
            return eval_assign(node, env);
        case NODE_FUNC_DEF: {
            VelValue* f = make_func(node);
            env_set(env, node->value, f);
            return f;
        }
        case NODE_IF: return eval_if(node, env, ret_sig);
        case NODE_MATCH: return eval_match(node, env, ret_sig);
        case NODE_LOOP_RANGE: return eval_loop_range(node, env, ret_sig);
        case NODE_REPEAT_WHILE: {
            VelValue* res = make_null();
            while (1) {
                VelValue* cond = eval(node->left, env, ret_sig);
                int truthy = is_truthy(cond);
                free_value(cond);
                if (!truthy) break;
                free_value(res);
                res = eval_block(node->children[0], env, ret_sig);
                if (ret_sig && ret_sig->triggered) break;
            }
            return res;
        }
        case NODE_CALL: return eval_call(node, env, ret_sig);
        case NODE_PIPELINE: return eval_pipeline(node, env, ret_sig);
        case NODE_BINOP: return eval_binop(node, env);
        case NODE_UNOP: return eval_unop(node, env);
        case NODE_SHOW: return eval_show(node, env, ret_sig);
        case NODE_RETURN: return eval_return(node, env, ret_sig);
        case NODE_BLOCK: return eval_block(node, env, ret_sig);
        case NODE_IDENT: return eval_ident(node, env);
        case NODE_NUMBER: return make_number(atof(node->value));
        case NODE_STRING: return make_string(node->value);
        case NODE_BOOL: return make_bool(strcmp(node->value, "true") == 0);
        case NODE_ARRAY: return eval_array(node, env, ret_sig);
        case NODE_INDEX: return eval_index(node, env, ret_sig);
        default: return make_null();
    }
}
