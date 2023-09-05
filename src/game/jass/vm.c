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
#define INF_LOOP_PROTECTION 1024

#define assert_type(var, type) assert(jass_checktype(var, type))
#define JASSALLOC(type) gi.MemAlloc(sizeof(type))

#define JASS_ADD_STACK(j, VAR, TYPE) \
LPJASSVAR VAR = &j->stack[j->num_stack++]; \
memset(VAR, 0, sizeof(*VAR)); \
VAR->type = &jass_types[TYPE];

#define JASS_SET_VALUE(VAR, VALUE, SIZE) \
jass_setnull(VAR); \
if (VALUE) { \
  (VAR)->value = gi.MemAlloc(SIZE); \
  memcpy((VAR)->value, VALUE, SIZE); \
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

KNOWN_AS(jass_array, JASSARRAY);
KNOWN_AS(jass_dict, JASSDICT);
KNOWN_AS(jass_arg, JASSARG);
KNOWN_AS(jass_env, JASSENV);

VMPROGRAM VM_Compile(LPCTOKEN token);
LPCMAPPLAYER currentplayer = NULL;
LPEDICT currentunit = NULL;
LPEDICT enteringunit = NULL;
LPEDICT leavingunit = NULL;

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
    DWORD *refcount;
    BOOL constant;
    BOOL array;
    struct {
        LPJASSDICT locals;
        DWORD returnstack;
        BOOL done;
    } env;
    LPJASSARRAY _array;
};

struct jass_type {
    LPCJASSTYPE inherit;
    LPJASSTYPE next;
    LPCSTR name;
};

struct jass_arg {
    LPJASSARG next;
    LPCJASSTYPE type;
    LPCSTR name;
};

JASSTYPE jass_types[] = {
    { NULL, NULL, "integer" },
    { NULL, NULL, "real" },
    { NULL, NULL, "string" },
    { NULL, NULL, "boolean" },
    { NULL, NULL, "code" },
    { NULL, NULL, "handle" },
    { NULL, NULL, "cfunction" },
};

static LPJASSVAR jass_stackvalue(LPJASS j, int index);
static JASSTYPEID jass_getvarbasetype(LPCJASSVAR var);
static DWORD jass_dotoken(LPJASS j, LPCTOKEN token);

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
        case jasstype_code: return !memcmp(a->value, b->value, sizeof(HANDLE));
        case jasstype_cfunction: return !memcmp(a->value, b->value, sizeof(HANDLE));
        case jasstype_handle: return !memcmp(a->value, b->value, sizeof(HANDLE));
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

