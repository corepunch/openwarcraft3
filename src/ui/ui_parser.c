#include "ui.h"
#include "../client/client.h"

#include <StormLib.h>

#define TOKEN_LEN 1024

typedef struct {
    LPSTR tok;
    LPCSTR str;
    bool error;
    bool comma_space;
    char token[TOKEN_LEN];
} parser_t;

extern configSection_t *skin;

LPCSTR FrameType[] = { "SIMPLEFRAME", NULL };
LPCSTR AlphaMode[] = { "ALPHAKEY", NULL };
LPCSTR Anchor[] = { "TOPLEFT", "TOPRIGHT", "BOTTOMLEFT", "BOTTOMRIGHT", NULL };

uiFrameDef_t *FDF_ParseFrame(parser_t *p);

void UI_ParseBuffer(parser_t *p);

bool ParserDone(parser_t *p) {
    return !*p->str || p->error;
}

bool ParserComment(parser_t *p, LPCSTR str) {
    return str[0] == '/' && str[1] == '/';
}

bool ParserSpace(parser_t *p, LPCSTR str) {
    return isspace(*str) || (p->comma_space && *str == ',') || *str == '"';
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

int ParseEnumString(LPCSTR token, LPCSTR const *values) {
    for (int i = 0; *values; i++, values++) {
        if (!strcmp(token, *values)) {
            return i;
        }
    }
    return -1;
}

int ParseEnum(parser_t *p, LPCSTR const *values) {
    LPCSTR token = ParserGetToken(p);
    int value = ParseEnumString(token, values);
    if (value == -1) {
        p->error = true;
    }
    return value;
}

uiTextureDef_t *FDF_ParseTexture(parser_t *p, uiFrameDef_t const *frameDef) {
    uiTextureDef_t *texDef = MemAlloc(sizeof(uiTextureDef_t));
    DWORD state = 0;
    for (LPCSTR tok = ParserGetToken(p);
         *tok != '{';
         tok = ParserGetToken(p))
    {
        if (state == 0) {
            strcpy(texDef->Name, ParserGetToken(p));
        } else {
            goto return_error;
        }
    }
    for (LPCSTR tok = ParserGetToken(p);
         !ParserDone(p);
         tok = ParserGetToken(p), state++)
    {
        if (!strcmp(tok, "File")) {
            LPCSTR File = ParserGetToken(p);
            if (!frameDef->DecorateFileNames || (File = INI_FindValue(skin, File))) {
                PATHSTR blp;
                sprintf(blp, "%s.blp", File);
                texDef->Texture = re.LoadTexture(blp);
            }
        } else if (!strcmp(tok, "Width")) {
            texDef->Width = atof(ParserGetToken(p));
        } else if (!strcmp(tok, "Height")) {
            texDef->Height = atof(ParserGetToken(p));
        } else if (!strcmp(tok, "TexCoord")) {// 0, 0.33984375, 0, 0.125,
            float const left = atof(ParserGetToken(p));
            float const right = atof(ParserGetToken(p));
            float const top = atof(ParserGetToken(p));
            float const bottom = atof(ParserGetToken(p));
            texDef->TexCoord.x = left;
            texDef->TexCoord.y = top;
            texDef->TexCoord.width = right - left;
            texDef->TexCoord.height = bottom - top;
        } else if (!strcmp(tok, "AlphaMode")) {// "ALPHAKEY",
            texDef->AlphaMode = ParseEnum(p, AlphaMode);
        } else if (!strcmp(tok, "Anchor")) {
            texDef->Anchor.corner = ParseEnum(p, Anchor);
            texDef->Anchor.x = atof(ParserGetToken(p));
            texDef->Anchor.y = atof(ParserGetToken(p));
        } else if (!strcmp(tok, "}")) {
            return texDef;
        } else {
            goto return_error;
        }
    }
return_error:
    p->error = true;
    MemFree(texDef);
    return NULL;
}

uiFrameDef_t *FDF_ParseFrame(parser_t *p) {
    uiFrameDef_t *frameDef = MemAlloc(sizeof(uiFrameDef_t));
    DWORD state = 0;
    for (LPCSTR tok = ParserGetToken(p);
         *tok != '{';
         tok = ParserGetToken(p), state++)
    {
        if (state == 0) {
            frameDef->Type = ParseEnumString(tok, FrameType);
        } else if (state == 1) {
            strcpy(frameDef->Name, tok);
        } else {
            goto return_error;
        }
    }
    for (LPCSTR tok = ParserGetToken(p);
         !ParserDone(p);
         tok = ParserGetToken(p))
    {
        if (!strcmp(tok, "DecorateFileNames")) {
            frameDef->DecorateFileNames = true;
        } else if (!strcmp(tok, "Texture")) {
            uiTextureDef_t *texture = FDF_ParseTexture(p, frameDef);
            if (texture) {
                ADD_TO_LIST(texture, frameDef->textures);
            } else {
                goto return_error;
            }
        } else if (!strcmp(tok, "}")) {
            return frameDef;
        }
    }
return_error:
    p->error = true;
    MemFree(frameDef);
    return NULL;
}

uiFrameDef_t *FDF_ParseScene(parser_t *p) {
    uiFrameDef_t *globals = NULL;
    for (LPCSTR tok = ParserGetToken(p);
         !ParserDone(p);
         tok = ParserGetToken(p))
    {
        if (!strcmp(tok, "Frame")) {
            uiFrameDef_t *frame = FDF_ParseFrame(p);
            if (frame) {
                ADD_TO_LIST(frame, globals);
            } else {
                // TODO: report error?
            }
        } else {
            p->error = true;
            return globals;
        }
    }
    return globals;
}

LPSTR ReadFileIntoString(LPCSTR fileName) {
    HANDLE fp = FS_OpenFile(fileName);
    DWORD const fileSize = SFileGetFileSize(fp, NULL);
    LPSTR buffer = MemAlloc(fileSize + 1);
    SFileReadFile(fp, buffer, fileSize, NULL, NULL);
    SFileCloseFile(fp);
    buffer[fileSize] = '\0';
    return buffer;
}

uiFrameDef_t *FDF_ParseFile(LPCSTR fileName) {
    LPSTR buffer = ReadFileIntoString(fileName);
    parser_t parser = {
        .tok = parser.token,
        .str = buffer,
        .error = false,
        .comma_space = true,
    };
    uiFrameDef_t *scene = FDF_ParseScene(&parser);
    if (parser.error) {
        fprintf(stderr, "Failed to parse %s\n", fileName);
    }
    MemFree(buffer);
    return scene;
}

configSection_t *INI_ParseConfig(parser_t *p) {
    configSection_t *config = NULL;
    configSection_t *current = NULL;
    for (LPSTR tok = ParserGetToken(p);
         !ParserDone(p);
         tok = ParserGetToken(p))
    {
        if (tok[0] != '[' || tok[strlen(tok) - 1] != ']') {
            if (!current) {
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
                    strcpy(value->Name, tok);
                    value->Value = strdup(eq + 1);
                    ADD_TO_LIST(value, current->values);
                }
            }
        } else {
            current = MemAlloc(sizeof(configSection_t));
            ADD_TO_LIST(current, config);
            tok[strlen(tok) - 1] = '\0';
            strcpy(current->Name, &tok[1]);
        }
    }
    return config;
}

configSection_t *INI_ParseFile(LPCSTR fileName) {
    LPSTR buffer = ReadFileIntoString(fileName);
    parser_t parser = {
        .tok = parser.token,
        .str = buffer,
        .error = false,
        .comma_space = false,
    };
    configSection_t *config = INI_ParseConfig(&parser);
    if (parser.error) {
        fprintf(stderr, "Failed to parse %s\n", fileName);
    }
    MemFree(buffer);
    return config;
}

configSection_t *INI_FindSection(configSection_t *ini, LPCSTR sectionName) {
    FOR_EACH_LIST(configSection_t, s, ini) {
        if (!strcmp(s->Name, sectionName))
            return s;
    }
    return NULL;
}

LPCSTR INI_FindValue(configSection_t *ini, LPCSTR valueName) {
    FOR_EACH_LIST(configValue_t, v, ini->values) {
        if (!strcmp(v->Name, valueName))
            return v->Value;
    }
    return NULL;
}
