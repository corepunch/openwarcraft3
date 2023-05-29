#include <StormLib.h>

#include "common.h"

struct configValue_s {
    char Section[64];
    char Name[64];
    LPSTR Value;
    struct configValue_s *next;
};

bool ParserDone(parser_t *p) {
    return !*p->str || p->error;
}

bool ParserComment(parser_t *p, LPCSTR str) {
    return str[0] == '/' && str[1] == '/';
}

bool ParserSpace(parser_t *p, LPCSTR str) {
    return isspace(*str) || (p->comma_space && *str == ',') || *str == '"';
}

LPSTR FS_ReadFileIntoString(LPCSTR fileName) {
    HANDLE fp = FS_OpenFile(fileName);
    DWORD const fileSize = SFileGetFileSize(fp, NULL);
    LPSTR buffer = MemAlloc(fileSize + 1);
    SFileReadFile(fp, buffer, fileSize, NULL, NULL);
    SFileCloseFile(fp);
    buffer[fileSize] = '\0';
    return buffer;
}


LPSTR ParserGetToken(parser_t *p) {
    for (; !ParserDone(p); p->str++) {
        bool has_token = p->tok != p->token;
        if (ParserComment(p, p->str)) {
            *p->tok = '\0';
            for (; p->str[1] != '\n' && p->str[1] != '\0'; p->str++)
            p->tok = p->token;
            if (has_token) {
                return p->token;
            }
        }
        if (ParserSpace(p, p->str)) {
            if (has_token) {
                *p->tok = '\0';
                p->tok = p->token;
                return p->token;
            }
        } else {
            *(p->tok++) = *p->str;
        }
    }
    return NULL;
}

configValue_t *INI_ParseConfig(parser_t *p) {
    configValue_t *config = NULL;
    PATHSTR currentSection = { 0 };
    for (LPSTR tok = ParserGetToken(p);
         !ParserDone(p);
         tok = ParserGetToken(p))
    {
        if (tok[0] != '[' || tok[strlen(tok) - 1] != ']') {
            if (!*currentSection) {
                p->error = true;
                return NULL;
            } else {
                LPSTR eq = strstr(tok, "=");
                if (!eq) {
                    p->error = true;
                    return NULL;
                } else {
                    *eq = '\0';
                    configValue_t *value = MemAlloc(sizeof(configValue_t));
                    strcpy(value->Section, currentSection);
                    strcpy(value->Name, tok);
                    value->Value = strdup(eq + 1);
                    ADD_TO_LIST(value, config);
                }
            }
        } else {
            tok[strlen(tok) - 1] = '\0';
            strcpy(currentSection, &tok[1]);
        }
    }
    return config;
}

configValue_t *INI_ParseFile(LPCSTR fileName) {
    LPSTR buffer = FS_ReadFileIntoString(fileName);
    parser_t parser = {
        .tok = parser.token,
        .str = buffer,
        .error = false,
        .comma_space = false,
    };
    configValue_t *config = INI_ParseConfig(&parser);
    if (parser.error) {
        fprintf(stderr, "Failed to parse %s\n", fileName);
    }
    MemFree(buffer);
    return config;
}

LPCSTR INI_FindValue(configValue_t *ini, LPCSTR sectionName, LPCSTR valueName) {
    FOR_EACH_LIST(configValue_t, v, ini) {
        if (!strcmp(v->Section, sectionName) && !strcmp(v->Name, valueName))
            return v->Value;
    }
    return NULL;
}

