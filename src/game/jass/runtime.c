#include "api.h"
#include "jass_parser.h"
#include "../parser.h"

#define F_END { NULL }
#define MAX_JASS_STACK 256
#define JASS_DELIM ",;()[]+-/*="
#define JASS_CONSTANT "constant"
#define JASS_ARRAY "array"
#define JASS_NULL "null"
#define JASS_FALSE "false"
#define JASS_TRUE "true"
#define JASS_UNM "-"
#define JASS_COMMA ","
#define JASS_OPERATOR(NAME) { #NAME, NAME }

#define assert_type(var, type) assert(jass_checktype(var, type))
#define JASSALLOC(type) gi.MemAlloc(sizeof(type))

KNOWN_AS(jass_dict, JASSDICT);
KNOWN_AS(jass_arg, JASSARG);
KNOWN_AS(jass_env, JASSENV);

LPCSTR keywords[] = {
    "elseif", "else", "endif", "set", "endfunction", "local", "then", NULL
};

extern JASSMODULE jass_funcs[];

typedef struct {
    LPCSTR name;
    void (*func)(LPJASS, LPJASSENV, LPPARSER);
} parseStatement_t;

struct jass_var {
    LPCJASSTYPE type;
    HANDLE value;
    BOOL constant;
    BOOL array;
};

struct jass_type {
    LPCSTR name;
    LPCSTR ctype;
    LPCJASSTYPE inherit;
    LPJASSTYPE next;
};

struct jass_arg {
    LPJASSARG next;
    LPCJASSTYPE type;
    LPCSTR name;
};

JASSTYPE jass_types[] = {
    { "integer", "LONG", NULL },
    { "real", "FLOAT", NULL },
    { "string", "LPCSTR", NULL },
    { "boolean", "BOOL", NULL },
    { "code", "LPCSTR", NULL },
    { "handle", "HANDLE", NULL },
    { "function", "HANDLE", NULL },
    { "cfunction", "HANDLE", NULL },
};

static LPJASSVAR jass_stackvalue(LPJASS j, int index);
static JASSTYPEID jass_getvarbasetype(LPCJASSVAR var);

BOOL atob(LPCSTR str) {
    return !strcmp(str, "true");
}

DWORD __unm(LPJASS j) {
    if (jass_gettype(j, 1) == jasstype_integer) {
        return jass_pushinteger(j, -jass_checkinteger(j, 1));
    } else {
        return jass_pushnumber(j, -jass_checknumber(j, 1));
    }
}


#define JASS_CMPOP(NAME, OP) \
DWORD NAME(LPJASS j) { \
    return jass_pushboolean(j, jass_checknumber(j, 1) OP jass_checknumber(j, 2)); \
}

#define JASS_NUMOP(NAME, OP) \
DWORD NAME(LPJASS j) { \
    if (jass_gettype(j, 1) == jasstype_integer && jass_gettype(j, 2) == jasstype_integer) { \
        return jass_pushinteger(j, jass_checkinteger(j, 1) OP jass_checkinteger(j, 2)); \
    } else { \
        return jass_pushnumber(j, jass_checknumber(j, 1) OP jass_checknumber(j, 2)); \
    } \
}

JASS_NUMOP(__add, +);
JASS_NUMOP(__sub, -);
JASS_NUMOP(__mul, *);
JASS_NUMOP(__div, /);
JASS_CMPOP(__le, <=);
JASS_CMPOP(__ge, >=);
JASS_CMPOP(__gt, >);
JASS_CMPOP(__lt, <);

static BOOL var_eq(LPCJASSVAR a, LPCJASSVAR b) {
    if (jass_getvarbasetype(a) != jass_getvarbasetype(b)) {
        return false;
    }
    switch ((a->value == NULL) + (b->value == NULL)) {
        case 2: return true;
        case 1: return false;
    }
    switch (jass_getvarbasetype(a)) {
        case jasstype_integer: return !memcmp(a->value, b->value, sizeof(LONG));
        case jasstype_real: return !memcmp(a->value, b->value, sizeof(FLOAT));
        case jasstype_string: return !strcmp(a->value, b->value);
        case jasstype_boolean: return !memcmp(a->value, b->value, sizeof(BOOL));
        case jasstype_code: return !strcmp(a->value, b->value);
        case jasstype_handle: return !memcmp(a->value, b->value, sizeof(HANDLE));
        case jasstype_function: return !memcmp(a->value, b->value, sizeof(HANDLE));
        case jasstype_cfunction: return !memcmp(a->value, b->value, sizeof(HANDLE));
    }
    return false;
}