DWORD __not(LPJASS j) {
    return jass_pushboolean(j, !jass_toboolean(j, 1));
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
    JASS_OPERATOR(__not),
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

struct jass_array {
    LPJASSARRAY next;
    DWORD index;
    JASSVAR value;
};

struct jass_dict {
    LPJASSDICT next;
    LPCSTR key;
    JASSVAR value;
};

struct jass_s {
    LPJASSDICT globals;
    LPJASSTYPE types;
    LPJASSFUNC functions;
    JASSVAR stack[MAX_JASS_STACK];
    DWORD num_stack;
    LPJASSVAR stack_pointer;
    LPTRIGGER host_trigger;
    gtriggeraction_t *current_action;
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

static HANDLE RunAction(HANDLE handle) {
    struct jass_s *j = handle;
    jass_pushfunction(j, j->current_action->func);
    jass_call(j, 0);
    gi.MemFree(handle);
    return NULL;
}

LPTRIGGER jass_gethosttrigger(LPJASS j) {
    return j->host_trigger;
}

BOOL jass_calltrigger(LPJASS j, LPTRIGGER trigger) {
    if (trigger->disabled)
        return false;
    FOR_EACH_LIST(gtriggercondition_t, cond, trigger->conditions) {
        jass_pushfunction(j, cond->expr);
        if (jass_call(j, 0) != 1 || !jass_popboolean(j)) {
            return false;
        }
    }
    FOR_EACH_LIST(gtriggeraction_t, action, trigger->actions) {
        struct jass_s *thread = JASSALLOC(struct jass_s);
        memcpy(thread, j, sizeof(struct jass_s));
        memset(thread->stack, 0, sizeof(thread->stack));
        thread->num_stack = 0;
        thread->host_trigger = trigger;
        thread->current_action = action;
        gi.CreateThread(RunAction, thread);
    }
    return true;
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

LPCJASSTYPE get_base_type(LPCJASSTYPE type) {
    while (type->inherit) {
        type = type->inherit;
    }
    return type;
}

void jass_setreturn(LPJASS j) {
    jass_stackvalue(j, 0)->env.done = true;
    jass_stackvalue(j, 0)->env.returnstack = j->num_stack;
}

BOOL jass_mustreturn(LPJASS j) {
    return jass_stackvalue(j, 0)->env.done;
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
        return j->stack_pointer + index;
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

static void jass_setnull(LPJASSVAR var);

static void jass_deletedict(LPJASSDICT dict) {
    SAFE_DELETE(dict->next, jass_deletedict);
    jass_setnull(&dict->value);
    gi.MemFree(dict);
}

void jass_setnull(LPJASSVAR var) {
    SAFE_DELETE(var->env.locals, jass_deletedict);
    switch (jass_getvarbasetype(var)) {
        case jasstype_handle:
            if (var->refcount && *var->refcount > 0) {
                (*var->refcount)--;
                var->value = NULL;
                var->refcount = NULL;
            } else {
                SAFE_DELETE(var->value, gi.MemFree);
                SAFE_DELETE(var->refcount, gi.MemFree);
            }
            break;
        case jasstype_code:
        case jasstype_cfunction:
            // skip
            break;
        default:
            SAFE_DELETE(var->value, gi.MemFree);
            break;
    }
}

//void jass_setenv(LPJASS j, int index, LPJASSDICT env) {
//    jass_stackvalue(j, index)->env.locals = env;
//}

BOOL is_handle_convertible(LPCJASSTYPE from, LPCJASSTYPE to) {
    if (from == to) {
        return true;
    } else if (from->inherit) {
        return is_handle_convertible(from->inherit, to);
    } else {
        return false;
    }
}

static LPJASSVAR ensure_array_value(LPJASS j, LPJASSVAR dest, DWORD index) {
    FOR_EACH_LIST(JASSARRAY, var, dest->_array) {
        if (var->index == index) {
            return &var->value;
        }
    }
    LPJASSARRAY jv = JASSALLOC(JASSARRAY);
    jv->value.type = dest->type;
    jv->index = index;
    ADD_TO_LIST(jv, dest->_array);
    return &jv->value;
}

void jass_copy(LPJASS j, LPJASSVAR var, LPCJASSVAR other) {
    FLOAT fval = 0;
    jass_setnull(var);
    if (other->_array) {
        var->type = other->type;
        FOR_EACH_LIST(JASSARRAY, srcar, other->_array) {
            jass_copy(j, ensure_array_value(j, var, srcar->index), &srcar->value);
        }
        return;
    } else if (!other->value) {
        return;
    } else switch (jass_getvarbasetype(var)) {
        case jasstype_integer:
            assert(other->type == var->type);
            JASS_SET_VALUE(var, other->value, sizeof(LONG));
            break;
        case jasstype_handle:
            if (!is_handle_convertible(other->type, var->type)) {
                fprintf(stderr, "Warning: Passing %s to %s type\n", other->type->name, var->type->name);
            }
            var->value = other->value;
            var->refcount = other->refcount;
            if (var->refcount) {
                (*var->refcount)++;
            }
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
            assert(other->type == var->type);
            JASS_SET_VALUE(var, other->value, sizeof(BOOL));
            break;
        case jasstype_string:
            assert(other->type == var->type);
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

DWORD jass_pushhandle(LPJASS j, HANDLE value, LPCSTR type) {
    JASS_ADD_STACK(j, var, jasstype_handle);
    jass_setnull(var);
    var->type = find_type(j, type);
    var->value = value;
    var->refcount = gi.MemAlloc(sizeof(DWORD));
    return 1;
}

DWORD jass_pushlighthandle(LPJASS j, HANDLE value, LPCSTR type) {
    JASS_ADD_STACK(j, var, jasstype_handle);
    jass_setnull(var);
    var->type = find_type(j, type);
    var->value = value;
    var->refcount = gi.MemAlloc(sizeof(DWORD));
    *var->refcount = 1; // so that runtime won't ever delete it
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
        JASS_ADD_STACK(j, var, jasstype_code);
        var->value = (LPJASSFUNC)func;
        return 1;
    }
}

DWORD jass_pushvalue(LPJASS j, LPCJASSVAR other) {
    LPCJASSTYPE type = other->type;
    LPJASSVAR var = &j->stack[j->num_stack++];
    memset(var, 0, sizeof(*var));
    var->type = type;
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
        case jasstype_handle: return true;
        case jasstype_code: return true;
        default: return false;
    }
}

LPCSTR jass_checkstring(LPJASS j, int index) {
    LPCJASSVAR var = jass_stackvalue(j, index);
    assert_type(var, jasstype_string);
    return var->value;
}

LPCJASSFUNC jass_checkcode(LPJASS j, int index) {
    LPCJASSVAR var = jass_stackvalue(j, index);
    assert_type(var, jasstype_code);
    return var->value;
}

HANDLE jass_checkhandle(LPJASS j, int index, LPCSTR type) {
    LPCJASSVAR var = jass_stackvalue(j, index);
    assert(is_handle_convertible(var->type, find_type(j, type)));
    return var->value;
}

void jass_swap(LPJASS j, int a, int b) {
    JASSVAR tmp = *jass_stackvalue(j, a);
    *jass_stackvalue(j, a) = *jass_stackvalue(j, b);
    *jass_stackvalue(j, -2) = tmp;
}

BOOL jass_popboolean(LPJASS j) {
    BOOL value = jass_toboolean(j, -1);
    jass_pop(j, 1);
    return value;
}

DWORD jass_popinteger(LPJASS j) {
    DWORD value = jass_checkinteger(j, -1);
    jass_pop(j, 1);
    return value;
}

HANDLE VM_TriggerThread(HANDLE arg) {
    JASS_ExecuteFunc(level.vm, "main");
    return NULL;
}

DWORD VM_EvalInteger(LPJASS j, LPCTOKEN token) {
    return jass_pushinteger(j, atoi(token->primary));
}

DWORD VM_EvalReal(LPJASS j, LPCTOKEN token) {
    return jass_pushnumber(j, atof(token->primary));
}

DWORD VM_EvalString(LPJASS j, LPCTOKEN token) {
    return jass_pushstring(j, token->primary);
}

DWORD VM_EvalBoolean(LPJASS j, LPCTOKEN token) {
    return jass_pushboolean(j, atob(token->primary));
}

DWORD VM_EvalIdentifier(LPJASS j, LPCTOKEN token) {
    LPCJASSFUNC f = NULL;
    LPCJASSVAR v = NULL;
    if (token->flags & TF_FUNCTION) {
        if ((f = find_function(j, token->primary))) {
            return jass_pushfunction(j, f);
        } else {
            return jass_pushnull(j);
        }
    } else if ((v = find_dict(j->globals, token->primary))) {
        return jass_pushvalue(j, v);
    } else if ((v = find_dict(jass_stackvalue(j, 0)->env.locals, token->primary))) {
        return jass_pushvalue(j, v);
    } else {
        return jass_pushnull(j);
    }
}

DWORD VM_EvalArrayAccess(LPJASS j, LPCTOKEN token) {
    assert(jass_dotoken(j, token->index) == 1);
    DWORD index_val = jass_popinteger(j);
    VM_EvalIdentifier(j, token);
    LPJASSVAR var = jass_stackvalue(j, -1);
    LPJASSVAR item = ensure_array_value(j, var, index_val);
    jass_pop(j, 1);
    JASSVAR tmp;
    memcpy(&tmp, item, sizeof(JASSVAR));
    jass_pushvalue(j, &tmp);
    return 1;
}

DWORD VM_EvalFourCC(LPJASS j, LPCTOKEN token) {
    return jass_pushinteger(j, *(DWORD *)token->primary);
}

DWORD VM_EvalCall(LPJASS j, LPCTOKEN token) {
    LPCJASSFUNC f = NULL;
    LPJASSCFUNCTION cf = NULL;
    DWORD stacksize = j->num_stack;
    if (!strcmp(token->primary, "CommentString") && token->args) {
        fprintf(stdout, "%s\n", token->args->primary);
        return 0;
    } else if ((f = find_function(j, token->primary))) {
        DWORD args = 0;
        jass_pushfunction(j, f);
        FOR_EACH_LIST(TOKEN, arg, token->args) {
            jass_dotoken(j, arg);
            args++;
        }
        jass_call(j, args);
        return j->num_stack - stacksize;
    } else if ((cf = find_cfunction(j, token->primary))) {
        DWORD args = 0;
        jass_pushcfunction(j, cf);
        FOR_EACH_LIST(TOKEN, arg, token->args) {
            jass_dotoken(j, arg);
            args++;
        }
        jass_call(j, args);
        return j->num_stack - stacksize;
    } else {
        fprintf(stderr, "Can't find function %s\n", token->primary);
        assert(false);
        return 0;
    }
}

static struct {
    TOKENTYPE tokentype;
    DWORD (*func)(LPJASS j, LPCTOKEN token);
} token_types[] = {
    { TT_INTEGER, VM_EvalInteger },
    { TT_REAL, VM_EvalReal },
    { TT_STRING, VM_EvalString },
    { TT_BOOLEAN, VM_EvalBoolean },
    { TT_IDENTIFIER, VM_EvalIdentifier },
    { TT_ARRAYACCESS, VM_EvalArrayAccess },
    { TT_FOURCC, VM_EvalFourCC },
    { TT_CALL, VM_EvalCall },
};

DWORD jass_dotoken(LPJASS j, LPCTOKEN token) {
    if (!token)
        return 0;
    FOR_LOOP(idx, sizeof(token_types)/sizeof(*token_types)) {
        if (token_types[idx].tokentype == token->type) {
            return token_types[idx].func(j, token);
        }
    }
    assert(false);
    return 0;
}

static void jass_set_value(LPJASS j, LPJASSVAR dest, LPCTOKEN init) {
    DWORD stack = jass_dotoken(j, init);
    assert(stack == 1);
    jass_copy(j, dest, j->stack + jass_top(j));
    jass_pop(j, 1);
}

static void jass_set_array_value(LPJASS j, LPJASSVAR dest, LPCTOKEN index, LPCTOKEN init) {
    assert(jass_dotoken(j, index) == 1);
    DWORD index_val = jass_popinteger(j);
    LPJASSVAR index_dest = ensure_array_value(j, dest, index_val);
    assert(jass_dotoken(j, init) == 1);
    jass_copy(j, index_dest, j->stack + jass_top(j));
    jass_pop(j, 1);
}

LPJASSDICT parse_dict(LPJASS j, LPCTOKEN token) {
    LPJASSDICT item = JASSALLOC(JASSDICT);
    item->value.constant = token->flags & TF_CONSTANT;
    item->value.array = token->flags & TF_ARRAY;
    item->value.type = find_type(j, token->primary);
    item->key = token->secondary;
    if (token->init) {
        jass_set_value(j, &item->value, token->init);
    }
    return item;
}

#define TOKENFUNC(NAME) void eval_##NAME(LPJASS j, LPCTOKEN token)
#define TOKENEVAL(NAME) { #NAME, TT_##NAME, eval_##NAME }

TOKENFUNC(TOKENS);
TOKENFUNC(SINGLETOKEN);

TOKENFUNC(TYPEDEF) {
    LPJASSTYPE type = JASSALLOC(JASSTYPE);
    type->name = token->primary;
    type->inherit = find_type(j, token->secondary);
    ADD_TO_LIST(type, j->types);
}

BOOL uses_localplayer(LPCTOKEN token) {
    if (token->type == TT_CALL && !strcmp(token->primary, "GetLocalPlayer")) {
        return true;
    }
    FOR_EACH_LIST(TOKEN, arg, token->args) {
        if (uses_localplayer(arg)) {
            return true;
        }
    }
    return false;
}

TOKENFUNC(IF) {
    if (uses_localplayer(token->condition)) {
        FOR_LOOP(i, MAX_PLAYERS) {
            currentplayer = level.mapinfo->players+i;
            jass_dotoken(j, token->condition);
            if (jass_popboolean(j)) {
                eval_TOKENS(j, token->body);
            }
            currentplayer = NULL;
        }
        assert(!token->elseblock);
    } else while (token) {
        if (!token->condition) {
            eval_TOKENS(j, token->body);
            return;
        }
        jass_dotoken(j, token->condition);
        if (jass_popboolean(j)) {
            eval_TOKENS(j, token->body);
            return;
        }
        token = token->elseblock;
    }
}

TOKENFUNC(SET) {
    LPJASSVAR v = NULL;
    if ((v = find_dict(j->globals, token->secondary))) {
        if (token->index) {
            return jass_set_array_value(j, v, token->index, token->init);
        } else {
            return jass_set_value(j, v, token->init);
        }
    } else if ((v = find_dict(jass_stackvalue(j, 0)->env.locals, token->secondary))) {
        if (token->index) {
            return jass_set_array_value(j, v, token->index, token->init);
        } else {
            return jass_set_value(j, v, token->init);
        }
    } else {
        fprintf(stderr, "Can't find variable %s\n", token->primary);
    }
}

TOKENFUNC(VARDECL) {
    LPJASSDICT vardecl = parse_dict(j, token);
    ADD_TO_LIST(vardecl, jass_stackvalue(j, 0)->env.locals);
}

TOKENFUNC(GLOBAL) {
    LPJASSDICT global = parse_dict(j, token);
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
    jass_dotoken(j, token);
}

TOKENFUNC(LOOP) {
    for (DWORD i = 0;; i++) {
        FOR_EACH_LIST(TOKEN const, tok, token->body) {
            if (jass_mustreturn(j)) {
                return;
            } else if (token->type == TT_RETURN) {
                jass_setreturn(j);
                jass_dotoken(j, token->body);
                return;
            } else if (tok->type == TT_EXITWHEN) {
                jass_dotoken(j, tok->condition);
                if (jass_popboolean(j)) {
                    return;
                }
            } else {
                eval_SINGLETOKEN(j, tok);
            }
        }
        assert(i < INF_LOOP_PROTECTION);
    }
}

struct {
    LPCSTR name;
    TOKENTYPE type;
    void (*func)(LPJASS, LPCTOKEN);
} token_eval[] = {
    TOKENEVAL(TYPEDEF),
    TOKENEVAL(FUNCTION),
    TOKENEVAL(VARDECL),
    TOKENEVAL(GLOBAL),
    TOKENEVAL(CALL),
    TOKENEVAL(IF),
    TOKENEVAL(SET),
    TOKENEVAL(LOOP),
};

TOKENFUNC(SINGLETOKEN) {
    FOR_LOOP(index, sizeof(token_eval) / sizeof(*token_eval)) {
        if (token->type == token_eval[index].type) {
            token_eval[index].func(j, token);
            return;
        }
    }
    fprintf(stderr, "Can't evaluate token of type %d\n", token->type);
    assert(false);
}

TOKENFUNC(TOKENS) {
    FOR_EACH_LIST(TOKEN const, tok, token) {
        if (jass_mustreturn(j)) {
            return;
        } else if (tok->type == TT_RETURN) {
            jass_setreturn(j);
            jass_dotoken(j, tok->body);
        } else {
            eval_SINGLETOKEN(j, tok);
        }
    }
}

BOOL JASS_Parse_Buffer(LPJASS j, LPCSTR fileName, LPSTR buffer2) {
    LPSTR buffer = buffer2;
    gi.TextRemoveComments(buffer);
    gi.TextRemoveBom(buffer);
    LPTOKEN program = JASS_ParseTokens(&MAKE(PARSER, .buffer = buffer, .delimiters = JASS_DELIM));
//    VM_Compile(program);
    eval_TOKENS(j, program);
    return true;
}

LPJASS JASS_Allocate(void) {
    LPJASS j = JASSALLOC(JASS);
    j->stack_pointer = j->stack;
    return j;
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

#define DEBUG_JASS

#ifdef DEBUG_JASS
static int depth = 0, callnum = 0;
#endif

DWORD jass_call(LPJASS j, DWORD args) {
    LPJASSVAR root = &j->stack[j->num_stack - args - 1];
    LPJASSVAR old_stack_pointer = j->stack_pointer;
    DWORD ret = 0;
    j->stack_pointer = &j->stack[j->num_stack - args - 1];
#ifdef DEBUG_JASS
    callnum++;
    depth++;
    FOR_LOOP(i, depth) printf(" ");
#endif
    if (jass_getvarbasetype(root) == jasstype_cfunction) {
        LPJASSCFUNCTION func = *(LPJASSCFUNCTION *)root->value;
#ifdef DEBUG_JASS
        for (DWORD i = 0; jass_funcs[i].name; i++) {
            if (jass_funcs[i].func == func) {
                printf("%s (native)", jass_funcs[i].name);
                break;
            }
        }
        for (DWORD i = 0; jass_operators[i].name; i++) {
            if (jass_operators[i].func == func) {
                printf("%s (native)", jass_operators[i].name);
                break;
            }
        }
        printf("\n");
#endif
        ret = func(j);
    } else {
        LPCJASSFUNC func = root->value;
        LPJASSDICT locals = NULL;
        DWORD argnum = 1;
#ifdef DEBUG_JASS
        printf("%s\n", func->name);
#endif
        FOR_EACH_LIST(JASSARG, arg, func->args) {
            LPJASSDICT local = JASSALLOC(JASSDICT);
            local->key = arg->name;
            local->value.type = arg->type;
            jass_copy(j, &local->value, &j->stack_pointer[argnum]);
            PUSH_BACK(JASSDICT, local, locals);
            argnum++;
        }
//        printf("%s\n", func->name);
        root->env.done = false;
        root->env.returnstack = -1;
        root->env.locals = locals;
        eval_TOKENS(j, func->code);
        if (root->env.returnstack != -1) {
            ret = j->num_stack - root->env.returnstack;
        }
    }
    LPJASSVAR last = &j->stack[j->num_stack - ret];
    for (LPJASSVAR it = root; it < last; it++) jass_setnull(it);
    memmove(root, last, ret * sizeof(JASSVAR));
    j->num_stack -= last - root;
    j->stack_pointer = old_stack_pointer;
#ifdef DEBUG_JASS
    depth--;
#endif
    return ret;
}

void JASS_ExecuteFunc(LPJASS j, LPCSTR name) {
    LPCJASSFUNC f = find_function(j, name);
    if (f) {
        jass_pushfunction(j, f);
        jass_call(j, 0);
    }
}
