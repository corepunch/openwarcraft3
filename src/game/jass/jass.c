#include "api.h"
#include "../parser.h"

#define F_END { NULL }
#define MAX_JASS_TYPES 256
#define MAX_JASS_STACK 256
#define MAX_JASS_FUNCTIONS 8000
#define MAX_JASS_ARGS 16
#define JASS_DELIM ",;()+-/*"
#define JASS_CONSTANT "constant"
#define JASS_ARRAY "array"
#define JASS_NULL "null"
#define JASS_FALSE "false"
#define JASS_TRUE "true"
#define JASS_UNM "-"
#define JASS_COMMA ","

#define assert_type(var, type) assert(jass_checktype(var, type))
#define JASSALLOC(type) gi.MemAlloc(sizeof(type))

KNOWN_AS(jass_dict, JASSDICT);
KNOWN_AS(jass_arg, JASSARG);
KNOWN_AS(jass_env, JASSENV);

extern JASSMODULE jass_funcs[];

typedef struct {
    LPCSTR name;
    void (*func)(LPJASS, LPPARSER);
} parseClass_t;

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
    UINAME name;
    LPCSTR ctype;
    LPCJASSTYPE inherit;
};

struct jass_arg {
    LPJASSARG next;
    LPCJASSTYPE type;
    UINAME name;
};

JASSTYPE jass_types[] = {
    { "integer", "LONG", NULL },
    { "real", "FLOAT", NULL },
    { "string", "LPCSTR", NULL },
    { "boolean", "BOOL", NULL },
    { "code", "LPCSTR", NULL },
    { "handle", "HANDLE", NULL },
    { "function", "HANDLE", NULL },
};

static LPJASSVAR jass_stackvalue(LPJASS j, int index);
static JASSTYPEID jass_getvarbasetype(LPCJASSVAR var);

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
    }
    return false;
}


DWORD __eq(LPJASS j) {
    return jass_pushboolean(j, var_eq(jass_stackvalue(j, 1), jass_stackvalue(j, 2)));
}

DWORD __ne(LPJASS j) {
    return jass_pushboolean(j, !var_eq(jass_stackvalue(j, 1), jass_stackvalue(j, 2)));
}

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
    UINAME name;
    LPSTR code;
    DWORD (*nativefunc)(LPJASS j);
    BOOL constant;
};

struct jass_dict {
    LPJASSDICT next;
    UINAME key;
    JASSVAR value;
};

struct jass_env {
    LPJASSDICT locals;
};

