#include <stdlib.h>

#include "ui.h"

#include "../client/client.h"

extern configValue_t *war3skins;

LPCSTR FrameType[] = { "SIMPLEFRAME", NULL };
LPCSTR AlphaMode[] = { "ALPHAKEY", NULL };
LPCSTR Anchor[] = { "TOPLEFT", "TOPRIGHT", "BOTTOMLEFT", "BOTTOMRIGHT", NULL };

uiFrameDef_t *FDF_ParseFrame(parser_t *p);

void UI_ParseBuffer(parser_t *p);

void ParserError(parser_t *p);

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
        ParserError(p);
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
            if (!frameDef->DecorateFileNames || (File = INI_FindValue(war3skins, "Default", File))) {
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
    ParserError(p);
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
    ParserError(p);
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
            ParserError(p);
            return globals;
        }
    }
    return globals;
}

uiFrameDef_t *FDF_ParseFile(LPCSTR fileName) {
    LPSTR buffer = FS_ReadFileIntoString(fileName);
    if (!buffer) {
        return NULL;
    }
    parser_t parser = { 0 };
    parser.tok = parser.token;
    parser.str = buffer;
    parser.comma_space = true;
    uiFrameDef_t *scene = FDF_ParseScene(&parser);
    if (parser.error) {
        fprintf(stderr, "Failed to parse %s\n", fileName);
    }
    MemFree(buffer);
    return scene;
}
