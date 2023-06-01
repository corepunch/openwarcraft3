#include <stdlib.h>

#include "ui_local.h"

LPCSTR FrameType[] = {
    "",
    "BACKDROP",
    "BUTTON",
    "CHATDISPLAY",
    "CHECKBOX",
    "CONTROL",
    "DIALOG",
    "EDITBOX",
    "FRAME",
    "GLUEBUTTON",
    "GLUECHECKBOX",
    "GLUEEDITBOX",
    "GLUEPOPUPMENU",
    "GLUETEXTBUTTON",
    "HIGHLIGHT",
    "LISTBOX",
    "MENU",
    "MODEL",
    "POPUPMENU",
    "SCROLLBAR",
    "SIMPLEBUTTON",
    "SIMPLECHECKBOX",
    "SIMPLEFRAME",
    "SIMPLESTATUSBAR",
    "SLASHCHATBOX",
    "SLIDER",
    "SPRITE",
    "TEXT",
    "TEXTAREA",
    "TEXTBUTTON",
    "TIMERTEXT",
    NULL
};
LPCSTR AlphaMode[] = {
    "ALPHAKEY",
    NULL
};
LPCSTR Anchor[] = {
    "TOPLEFT",
    "TOPRIGHT",
    "BOTTOMLEFT",
    "BOTTOMRIGHT",
    NULL
};

uiFrameDef_t *FDF_ParseFrame(parser_t *p);

void UI_ParseBuffer(parser_t *p);

int ParseEnumString(LPCSTR token, LPCSTR const *values) {
    for (int i = 0; *values; i++, values++) {
        if (!strcmp(token, *values)) {
            return i;
        }
    }
    return -1;
}

int ParseEnum(parser_t *p, LPCSTR const *values) {
    LPCSTR token = imp.ParserGetToken(p);
    int value = ParseEnumString(token, values);
    if (value == -1) {
        imp.ParserError(p);
    }
    return value;
}

uiFrameDef_t *FDF_ParseTexture(parser_t *p, uiFrameDef_t const *parent) {
    uiFrameDef_t *frame = UI_MakeTextureFrame();
    uiTextureDef_t *texture = frame->typedata;
    DWORD state = 0;
    for (LPCSTR tok = imp.ParserGetToken(p);
         *tok != '{';
         tok = imp.ParserGetToken(p))
    {
        if (state == 0) {
            strcpy(frame->Name, imp.ParserGetToken(p));
        } else {
            goto return_error;
        }
    }
    for (LPCSTR tok = imp.ParserGetToken(p); tok; tok = imp.ParserGetToken(p), state++) {
        if (!strcmp(tok, "File")) {
            LPCSTR File = imp.ParserGetToken(p);
            if (!parent->DecorateFileNames ||
                (File = imp.FindConfigValue(ui.theme, "Default", File)))
            {
                PATHSTR blp;
                sprintf(blp, "%s.blp", File);
                texture->Texture = imp.LoadTexture(blp);
            }
        } else if (!strcmp(tok, "Width")) {
            frame->Width = atof(imp.ParserGetToken(p));
        } else if (!strcmp(tok, "Height")) {
            frame->Height = atof(imp.ParserGetToken(p));
        } else if (!strcmp(tok, "TexCoord")) {
            float const left = atof(imp.ParserGetToken(p));
            float const right = atof(imp.ParserGetToken(p));
            float const top = atof(imp.ParserGetToken(p));
            float const bottom = atof(imp.ParserGetToken(p));
            texture->TexCoord.x = left;
            texture->TexCoord.y = top;
            texture->TexCoord.width = right - left;
            texture->TexCoord.height = bottom - top;
        } else if (!strcmp(tok, "AlphaMode")) {// "ALPHAKEY",
            texture->AlphaMode = ParseEnum(p, AlphaMode);
        } else if (!strcmp(tok, "Anchor")) {
            texture->Anchor.corner = ParseEnum(p, Anchor);
            texture->Anchor.x = atof(imp.ParserGetToken(p));
            texture->Anchor.y = atof(imp.ParserGetToken(p));
        } else if (!strcmp(tok, "}")) {
            return frame;
        } else {
            goto return_error;
        }
    }
return_error:
    imp.ParserError(p);
    SAFE_DELETE(texture, imp.MemFree);
    SAFE_DELETE(frame, imp.MemFree);
    return NULL;
}

uiFrameDef_t *FDF_ParseFrame(parser_t *p) {
    uiFrameDef_t *frameDef = imp.MemAlloc(sizeof(uiFrameDef_t));
    DWORD state = 0;
    for (LPCSTR tok = imp.ParserGetToken(p);
         *tok != '{';
         tok = imp.ParserGetToken(p), state++)
    {
        if (state == 0) {
            frameDef->Type = ParseEnumString(tok, FrameType);
        } else if (state == 1) {
            strcpy(frameDef->Name, tok);
        } else {
            goto return_error;
        }
    }
    for (LPCSTR tok = imp.ParserGetToken(p); tok; tok = imp.ParserGetToken(p)) {
        if (!strcmp(tok, "DecorateFileNames")) {
            frameDef->DecorateFileNames = true;
        } else if (!strcmp(tok, "Texture")) {
            uiFrameDef_t *texture = FDF_ParseTexture(p, frameDef);
            if (texture) {
                ADD_TO_LIST(texture, frameDef->children);
            } else {
                goto return_error;
            }
        } else if (!strcmp(tok, "}")) {
            return frameDef;
        }
    }
return_error:
    imp.ParserError(p);
    imp.MemFree(frameDef);
    return NULL;
}

uiFrameDef_t *FDF_ParseScene(parser_t *p) {
    uiFrameDef_t *globals = NULL;
    for (LPCSTR tok = imp.ParserGetToken(p); tok; tok = imp.ParserGetToken(p)) {
        if (!strcmp(tok, "Frame")) {
            uiFrameDef_t *frame = FDF_ParseFrame(p);
            if (frame) {
                ADD_TO_LIST(frame, globals);
            } else {
                // TODO: report error?
            }
        } else {
            imp.ParserError(p);
            return globals;
        }
    }
    return globals;
}

uiFrameDef_t *FDF_ParseBuffer(LPCSTR buffer, LPCSTR fileName) {
    parser_t parser = { 0 };
    parser.tok = parser.token;
    parser.str = buffer;
    parser.comma_space = true;
    uiFrameDef_t *scene = FDF_ParseScene(&parser);
    if (parser.error) {
        fprintf(stderr, "Failed to parse %s\n", fileName);
    }
    return scene;
}

uiFrameDef_t *FDF_ParseFile(LPCSTR fileName) {
    LPSTR buffer = imp.ReadFileIntoString(fileName);
    if (buffer) {
        uiFrameDef_t *scene = FDF_ParseBuffer(buffer, fileName);
        imp.MemFree(buffer);
        return scene;
    } else {
        return NULL;
    }
}