DWORD __eq(LPJASS j) {
    return jass_pushboolean(j, var_eq(jass_stackvalue(j, 1), jass_stackvalue(j, 2)));
}

DWORD __ne(LPJASS j) {
    return jass_pushboolean(j, !var_eq(jass_stackvalue(j, 1), jass_stackvalue(j, 2)));
}

DWORD __and(LPJASS j) {
    return jass_pushboolean(j, jass_toboolean(j, 1) && jass_toboolean(j, 2));
}

DWORD __or(LPJASS j) {
    return jass_pushboolean(j, jass_toboolean(j, 1) || jass_toboolean(j, 2));
}

JASSMODULE jass_operators[] = {
    JASS_OPERATOR(__add),
    JASS_OPERATOR(__sub),
    JASS_OPERATOR(__mul),
    JASS_OPERATOR(__div),
    JASS_OPERATOR(__ne),
    JASS_OPERATOR(__eq),
    JASS_OPERATOR(__ge),
    JASS_OPERATOR(__le),
    JASS_OPERATOR(__gt),
    JASS_OPERATOR(__lt),
    JASS_OPERATOR(__and),
    JASS_OPERATOR(__or),
    JASS_OPERATOR(__unm),
    { NULL },
};

void removeDoubleBackslashes(LPSTR str) {
    size_t len = strlen(str);
    size_t j = 0;
    for (size_t i = 0; i < len; i++) {
        str[j++] = str[i];
        if (str[i] == '\\' && str[i + 1] == '\\') {
            i++; // Skip the second backslash
        }
    }
    str[j] = '\0'; // Null-terminate the modified string
}

struct jass_function {
    LPJASSARG args;
    LPCJASSTYPE returns;
    LPJASSFUNC next;
    LPCSTR name;
    LPCTOKEN code;
    DWORD (*nativefunc)(LPJASS j);
    BOOL constant;
};

struct jass_dict {
    LPJASSDICT next;
    LPCSTR key;
    JASSVAR value;
};

struct jass_env {
    LPJASSDICT locals;
};

struct jass_s {
    LPJASSDICT globals;
    LPJASSTYPE types;
    LPJASSFUNC functions;
    JASSVAR stack[MAX_JASS_STACK];
    DWORD num_stack;
    DWORD stack_offset;
};

BOOL is_integer(LPCSTR tok) {
    LPSTR endptr;
    strtol(tok, &endptr, 10);
    return *endptr == '\0';
}

BOOL is_float(LPCSTR tok) {
    LPSTR endptr;
    strtod(tok, &endptr);
    return *endptr == '\0';
}

BOOL is_fourcc(LPCSTR tok) {
    return *tok == '\'';
}

BOOL is_string(LPCSTR tok) {
    return *tok == '\"';
}

BOOL is_identifier(LPCSTR str) {
    if (!isalpha(*str) && *str != '_')
        return false;
    for (LPCSTR s = str; *s; ++s) {
        if (!isalnum(*s) && *s != '_')
            return false;
    }
    for (LPCSTR *kw = keywords; *kw; kw++) {
        if (!strcmp(str, *kw)) {
            return false;
        }
    }
    return true;
}

BOOL is_comma(LPCSTR str) {
    return !strcmp(str, ",");
}

static LPJASSCFUNCTION find_cfunction(LPCJASS j, LPCSTR name) {
    for (LPCJASSMODULE m = jass_operators; m->name; m++) {
        if (!strcmp(m->name, name)) {
            return m->func;
        }
    }
    return NULL;
}

static LPCJASSFUNC find_function(LPCJASS j, LPCSTR name) {
    FOR_EACH_LIST(JASSFUNC, func, j->functions) {
        if (!strcmp(func->name, name)) {
            return func;
        }
    }
    return NULL;
}

static LPJASSVAR find_dict(LPJASSDICT dict, LPCSTR name) {
    FOR_EACH_LIST(JASSDICT, item, dict) {
        if (!strcmp(item->key, name)) {
            return &item->value;
        }
    }
    return NULL;
}

