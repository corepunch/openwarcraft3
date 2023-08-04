#include "jass_parser.h"

#define ALLOC(type) gi.MemAlloc(sizeof(type))
#define FREE(val) SAFE_DELETE(val, gi.MemFree)
#define PARSER(NAME) static LPTOKEN NAME(LPPARSER p)

typedef LPTOKEN (*LPGRAMMARFUNC)(LPPARSER);

typedef struct {
    LPCSTR name;
    LPGRAMMARFUNC func;
} parseClass_t;

extern parseClass_t function_keywords[];

BOOL is_operator(LPCSTR str);
BOOL is_integer(LPCSTR tok);
BOOL is_float(LPCSTR tok);
BOOL is_identifier(LPCSTR str);
BOOL is_string(LPCSTR tok);

LPSTR read_identifier(LPPARSER p) {
    if (is_identifier(peek_token(p))) {
        return strdup(parse_token(p));
    } else {
        return NULL;
    }
}

static void parser_throw(LPPARSER p, LPCSTR error) {
    parser_error(p);
}

static LPGRAMMARFUNC eat_keyword(LPPARSER p, parseClass_t *keywords) {
    for (parseClass_t *cl = keywords; cl->name; cl++) {
        if (eat_token(p, cl->name)) {
            return cl->func;
        }
    }
    return NULL;
}

static BOOL parse_body(LPPARSER p, LPTOKEN function) {
    LPTOKEN token = NULL;
    LPGRAMMARFUNC func = eat_keyword(p, function_keywords);
    if (func && (token = func(p))) {
        PUSH_BACK(TOKEN, token, function->body);
    } else {
        parser_throw(p, "error parsing function");
        return false;
    }
    return true;
}

static LPTOKEN alloc_token(TOKENTYPE type) {
    LPTOKEN token = ALLOC(TOKEN);
    token->type = type;
    return token;
}

PARSER(parse_identifier) {
    LPTOKEN token = alloc_token(TT_IDENTIFIER);
    token->value = read_identifier(p);
    return token;
}

PARSER(keyword_type) {
    LPTOKEN token = alloc_token(TT_TYPEDEF);
    token->value = read_identifier(p);
    if (eat_token(p, "extends")) {
        token->extends = parse_identifier(p);
    }
    return token;
}

PARSER(parse_args) {
    if (eat_token(p, "nothing")) {
        return NULL;
    }
    LPTOKEN args = NULL;
    while (!args || eat_token(p, ",")) {
        LPTOKEN arg = alloc_token(TT_VARDECL);
        arg->value = read_identifier(p);
        arg->name = read_identifier(p);
        PUSH_BACK(TOKEN, arg, args);
    }
    return args;
}

PARSER(parse_function_decl) {
    LPTOKEN token = alloc_token(TT_TYPEDEF);
    token->value = read_identifier(p);
    if (eat_token(p, "takes")) {
        token->args = parse_args(p);
    }
    if (eat_token(p, "returns")) {
        token->returns = parse_identifier(p);
    }
    return token;
}

PARSER(keyword_native) {
    LPTOKEN token = parse_function_decl(p);
    token->flags |= TF_NATIVE;
    return token;
}

PARSER(keyword_constant) {
    if (eat_token(p, "native")) {
        LPTOKEN token = keyword_native(p);
        token->flags |= TF_CONSTANT;
        return token;
    } else {
        parser_throw(p, "expected native after constant");
        return NULL;
    }
}

PARSER(parse_expression) {
    LPCSTR tok = parse_token(p);
    LPTOKEN left = NULL;
    if (!strcmp(tok, "-")) {
        left = alloc_token(TT_OPERATOR);
        left->value = strdup(parse_token(p));
        left->body = parse_expression(p);
        return left;
    } else if (is_integer(tok)) {
        left = alloc_token(TT_INTEGER);
        left->value = strdup(tok);
    } else if (is_float(tok)) {
        left = alloc_token(TT_REAL);
        left->value = strdup(tok);
    } else if (is_string(tok)) {
        left = alloc_token(TT_STRING);
        left->value = strdup(tok);
    } else if (!strcmp(tok, "true") || !strcmp(tok, "false")) {
        left = alloc_token(TT_BOOLEAN);
        left->value = strdup(tok);
    } else if (is_identifier(tok)) {
        left = alloc_token(TT_IDENTIFIER);
        left->value = strdup(tok);
        if (eat_token(p, "(")) {
            left->type = TT_CALL;
            left->args = parse_expression(p);
        }
    } else {
        parser_throw(p, "Invalid identifier");
        return NULL;
    }
    if (is_operator(peek_token(p))) {
        LPTOKEN oper = alloc_token(TT_OPERATOR);
        oper->value = strdup(parse_token(p));
        if (!strcmp(peek_token(p), "=")) {
            LPSTR op = oper->value;
            oper->value = gi.MemAlloc(3);
            oper->value[0] = *op;
            oper->value[1] = *parse_token(p);
            FREE(op);
        }
        oper->left = left;
        oper->right = parse_expression(p);
        return oper;
    }
    if (eat_token(p, ",")) {
        left->next = parse_expression(p);
        return left;
    }
    if (eat_token(p, ")")) {
        return left;
    }
    return left;
}

