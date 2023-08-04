#ifndef jass_parser_h
#define jass_parser_h

#include "../g_local.h"

KNOWN_AS(token, TOKEN);

typedef enum {
    TT_UNKNOWN,
    TT_VALUE,
    TT_OPERATOR,
    TT_FUNCTION,
    TT_TYPEDEF,
    TT_VARDECL,
    TT_IDENTIFIER,
    TT_CALL,
    TT_INTEGER,
    TT_REAL,
    TT_STRING,
    TT_BOOLEAN,
    TT_IF,
    TT_SET,
    TT_LOOP,
    TT_ELSE,
    TT_EXITWHEN,
    TT_RETURN,
} TOKENTYPE;

enum {
    TF_NATIVE = 1,
    TF_CONSTANT = 2,
    TF_ARRAY = 4,
};

struct token {
    LPSTR value;
    LPSTR name;
    TOKENTYPE type;
    DWORD flags;
    LPTOKEN init;
    LPTOKEN body;
    LPTOKEN next;
    LPTOKEN args;
    LPTOKEN left;
    LPTOKEN right;
    LPTOKEN extends;
    LPTOKEN returns;
    LPTOKEN condition;
    LPTOKEN elseblock;
};

LPTOKEN JASS_ParseTokens(LPPARSER p);

#endif
