#include "mpq_adapter.h"

#include "common.h"

bool ParserDone(parser_t *p) {
    return !*p->str || p->error;
}

bool ParserSingleLineComment(parser_t *p, LPCSTR str) {
    return str[0] == '/' && str[1] == '/';
}

bool ParserSpace(parser_t *p, LPCSTR str) {
    if (!p->reading_string) {
        if (isspace(*str)) return true;
        if (p->comma_space && *str == ',') return true;
        if (*str == '\"') return true;
    }
    return false;
}

void ParserError(parser_t *p) {
    p->error = true;
}

LPSTR FS_ReadFileIntoString(LPCSTR fileName) {
    HANDLE fp = FS_OpenFile(fileName);
    if (!fp)
        return NULL;
    DWORD const fileSize = SFileGetFileSize(fp, NULL);
    LPSTR buffer = MemAlloc(fileSize + 1);
    SFileReadFile(fp, buffer, fileSize, NULL, NULL);
    FS_CloseFile(fp);
    buffer[fileSize] = '\0';
    return buffer;
}

HANDLE FS_ReadFile(LPCSTR filename, LPDWORD size) {
    HANDLE fp = FS_OpenFile(filename);
    if (!fp)
        return NULL;
    *size = SFileGetFileSize(fp, NULL);
    LPSTR buffer = MemAlloc(*size);
    SFileReadFile(fp, buffer, *size, NULL, NULL);
    FS_CloseFile(fp);
    return buffer;
}

LPSTR ParserGetToken(parser_t *p) {
    for (; !ParserDone(p); p->str++) {
        if (ParserSingleLineComment(p, p->str)) {
            *p->tok = '\0';
            for (; p->str[1] != '\n' && p->str[1] != '\0'; p->str++)
            p->tok = p->token;
            if (p->tok != p->token) {
                return p->token;
            }
        }
        if (ParserSpace(p, p->str)) {
            if (p->tok != p->token) {
                *p->tok = '\0';
                p->tok = p->token;
                return p->token;
            }
        } else {
            *(p->tok++) = *p->str;
        }
    }
    if (p->tok != p->token) {
        *p->tok = '\0';
        p->tok = p->token;
        return p->token;
    } else {
        return NULL;
    }
}
