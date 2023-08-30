#ifndef jass_parser_h
#define jass_parser_h

#include "../g_local.h"

KNOWN_AS(token, TOKEN);

typedef enum {
    TT_UNKNOWN,
    TT_VALUE,
    TT_FUNCTION,
    TT_TYPEDEF,
    TT_VARDECL,
    TT_GLOBAL,
    TT_IDENTIFIER,
    TT_ARRAYACCESS,
    TT_CALL,
    TT_INTEGER,
    TT_REAL,
    TT_STRING,
    TT_FOURCC,
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
    TF_FUNCTION = 8,
};

struct token {
    LPSTR primary;
    LPSTR secondary;
    TOKENTYPE type;
    DWORD flags;
    LPTOKEN init;
    LPTOKEN body;
    LPTOKEN next;
    LPTOKEN args;
    LPTOKEN condition;
    LPTOKEN elseblock;
    LPTOKEN index;
};

LPTOKEN JASS_ParseTokens(LPPARSER p);

#endif