static LPCJASSTYPE find_type(LPCJASS j, LPCSTR name) {
    FOR_LOOP(i, sizeof(jass_types)/sizeof(*jass_types)) {
        if (!strcmp(jass_types[i].name, name)) {
            return &jass_types[i];
        }
    }
    FOR_EACH_LIST(JASSTYPE, type, j->types) {
        if (!strcmp(type->name, name)) {
            return type;
        }
    }
    return NULL;
}

static void jass_call(LPJASS j, DWORD args);

LPCJASSTYPE get_base_type(LPCJASSTYPE type) {
    while (type->inherit) {
        type = type->inherit;
    }
    return type;
}

DWORD jass_top(LPJASS j) {
    return j->num_stack-1;
}

LPJASSVAR jass_topvalue(LPJASS j) {
    return j->stack+jass_top(j);
}

LPJASSVAR jass_stackvalue(LPJASS j, int index) {
    if (index < 0) {
        return j->stack + (j->num_stack + index);
    } else {
        return j->stack + (j->stack_offset + index);
    }
}

JASSTYPEID jass_getvarbasetype(LPCJASSVAR var) {
    return (JASSTYPEID)(get_base_type(var->type) - jass_types);
}

JASSTYPEID jass_gettype(LPJASS j, int index) {
    LPCJASSVAR var = jass_stackvalue(j, index);
    return jass_getvarbasetype(var);
}

BOOL jass_checktype(LPCJASSVAR var, JASSTYPEID type) {
    return get_base_type(var->type) == jass_types+type;
}

void jass_pop(LPJASS j, DWORD count) {
    j->num_stack -= count;
}

#define JASS_ADD_STACK(j, VAR, TYPE) \
LPJASSVAR VAR = &j->stack[j->num_stack++]; \
memset(VAR, 0, sizeof(*VAR)); \
VAR->type = &jass_types[TYPE];

#define JASS_SET_NULL(VAR) \
SAFE_DELETE((VAR)->value, gi.MemFree);

#define JASS_SET_VALUE(VAR, VALUE, SIZE) \
JASS_SET_NULL(VAR); \
if (VALUE) { \
  (VAR)->value = gi.MemAlloc(SIZE); \
  memcpy((VAR)->value, VALUE, SIZE); \
}

void jass_copy(LPJASS j, LPJASSVAR var, LPCJASSVAR other) {
    FLOAT fval = 0;
    if (!other->value) {
        JASS_SET_NULL(var);
        return;
    }
    switch (jass_getvarbasetype(var)) {
        case jasstype_integer:
            assert(jass_checktype(other, jasstype_integer));
            JASS_SET_VALUE(var, other->value, sizeof(LONG));
            break;
        case jasstype_handle:
            assert(jass_checktype(other, jasstype_handle));
            JASS_SET_VALUE(var, other->value, sizeof(LONG));
            break;
        case jasstype_real:
            switch (jass_getvarbasetype(other)) {
                case jasstype_real:
                    fval = *(FLOAT const *)other->value;
                    break;
                case jasstype_integer:
                    fval = *(LONG const *)other->value;
                    break;
                default:
                    assert(false);
                    return;
            }
            JASS_SET_VALUE(var, &fval, sizeof(FLOAT));
            break;
        case jasstype_boolean:
            assert(jass_checktype(other, jasstype_boolean));
            JASS_SET_VALUE(var, other->value, sizeof(BOOL));
            break;
        case jasstype_string:
            assert(jass_checktype(other, jasstype_string));
            JASS_SET_VALUE(var, other->value, strlen(other->value)+1);
            break;
        default:
            assert(false);
            break;
    }
}

DWORD jass_pushnull(LPJASS j) {
    JASS_ADD_STACK(j, var, jasstype_handle);
    return 1;
}

DWORD jass_pushinteger(LPJASS j, LONG value) {
    JASS_ADD_STACK(j, var, jasstype_integer);
    JASS_SET_VALUE(var, &value, sizeof(value));
    return 1;
}

DWORD jass_pushhandle(LPJASS j, LONG value, LPCSTR type) {
    JASS_ADD_STACK(j, var, jasstype_handle);
    JASS_SET_VALUE(var, &value, sizeof(value));
    return 1;
}

DWORD jass_pushnumber(LPJASS j, FLOAT value) {
    JASS_ADD_STACK(j, var, jasstype_real);
    JASS_SET_VALUE(var, &value, sizeof(value));
    return 1;
}

