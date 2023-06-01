#include <StormLib.h>

#include "common.h"

bool ParserDone(parser_t *p) {
    return !*p->str || p->error;
}

bool ParserComment(parser_t *p, LPCSTR str) {
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
    if (!fp) {
        return NULL;
    }
    DWORD const fileSize = SFileGetFileSize(fp, NULL);
    LPSTR buffer = MemAlloc(fileSize + 1);
    SFileReadFile(fp, buffer, fileSize, NULL, NULL);
    FS_CloseFile(fp);
    buffer[fileSize] = '\0';
    return buffer;
}

LPSTR ParserGetToken(parser_t *p) {
    for (; !ParserDone(p); p->str++) {
        if (ParserComment(p, p->str)) {
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

#define MAX_INI_LINE 1024

configValue_t *INI_ParseConfig(LPCSTR buffer) {
    configValue_t *config = NULL;
    char currentSec[64] = { 0 };
    LPCSTR p = buffer;
    
    while (true) {
        while (*p && isspace(*p)) p++;
        if (!*p)
            return config;
        if (p[0] == '/' && p[1] == '/') {
            for (; *p != '\n' && *p != '\0'; p++);
        } else if (*p == '[') {
            p++;
            memset(currentSec, 0, sizeof(currentSec));
            for (LPSTR w = currentSec; *p != ']'; w++, p++) {
                *w = *p;
            }
            p++;
        } else {
            char line[MAX_INI_LINE] = { 0 };
            for (LPSTR w = line; *p != '\n' && *p != '\r' && *p != '\0'; p++, w++) {
                *w = *p;
            }
            LPSTR eq = strstr(line, "=");
            if (eq) {
                *eq = '\0';
                for (LPSTR s = eq; s > line; s--) {
                    if (*s == ' ') *s = '\0';
                    else break;
                }
                eq++;
                for (; *eq == ' '; eq++);
//                printf("%s.%s %s\n", currentSec, line, eq);
                configValue_t *value = MemAlloc(sizeof(configValue_t));
                strcpy(value->Section, currentSec);
                strcpy(value->Name, line);
                value->Value = strdup(eq);
                ADD_TO_LIST(value, config);
            }
        }
    }
    return config;
}

configValue_t *FS_ParseConfig(LPCSTR fileName) {
//    printf("    %s\n", fileName);
    LPSTR buffer = FS_ReadFileIntoString(fileName);
    if (!buffer) {
        return NULL;
    }
    configValue_t *config = INI_ParseConfig(buffer);
    if (!config) {
        fprintf(stderr, "Failed to parse %s\n", fileName);
    }
    MemFree(buffer);
    return config;
}

LPCSTR INI_FindValue(configValue_t *ini, LPCSTR sectionName, LPCSTR valueName) {
    FOR_EACH_LIST(configValue_t, v, ini) {
        if (!strcasecmp(v->Section, sectionName) && !strcasecmp(v->Name, valueName))
            return v->Value;
    }
    return NULL;
}