PARSER(keyword_globals) {
    LPTOKEN globals = NULL;
    while (!eat_token(p, "endglobals")) {
        LPTOKEN token = alloc_token(TT_VARDECL);
        if (eat_token(p, "constant")) {
            token->flags |= TF_CONSTANT;
        }
        token->value = read_identifier(p);
        if (eat_token(p, "array")) {
            token->flags |= TF_ARRAY;
        }
        token->name = read_identifier(p);
        if (eat_token(p, "=")) {
            token->init = parse_expression(p);
        }
        PUSH_BACK(TOKEN, token, globals);
    }
    return globals;
}

PARSER(statement_set) {
    LPTOKEN token = alloc_token(TT_SET);
    token->name = read_identifier(p);
    if (eat_token(p, "=")) {
        token->init = parse_expression(p);
    }
    return token;
}

PARSER(statement_call) {
    return parse_expression(p);
}

PARSER(statement_local) {
    LPTOKEN token = alloc_token(TT_VARDECL);
    token->value = read_identifier(p);
    token->name = read_identifier(p);
    if (eat_token(p, "=")) {
        token->init = parse_expression(p);
    }
    return token;
}

PARSER(statement_if) {
    LPTOKEN token = alloc_token(TT_IF);
    LPTOKEN target = token;
    if (eat_token(p, "(")) {
        token->condition = parse_expression(p);
    }
    if (!eat_token(p, "then")) {
        parser_throw(p, "THEN expected");
        FREE(token);
        return NULL;
    }
    while (!eat_token(p, "endif")) {
        if (eat_token(p, "else")) {
            target = alloc_token(TT_ELSE);
            token->elseblock = target;
        } else if (!parse_body(p, target)) {
            FREE(token);
            return NULL;
        }
    }
    return token;
}

PARSER(statement_exitwhen) {
    LPTOKEN token = alloc_token(TT_EXITWHEN);
    token->condition = parse_expression(p);
    return token;
}

PARSER(statement_loop) {
    LPTOKEN loop = alloc_token(TT_LOOP);
    while (!eat_token(p, "endloop")) {
        if (eat_token(p, "exitwhen")) {
            LPTOKEN exitwhen = statement_exitwhen(p);
            PUSH_BACK(TOKEN, exitwhen, loop->body);
        } else if (!parse_body(p, loop)) {
            FREE(loop);
            return NULL;
        }
    }
    return loop;
}

PARSER(statement_return) {
    LPTOKEN ret = alloc_token(TT_RETURN);
    ret->body = parse_expression(p);
    return ret;
}

parseClass_t function_keywords[] = {
    { "set", statement_set },
    { "call", statement_call },
    { "local", statement_local },
    { "if", statement_if },
    { "loop", statement_loop },
    { "return", statement_return },
    { NULL },
};

PARSER(keyword_function) {
    LPTOKEN function = parse_function_decl(p);
    while (!eat_token(p, "endfunction")) {
        if (!parse_body(p, function)) {
            FREE(function);
            return NULL;
        }
    }
    return function;
}

static parseClass_t global_keywords[] = {
    { "globals", keyword_globals },
    { "function", keyword_function },
    { "type", keyword_type },
    { "native", keyword_native },
    { "constant", keyword_constant },
    { NULL },
};

LPTOKEN JASS_ParseTokens(LPPARSER p) {
    LPTOKEN tokens = NULL, token = NULL;
    while (*peek_token(p)) {
        LPGRAMMARFUNC func = eat_keyword(p, global_keywords);
        if (func && (token = func(p))) {
            PUSH_BACK(TOKEN, token, tokens);
        } else {
            parser_throw(p, "unknwon keyword");
            return NULL;
        }
    }
    return tokens;
}