DWORD jass_pushboolean(LPJASS j, BOOL value) {
    JASS_ADD_STACK(j, var, jasstype_boolean);
    JASS_SET_VALUE(var, &value, sizeof(value));
    return 1;
}

DWORD jass_pushstringlen(LPJASS j, LPCSTR value, DWORD len) {
    JASS_ADD_STACK(j, var, jasstype_string);
    JASS_SET_VALUE(var, value, len+1);
    ((LPSTR)var->value)[len] = '\0';
    removeDoubleBackslashes(var->value);
    return 1;
}

DWORD jass_pushstring(LPJASS j, LPCSTR value) {
    jass_pushstringlen(j, value, (DWORD)strlen(value));
    return 1;
}

DWORD jass_pushcfunction(LPJASS j, LPJASSCFUNCTION func) {
    JASS_ADD_STACK(j, var, jasstype_cfunction);
    JASS_SET_VALUE(var, &func, sizeof(LPJASSCFUNCTION));
    return 1;
}
 
DWORD jass_pushfunction(LPJASS j, LPCJASSFUNC func) {
    if (func->nativefunc) {
        return jass_pushcfunction(j, func->nativefunc);
    } else {
        JASS_ADD_STACK(j, var, jasstype_function);
        JASS_SET_VALUE(var, func, sizeof(*func));
        return 1;
    }
}

DWORD jass_pushvalue(LPJASS j, LPCJASSVAR other) {
    JASS_ADD_STACK(j, var, get_base_type(other->type) - jass_types);
    jass_copy(j, var, other);
    return 1;
}

LONG jass_checkinteger(LPJASS j, int index) {
    LPCJASSVAR var = jass_stackvalue(j, index);
    assert_type(var, jasstype_integer);
    return var->value ? *(LONG *)var->value : 0;
}

FLOAT jass_checknumber(LPJASS j, int index) {
    LPCJASSVAR var = jass_stackvalue(j, index);
    if (jass_checktype(var, jasstype_real)) {
        return var->value ? *(FLOAT *)var->value : 0;
    }
    if (jass_checktype(var, jasstype_integer)) {
        return var->value ? *(LONG *)var->value : 0;
    }
    assert(false);
}

BOOL jass_checkboolean(LPJASS j, int index) {
    LPCJASSVAR var = jass_stackvalue(j, index);
    assert_type(var, jasstype_boolean);
    return var->value ? *(BOOL *)var->value : 0;
}

BOOL jass_toboolean(LPJASS j, int index) {
    LPCJASSVAR var = jass_stackvalue(j, index);
    JASSTYPEID type = jass_getvarbasetype(var);
    if (var->value == NULL)
        return false;
    switch (type) {
        case jasstype_integer: return *(LONG *)var->value != 0;
        case jasstype_real: return *(FLOAT *)var->value != 0;
        case jasstype_string: return strlen(var->value) > 0;
        case jasstype_boolean: return *(BOOL *)var->value != 0;
        case jasstype_code: return strlen(var->value) > 0;
        case jasstype_handle: return true;
        case jasstype_function: return true;
        default: return false;
    }
}

LPCSTR jass_checkstring(LPJASS j, int index) {
    LPCJASSVAR var = jass_stackvalue(j, index);
    assert_type(var, jasstype_string);
    return var->value;
}

LPCSTR jass_checkcode(LPJASS j, int index) {
    LPCJASSVAR var = jass_stackvalue(j, index);
    assert_type(var, jasstype_code);
    return var->value;
}

HANDLE jass_checkhandle(LPJASS j, int index, LPCSTR type) {
    LPCJASSVAR var = jass_stackvalue(j, index);
    assert_type(var, jasstype_handle);
    return var->value ? *(HANDLE *)var->value : 0;
}

void jass_swap(LPJASS j, int a, int b) {
    JASSVAR tmp = *jass_stackvalue(j, a);
    *jass_stackvalue(j, a) = *jass_stackvalue(j, b);
    *jass_stackvalue(j, -2) = tmp;
}