struct jass_s {
    LPJASSDICT globals;
    JASSTYPE types[MAX_JASS_TYPES];
    JASSFUNC functions[MAX_JASS_FUNCTIONS];
    JASSVAR stack[MAX_JASS_STACK];
    DWORD num_stack;
    DWORD num_functions;
    DWORD num_types;
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

BOOL is_operator(LPCSTR str) {
    return \
    !strcmp(str, "+") ||
    !strcmp(str, "-") ||
    !strcmp(str, "*") ||
    !strcmp(str, "/") ||
    !strcmp(str, "!=") ||
    !strcmp(str, "==") ||
    !strcmp(str, ">=") ||
    !strcmp(str, "<=") ||
    !strcmp(str, ">") ||
    !strcmp(str, "<");
}

BOOL is_comma(LPCSTR str) {
    return !strcmp(str, ",");
}

static LPCJASSFUNC find_function(LPCJASS j, LPCSTR name) {
    FOR_LOOP(i, j->num_functions) {
        if (!strcmp(j->functions[i].name, name)) {
            return &j->functions[i];
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
    FOR_LOOP(i, j->num_types) {
        if (!strcmp(j->types[i].name, name)) {
            return &j->types[i];
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

DWORD jass_pushfunction(LPJASS j, LPCJASSFUNC func) {
    JASS_ADD_STACK(j, var, jasstype_function);
    JASS_SET_VALUE(var, func, sizeof(*func));
    return 1;
}
 
DWORD jass_pushvalue(LPJASS j, LPCJASSVAR other) {
    JASS_ADD_STACK(j, var, get_base_type(other->type) - jass_types);
    jass_copy(j, var, other);
    return 1;
}

JASSFUNC jass_getoperator(LPCSTR str) {
    if (!strcmp(str, "+")) return MAKE(JASSFUNC, .nativefunc = __add);
    if (!strcmp(str, "-")) return MAKE(JASSFUNC, .nativefunc = __sub);
    if (!strcmp(str, "*")) return MAKE(JASSFUNC, .nativefunc = __mul);
    if (!strcmp(str, "/")) return MAKE(JASSFUNC, .nativefunc = __div);
    if (!strcmp(str, "!=")) return MAKE(JASSFUNC, .nativefunc = __ne);
    if (!strcmp(str, "==")) return MAKE(JASSFUNC, .nativefunc = __eq);
    if (!strcmp(str, ">=")) return MAKE(JASSFUNC, .nativefunc = __ge);
    if (!strcmp(str, "<=")) return MAKE(JASSFUNC, .nativefunc = __le);
    if (!strcmp(str, ">")) return MAKE(JASSFUNC, .nativefunc = __gt);
    if (!strcmp(str, "<")) return MAKE(JASSFUNC, .nativefunc = __lt);
    return MAKE(JASSFUNC);
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

typedef enum {
    TT_UNKNOWN,
    TT_VALUE,
    TT_OPERATOR,
} TOKENTYPE;

static DWORD jass_doparser(LPJASS j, LPPARSER p, bool single, LPJASSDICT locals) {
    LPCSTR tok = NULL;
    LPCJASSFUNC f = NULL;
    LPCJASSVAR v = NULL;
    DWORD stacksize = j->num_stack;
    TOKENTYPE expect = TT_VALUE;
    while (*(tok = parse_token(p)) && strcmp(tok, ")")) {
        if (expect == TT_VALUE) {
            if (!strcmp(tok, "(")) {
                DWORD const top = jass_top(j);
                jass_doparser(j, p, false, locals);
                jass_call(j, jass_top(j) - top);
            } else if (*tok == '"') {
                jass_pushstringlen(j, tok+1, (DWORD)strlen(tok)-2);
            } else if (!strcmp(tok, JASS_NULL)) {
                jass_pushnull(j);
            } else if (!strcmp(tok, JASS_TRUE)) {
                jass_pushboolean(j, true);
            } else if (!strcmp(tok, JASS_FALSE)) {
                jass_pushboolean(j, false);
            } else if (!strcmp(tok, JASS_UNM)) {
                JASSFUNC func = MAKE(JASSFUNC, .nativefunc = __unm);
                jass_pushfunction(j, &func);
                jass_doparser(j, p, true, locals);
                jass_call(j, 1);
            } else if (is_integer(tok)) {
                jass_pushinteger(j, atoi(tok));
            } else if (is_float(tok)) {
                jass_pushnumber(j, atof(tok));
            } else if ((f = find_function(j, tok))) {
                jass_pushfunction(j, f);
                if (!strcmp(peek_token(p), "(")) {
                    DWORD const top = jass_top(j);
                    parse_token(p); // skip "("
                    jass_doparser(j, p, false, locals);
                    jass_call(j, jass_top(j) - top);
                }
            } else if ((v = find_dict(j->globals, tok))) {
                jass_pushvalue(j, v);
            } else if ((v = find_dict(locals, tok))) {
                jass_pushvalue(j, v);
            } else {
                assert(false);
            }
            expect = TT_OPERATOR;
        } else if (is_operator(tok)) {
            JASSFUNC func = jass_getoperator(tok);
            jass_pushfunction(j, &func);
            jass_swap(j, -1, -2);
            jass_doparser(j, p, true, locals);
            jass_call(j, 2);
            expect = TT_OPERATOR;
        } else if (!strcmp(tok, JASS_COMMA)) {
            expect = TT_VALUE;
        } else {
            assert(false);
        }
        if (single)
            break;
    }
    return j->num_stack - stacksize;
}

//void remove_leading_whitespace(LPSTR str) {
//    LPSTR start = str;
//    while (*start && isspace(*start)) {
//        start++;
//    }
//    memmove(str, start, strlen(start) + 1);
//}
//
//void remove_trailing_whitespace(LPSTR str) {
//    LPSTR end = str + strlen(str) - 1;
//    for (; end > str && isspace(*end); end--) {
//        *end = '\0';
//    }
//}

DWORD jass_eval(LPJASS j, LPCSTR from, LPCSTR to, LPJASSDICT locals) {
    LPSTR expr = gi.MemAlloc(to - from + 1);
    memcpy(expr, from, to - from);
//    remove_leading_whitespace(expr);
//    remove_trailing_whitespace(expr);
    PARSER p = { .buffer = expr, .delimiters = JASS_DELIM };
    DWORD retval = jass_doparser(j, &p, false, locals);
    gi.MemFree(expr);
    return retval;
}

LPJASSDICT parse_dict(LPJASS j, LPPARSER p, LPJASSDICT locals) {
    LPCSTR tok = parse_token(p);
    LPJASSDICT item = JASSALLOC(JASSDICT);
    if (!strcmp(tok, JASS_CONSTANT)) {
        item->value.constant = true;
        item->value.type = find_type(j, parse_token(p));
    } else {
        item->value.constant = false;
        item->value.type = find_type(j, tok);
    }
    tok = parse_token(p);
    if (!strcmp(tok, JASS_ARRAY)) {
        item->value.array = true;
        tok = parse_token(p);
    }
    strcpy(item->key, tok);
    if (strcmp(peek_token(p), "=")) {
        return item;
    } else {
        tok = parse_token(p);
    }
    LPCSTR newline = strchr(p->buffer, '\n');
    DWORD stack = jass_eval(j, p->buffer, newline, locals);
    if (stack > 0) {
        jass_copy(j, &item->value, j->stack + jass_top(j));
        jass_pop(j, 1);
    }
    p->buffer = newline+1;
    return item;
}

static void keyword_type(LPJASS j, LPPARSER p) {
    LPJASSTYPE type = &j->types[j->num_types++];
    strcpy(type->name, parse_token(p));
    if (!strcmp(parse_token(p), "extends")) {
        type->inherit = find_type(j, parse_token(p));
    } else {
        parser_error(p);
        j->num_types--;
    }
}

static void keyword_globals(LPJASS j, LPPARSER p) {
    while (strcmp(peek_token(p), "endglobals")) {
        LPJASSDICT item = parse_dict(j, p, NULL);
        ADD_TO_LIST(item, j->globals);
//        printf("%s\n", item->key);
    }
    parse_token(p); // eat "endglobals"
}

static LPCSTR parse_takes(LPJASS j, LPJASSFUNC f, LPCSTR tok, LPPARSER p) {
    if (strcmp(tok, "takes")) {
        parser_error(p);
        return NULL;
    }
    tok = parse_token(p);
    if (strcmp(tok, "nothing")) {
        while (true) {
            LPJASSARG arg = JASSALLOC(JASSARG);
            arg->type = find_type(j, tok);
            strcpy(arg->name, parse_token(p));
            tok = parse_token(p);
            if (strcmp(tok, ",")) {
                return tok;
            }
            tok = parse_token(p);
        }
    } else {
        return parse_token(p);
    }
}

static void parse_returns(LPJASS j, LPJASSFUNC f, LPCSTR tok, LPPARSER p) {
    if (strcmp(tok, "returns")) {
        parser_error(p);
        return;
    }
    f->returns = find_type(j, parse_token(p));
}

void parse_function_header(LPJASS j, LPPARSER p, LPJASSFUNC f) {
    LPCSTR tok = parse_token(p);
    strcpy(f->name, tok);
    tok = parse_token(p);
    tok = parse_takes(j, f, tok, p);
    parse_returns(j, f, tok, p);
}

//void print_header(LPCJASSFUNC f) {
//    if (!f->returns) {
//        printf("void %s(", f->name);
//    } else {
//        printf("%s %s(", f->returns->name, f->name);
//    }
//    if (f->num_args > 0) {
//        FOR_LOOP(i, f->num_args) {
//            if (i > 0) printf(", ");
//            printf("%s %s", f->args[i].type, f->args[i].name);
//        }
//    } else {
//        printf("void");
//    }
//    printf(")\n");
//}

//void print_api(LPCJASSFUNC f) {
//    printf("DWORD %s(LPJASS j) {\n", f->name);
//    FOR_LOOP(i, f->num_args) {
//        LPCJASSARG arg = f->args+i;
//        LPCJASSTYPE type = get_base_type(arg->type);
//        if (get_base_type(type) == jass_types + jasstype_handle) {
//            printf("  // %s %s = jass_check%s(j, %d, \"%s\");\n", type->ctype, arg->name, type->name, i+1, arg->type->name);
//        } else {
//            printf("  // %s %s = jass_check%s(j, %d);\n", type->ctype, arg->name, type->name, i+1);
//        }
//    }
//    if (!f->returns) {
//        printf("  return 0;\n");
//    } else if (get_base_type(f->returns) == jass_types + jasstype_handle) {
//        printf("  return jass_pushhandle(j, 0, \"%s\");\n", f->returns->name);
//    } else {
//        printf("  return jass_push%s(j, 0);\n", f->returns->name);
//    }
//    printf("}\n");
//}
//
//void print_header(LPCJASSFUNC f) {
//    printf("DWORD %s(LPJASS j);\n", f->name);
//}
//
//void print_module(LPCJASSFUNC f) {
//    printf("{ \"%s\", %s },\n", f->name, f->name);
//}

static void keyword_native(LPJASS j, LPPARSER p) {
    LPJASSFUNC f = &j->functions[j->num_functions++];
    parse_function_header(j, p, f);
//    print_header(f);
//    print_api(f);
//    print_module(f);
    LPJASSMODULE mod = find_in_array(jass_funcs, sizeof(JASSMODULE), f->name);
    if (mod) {
        f->nativefunc = mod->func;
    }
}

static void keyword_function(LPJASS j, LPPARSER p) {
    LPJASSFUNC f = &j->functions[j->num_functions++];
    parse_function_header(j, p, f);
    LPCSTR start = p->buffer;
    LPCSTR tok = parse_token(p);
    while (true) {
        if (!strcmp(tok, "endfunction")) {
            long const len = p->buffer - start - sizeof(tok);
            f->code = gi.MemAlloc(len + 1);
            memcpy(f->code, start, len);
            break;
        }
        tok = parse_token(p);
    }
}

static void statement_set(LPJASS j, LPJASSENV env, LPPARSER p) {
    return;
}

static void statement_call(LPJASS j, LPJASSENV env, LPPARSER p) {
    LPCSTR newline = strchr(p->buffer, '\n');
    jass_eval(j, p->buffer, newline, env->locals);
    p->buffer = newline+1;
}

static void statement_local(LPJASS j, LPJASSENV env, LPPARSER p) {
    LPJASSDICT local = parse_dict(j, p, env->locals);
    ADD_TO_LIST(local, env->locals);
}

static void statement_if(LPJASS j, LPJASSENV env, LPPARSER p) {
    LPCSTR tok = parse_token(p);
    if (strcmp(tok, "(")) {
        parser_error(p);
        return;
    }
    if (jass_doparser(j, p, false, env->locals) != 1) {
        parser_error(p);
        return;
    }
    BOOL const cond = jass_toboolean(j, -1);
    jass_pop(j, 1);
    if (cond) {
        
    } else {
        
    }
}

static parseClass_t global_keywords[] = {
    { "globals", keyword_globals },
    { "function", keyword_function },
    { "type", keyword_type },
    { "native", keyword_native },
    F_END,
};

static parseStatement_t function_keywords[] = {
    { "set", statement_set },
    { "call", statement_call },
    { "local", statement_local },
    { "if", statement_if },
    F_END,
};

BOOL JASS_Parse_Buffer(LPJASS j, LPCSTR fileName, LPSTR buffer2) {
    LPSTR buffer = buffer2;
    remove_comments(buffer);
    remove_bom(buffer, strlen(buffer));
    PARSER p = { .buffer = buffer, .delimiters = JASS_DELIM };
    LPCSTR tok = NULL;
    
    while (*(tok = parse_token(&p))) {
        if (!strcmp(tok, JASS_CONSTANT))
            continue;
        parseClass_t *keyword = find_in_array(global_keywords, sizeof(*global_keywords), tok);
        if (keyword) {
            keyword->func(j, &p);
        } else {
            parser_error(&p);
            return false;
        }
    }
    if (p.error) {
        fprintf(stderr, "Failed to parse %s\n", fileName);
        return false;
    } else {
        return true;
    }
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
    LPCJASSFUNC func = var->value;
    assert(func->nativefunc || func->code);
    DWORD old_stack_offset = j->stack_offset;
    j->stack_offset = j->num_stack - args - 1;
    DWORD ret = 0;
    if (func->nativefunc) {
        ret = func->nativefunc(j);
    } else {
        PARSER p = { .buffer = func->code, .delimiters = JASS_DELIM };
        LPCSTR tok = NULL;
        JASSENV env = { 0 };
        while (*(tok = parse_token(&p))) {
            parseStatement_t *stat = find_in_array(&function_keywords, sizeof(*function_keywords), tok);
            if (stat) {
                stat->func(j, &env, &p);
            } else {
                parser_error(&p);
                return;
            }
        }
        if (p.error) {
            fprintf(stderr, "Failed to parse function %s\n", func->name);
        }
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