static DWORD jass_dotoken(LPJASS j, LPCTOKEN token, LPJASSDICT locals) {
    LPCJASSFUNC f = NULL;
    LPJASSCFUNCTION cf = NULL;
    LPCJASSVAR v = NULL;
    DWORD stacksize = j->num_stack;
    switch (token->type) {
        case TT_INTEGER: return jass_pushinteger(j, atoi(token->primary));
        case TT_REAL: return jass_pushnumber(j, atof(token->primary));
        case TT_STRING: return jass_pushstring(j, token->primary);
        case TT_BOOLEAN: return jass_pushboolean(j, atob(token->primary));
        case TT_IDENTIFIER:
            if ((v = find_dict(j->globals, token->primary))) {
                return jass_pushvalue(j, v);
            } else if ((v = find_dict(locals, token->primary))) {
                return jass_pushvalue(j, v);
            } else {
                return jass_pushnull(j);
            }
        case TT_FOURCC: return jass_pushinteger(j, *(DWORD *)token->primary);
        case TT_CALL:
            if ((f = find_function(j, token->primary))) {
                DWORD args = 0;
                jass_pushfunction(j, f);
                FOR_EACH_LIST(TOKEN, arg, token->args) {
                    jass_dotoken(j, arg, locals);
                    args++;
                }
                jass_call(j, args);
                return j->num_stack - stacksize;
            } else if ((cf = find_cfunction(j, token->primary))) {
                DWORD args = 0;
                jass_pushcfunction(j, cf);
                FOR_EACH_LIST(TOKEN, arg, token->args) {
                    jass_dotoken(j, arg, locals);
                    args++;
                }
                jass_call(j, args);
                return j->num_stack - stacksize;
            } else {
                fprintf(stderr, "Can't find function %s\n", token->primary);
                assert(false);
                return 0;
            }
        default:
            assert(false);
            return 0;
    }
}

static void jass_set_value(LPJASS j, LPJASSVAR dest, LPCTOKEN init, LPJASSDICT locals) {
    DWORD stack = jass_dotoken(j, init, locals);
    if (stack > 0) {
        jass_copy(j, dest, j->stack + jass_top(j));
        jass_pop(j, 1);
    }
}

LPJASSDICT parse_dict(LPJASS j, LPCTOKEN token, LPJASSDICT locals) {
    LPJASSDICT item = JASSALLOC(JASSDICT);
    item->value.constant = token->flags & TF_CONSTANT;
    item->value.array = token->flags & TF_ARRAY;
    item->value.type = find_type(j, token->primary);
    item->key = token->secondary;
    if (token->init) {
        jass_set_value(j, &item->value, token->init, locals);
    }
    return item;
}

#define TOKENFUNC(NAME) void eval_##NAME(LPJASS j, LPJASSENV env, LPCTOKEN token)
#define TOKENEVAL(NAME) { #NAME, TT_##NAME, eval_##NAME }

TOKENFUNC(TOKENS);

TOKENFUNC(TYPEDEF) {
    LPJASSTYPE type = JASSALLOC(JASSTYPE);;
    type->name = token->primary;
    type->inherit = find_type(j, token->secondary);
    ADD_TO_LIST(type, j->types);
}

TOKENFUNC(IF) {
    jass_dotoken(j, token->condition, env->locals);
    BOOL const cond = jass_toboolean(j, -1);
    jass_pop(j, 1);
    if (cond) {
        eval_TOKENS(j, env, token->body);
    } else if (token->elseblock) {
        eval_TOKENS(j, env, token->elseblock);
    }
}

TOKENFUNC(SET) {
    LPJASSVAR v = NULL;
    if ((v = find_dict(j->globals, token->secondary))) {
        return jass_set_value(j, v, token->init, env->locals);
    } else if ((v = find_dict(env->locals, token->secondary))) {
        return jass_set_value(j, v, token->init, env->locals);
    } else {
        fprintf(stderr, "Can't find variable %s\n", token->primary);
    }
}

TOKENFUNC(VARDECL) {
    LPJASSDICT vardecl = parse_dict(j, token, NULL);
    ADD_TO_LIST(vardecl, env->locals);
}

TOKENFUNC(GLOBAL) {
    LPJASSDICT global = parse_dict(j, token, NULL);
    ADD_TO_LIST(global, j->globals);
}

TOKENFUNC(FUNCTION) {
    LPJASSFUNC func = JASSALLOC(JASSFUNC);
    func->name = token->primary;
    func->code = token->body;
    func->returns = find_type(j, token->secondary);
    FOR_EACH_LIST(TOKEN, arg, token->args) {
        LPJASSARG jarg = JASSALLOC(JASSARG);
        jarg->name = arg->secondary;
        jarg->type = find_type(j, arg->primary);
        PUSH_BACK(JASSARG, jarg, func->args);
    }
    if (token->flags & TF_NATIVE) {
        LPJASSMODULE mod = find_in_array(jass_funcs, sizeof(JASSMODULE), func->name);
        if (mod) {
            func->nativefunc = mod->func;
        }
    }
    ADD_TO_LIST(func, j->functions);
}

TOKENFUNC(CALL) {
    jass_dotoken(j, token, env->locals);
}

struct {
    LPCSTR name;
    TOKENTYPE type;
    void (*func)(LPJASS, LPJASSENV, LPCTOKEN);
} token_eval[] = {
    TOKENEVAL(TYPEDEF),
    TOKENEVAL(FUNCTION),
    TOKENEVAL(VARDECL),
    TOKENEVAL(GLOBAL),
    TOKENEVAL(CALL),
    TOKENEVAL(IF),
    TOKENEVAL(SET),
};

TOKENFUNC(TOKENS) {
    FOR_EACH_LIST(TOKEN const, tok, token) {
        if (tok->type == TT_RETURN) {
            if (tok->body){
                jass_dotoken(j, tok->body, env->locals);
            }
            return;
        }
        FOR_LOOP(index, sizeof(token_eval) / sizeof(*token_eval)) {
            if (tok->type == token_eval[index].type) {
                token_eval[index].func(j, env, tok);
                goto next_loop;
            }
        }
        fprintf(stderr, "Can't evaluate token of type %d\n", tok->type);
        break;
    next_loop:
        continue;
    }
}

BOOL JASS_Parse_Buffer(LPJASS j, LPCSTR fileName, LPSTR buffer2) {
    LPSTR buffer = buffer2;
    remove_comments(buffer);
    remove_bom(buffer, strlen(buffer));
    LPTOKEN program = JASS_ParseTokens(&MAKE(PARSER, .buffer = buffer, .delimiters = JASS_DELIM));
    JASSENV env = { 0 };
    eval_TOKENS(j, &env, program);
    return true;
}

LPJASS JASS_Allocate(void) {
    return JASSALLOC(JASS);
}

BOOL JASS_Parse(LPJASS j, LPCSTR fileName) {
    LPSTR buffer = gi.ReadFileIntoString(fileName);
    if (buffer) {
        BOOL success = JASS_Parse_Buffer(j, fileName, buffer);
        gi.MemFree(buffer);
        return success;
    } else {
        return false;
    }
}

BOOL JASS_Parse_Native(LPJASS j, LPCSTR fileName) {
    FILE *file = fopen(fileName, "rb");
    if (file == NULL) {
        fprintf(stderr, "Error opening the file.\n");
        return false;
    }
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    LPSTR buffer = gi.MemAlloc(file_size + 1); // +1 for null-terminator
    if (buffer == NULL) {
        fprintf(stderr, "Memory allocation failed.\n");
        fclose(file);
        return false;
    }
    fread(buffer, 1, file_size, file);
    buffer[file_size] = '\0';

    fclose(file);

    BOOL success = JASS_Parse_Buffer(j, fileName, buffer);
        
    // Free the buffer memory
    gi.MemFree(buffer);

    return success;
}

void jass_call(LPJASS j, DWORD args) {
    LPJASSVAR var = &j->stack[j->num_stack - args - 1];
    LPJASSVAR last = &j->stack[j->num_stack];
    DWORD old_stack_offset = j->stack_offset, ret = 0;
    j->stack_offset = j->num_stack - args - 1;
    if (jass_getvarbasetype(var) == jasstype_cfunction) {
        LPJASSCFUNCTION func = *(LPJASSCFUNCTION *)var->value;
        ret = func(j);
    } else {
        LPCJASSFUNC func = var->value;
        JASSENV env = { 0 };
        printf("%s\n", func->name);
        eval_TOKENS(j, &env, func->code);
    }
    for (LPJASSVAR it = var; it < last; it++) JASS_SET_NULL(it)
    memmove(var, last, ret * sizeof(JASSVAR));
    j->num_stack -= last - var;
    j->stack_offset = old_stack_offset;
}

void JASS_ExecuteFunc(LPJASS j, LPCSTR name) {
    LPCJASSFUNC f = find_function(j, name);
    if (f) {
        jass_pushfunction(j, f);
        jass_call(j, 0);
    }
}
