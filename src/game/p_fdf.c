#include "g_local.h"

#include <stdlib.h> // atof()
#include <ctype.h> // isspace()

static uint32_t fnv1a32(const char* apStr) {
    uint32_t hash = 2166136261U; // 32 bit offset_basis = 2166136261U
    for (uint32_t idx = 0; apStr[idx] != 0; ++idx) {
        // 32 bit FNV_prime = 224 + 28 + 0x93 = 16777619
        hash = 16777619U * (hash ^ (unsigned char)(apStr[idx]));
    }

    return hash;
}

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
LPCSTR FontJustificationH[] = {
    "JUSTIFYCENTER",
    "JUSTIFYLEFT",
    "JUSTIFYRIGHT",
    NULL
};
LPCSTR FontJustificationV[] = {
    "JUSTIFYMIDDLE",
    "JUSTIFYTOP",
    "JUSTIFYBOTTOM",
    NULL
};
LPCSTR FramePointType[] = {
    "TOPLEFT",
    "TOP",
    "TOPRIGHT",
    "<UNUSED>",
    "LEFT",
    "CENTER",
    "RIGHT",
    "<UNUSED>",
    "BOTTOMLEFT",
    "BOTTOM",
    "BOTTOMRIGHT",
    "<UNUSED>",
    NULL
};

#define MAX_UI_CLASSES 256
#define F(x, type) { #x,((uint8_t *)&((uiFrameDef_t*)0)->x - (uint8_t *)NULL), Parse##type }
#define ITEM(x) { #x, x }
#define TARGET(TYPE) TYPE *target = (TYPE *)((uint8_t *)frame + arg->fofs);

void FDF_ParseFrame(parser_t *p, uiFrameDef_t *frame);

uiFrameDef_t frames[MAX_UI_CLASSES] = { 0 };
uiFrameDef_t templates[MAX_UI_CLASSES] = { 0 };

uiFrameDef_t *UI_Clear(void) {
    memset(frames, 0, sizeof(frames));
    memset(templates, 0, sizeof(templates));
    frames[0].inuse = true;
    frames[0].f.size.width = 8000;
    frames[0].f.size.height = 6000;
    frames[0].f.flags.type = FT_SCREEN;
    frames[0].Name = 0x12345678;
    return frames;
}

uiFrameDef_t *UI_Spawn(uiFrameType_t type, uiFrameDef_t *parent) {
    FOR_LOOP(i, MAX_UI_CLASSES) {
        uiFrameDef_t *frame = &frames[i];
        if (!frame->inuse) {
            memset(frame, 0, sizeof(uiFrameDef_t));
            frame->inuse = true;
            frame->Name = rand();
            frame->f.number = i;
            frame->f.flags.type = type;
            frame->parent = parent;
            return frame;
        }
    }
    return NULL;
}

uiFrameDef_t *UI_Template(uiFrameType_t type) {
    FOR_LOOP(i, MAX_UI_CLASSES) {
        uiFrameDef_t *template = &templates[i];
        if (!template->inuse) {
            memset(template, 0, sizeof(uiFrameDef_t));
            template->inuse = true;
            template->Name = rand();
            template->f.number = i;
            template->f.flags.type = type;
            return template;
        }
    }
    return NULL;
}

typedef struct {
    LPCSTR name;
    DWORD fofs;
    void (*func)(LPCSTR, uiFrameDef_t *frame, void *);
} parseArg_t;

typedef struct {
    LPCSTR name;
    parseArg_t args[16];
    void (*func)(parser_t *, uiFrameDef_t *);
} parseItem_t;

void parser_error(parser_t *parser) {
    parser->error = true;
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
    LPCSTR token = gi.ParserGetToken(p);
    int value = ParseEnumString(token, values);
    if (value == -1) {
        parser_error(p);
    }
    return value;
}

LPCSTR ApplySkin(uiFrameDef_t *frame, LPCSTR entry, bool force) {
    if (force || (frame->parent && frame->parent->DecorateFileNames)) {
        LPCSTR filename = gi.FindSheetCell(game.config.theme, "Default", entry);
        if (filename) {
            return filename;
        } else {
            fprintf(stderr, "Can't find %s in skin", entry);
            return entry;
        }
    } else {
        return entry;
    }
}

LPCSTR EnsureExtension(LPCSTR file, LPCSTR ext) {
    static PATHSTR blp;
    if (!strstr(file, ext)) {
        sprintf(blp, "%s%s", file, ext);
        return blp;
    } else {
        return file;
    }
}

#define MAKE_PARSER(TYPE) \
void Parse##TYPE(LPCSTR token, uiFrameDef_t *frame, void *out)

#define MAKE_PARSERCALL(TYPE) \
void TYPE(parser_t *parser, uiFrameDef_t *frame)

#define MAKE_ENUMPARSER(TYPE, FIELD) \
MAKE_PARSER(TYPE) { \
    frame->f.flags.FIELD = ParseEnumString(token, TYPE); \
}

MAKE_PARSER(Float) { *((float *)out) = atof(token); }
MAKE_PARSER(Float8) { *((uint8_t *)out) = atof(token) * 0xff; }
MAKE_PARSER(Float16) { *((int16_t *)out) = UI_SCALE(atof(token)); }
MAKE_PARSER(Integer) { *((int *)out) = atoi(token); }

MAKE_PARSER(Vector4) {
    ((VECTOR4 *)out)->x = atof(token);
    ((VECTOR4 *)out)->y = atof(token);
    ((VECTOR4 *)out)->z = atof(token);
    ((VECTOR4 *)out)->w = atof(token);
}

MAKE_PARSER(Hash) {
    *((uint32_t *)out) = fnv1a32(token);
}

MAKE_PARSER(Name) {
    memset(out, 0, sizeof(uiName_t));
    memcpy(out, token, strlen(token));
}

MAKE_ENUMPARSER(AlphaMode, alphaMode);
MAKE_ENUMPARSER(FontJustificationH, textalignx);
MAKE_ENUMPARSER(FontJustificationV, textaligny);

MAKE_PARSER(FramePointType) {
    *((uiFramePointType_t *)out) = ParseEnumString(token, FramePointType);
}

MAKE_PARSER(File) {
    LPCSTR file, withext;
    switch (frame->f.flags.type) {
        case FT_TEXTURE:
            file = ApplySkin(frame, token, false);
            withext = EnsureExtension(file, ".blp");
            frame->f.tex.index = gi.ImageIndex(withext);
            break;
        default:
            fprintf(stderr, "\"File\" not supported here\n");
            break;
    }
}

MAKE_PARSERCALL(Font) {
    LPCSTR file = ApplySkin(frame, frame->Font.Name, true);
    frame->f.font.index = gi.FontIndex(file, frame->Font.Size / 10);
}

MAKE_PARSERCALL(SetPoint) {
    DWORD x = frame->SetPoint.type & 3;
    DWORD y = (frame->SetPoint.type >> 2) & 3;
    uiFramePoint_t *xp = &frame->f.points.x[x];
    uiFramePoint_t *yp = &frame->f.points.y[y];
    xp->used = true;
    yp->used = true;
    xp->offset = frame->SetPoint.x;
    yp->offset = frame->SetPoint.y;
    xp->targetPos = frame->SetPoint.target & 3;
    yp->targetPos = (frame->SetPoint.target >> 2) & 3;
    frame->pointNames.x[x] = frame->SetPoint.relativeTo;
    frame->pointNames.y[y] = frame->SetPoint.relativeTo;
}

MAKE_PARSERCALL(Anchor) {
    frame->SetPoint.type = frame->Anchor.corner;
    frame->SetPoint.target = frame->Anchor.corner;
    if ((frame->SetPoint.type & 3) == FPP_MAX) {
        frame->SetPoint.x = -frame->Anchor.x;
    } else {
        frame->SetPoint.x = frame->Anchor.x;
    }
    if (((frame->SetPoint.target >> 2) & 3) == FPP_MAX) {
        frame->SetPoint.y = frame->Anchor.y;
    } else {
        frame->SetPoint.y = -frame->Anchor.y;
    }
    if (frame->parent) {
        frame->SetPoint.relativeTo = frame->parent->Name;
    } else {
        frame->SetPoint.relativeTo = 0;
    }
    SetPoint(parser, frame);
}

MAKE_PARSERCALL(DecorateFileNames) {
    frame->DecorateFileNames = true;
}

MAKE_PARSERCALL(BackdropTileBackground) {
    frame->Backdrop.TileBackground = true;
}

MAKE_PARSERCALL(BackdropBlendAll) {
    frame->Backdrop.BlendAll = true;
}

MAKE_PARSERCALL(Texture) {
    uiFrameDef_t *current = frame ? UI_Spawn(FT_TEXTURE, frame) : UI_Template(FT_TEXTURE);
    current->f.tex.coord[1] = 0xff;
    current->f.tex.coord[3] = 0xff;
    FDF_ParseFrame(parser, current);
}

MAKE_PARSERCALL(String) {
    uiFrameDef_t *current = frame ? UI_Spawn(FT_STRING, frame) : UI_Template(FT_STRING);
    FDF_ParseFrame(parser, current);
}

MAKE_PARSERCALL(Frame) {
    LPCSTR stype = gi.ParserGetToken(parser);
    uiFrameType_t type = ParseEnumString(stype, FrameType);
    uiFrameDef_t *current = frame ? UI_Spawn(type, frame) : UI_Template(type);
    FDF_ParseFrame(parser, current);
}

MAKE_PARSERCALL(IncludeFile) {
    LPCSTR filename = gi.ParserGetToken(parser);
}

#define F_END { NULL }

parseItem_t items[] = {
    // Types
    { "Frame", { F_END }, Frame },
    { "Texture", { F_END }, Texture },
    { "String", { F_END }, String },
    // Flags
    { "DecorateFileNames", { F_END }, DecorateFileNames },
    // Fields
    { "Width", { F(f.size.width, Float16), F_END } },
    { "Height", { F(f.size.height, Float16), F_END } },
    { "File", { F(f.tex.index, File), F_END } },
    { "TexCoord", { F(f.tex.coord[0], Float8), F(f.tex.coord[1], Float8), F(f.tex.coord[2], Float8), F(f.tex.coord[3], Float8), F_END } },
    { "AlphaMode", { F(f.flags, AlphaMode), F_END } },
    { "Anchor", { F(Anchor.corner, FramePointType), F(Anchor.x, Float16), F(Anchor.y, Float16), F_END }, Anchor },
    { "Font", { F(Font.Name, Name), F(Font.Size, Float16), F_END }, Font },
    { "Text", { F(f.text, Name), F_END } },
    { "ButtonText", { F(f.text, Name), F_END } },
    { "TextLength", { F(f.textLength, Integer), F_END } },
    { "FontJustificationH", { F(f.flags, FontJustificationH), F_END } },
    { "FontJustificationV", { F(f.flags, FontJustificationV), F_END } },
    { "SetPoint", { F(SetPoint.type, FramePointType), F(SetPoint.relativeTo, Hash), F(SetPoint.target, FramePointType), F(SetPoint.x, Float16), F(SetPoint.y, Float16), F_END }, SetPoint },
    { "DialogBackdrop", { F(DialogBackdrop, Hash), F_END } },
    { "BackdropTileBackground", F_END, BackdropTileBackground },
    { "BackdropBackground", { F(Backdrop.Background, Name), F_END } },
    { "BackdropCornerFlags", { F(Backdrop.CornerFlags, Name), F_END } },
    { "BackdropCornerSize", { F(Backdrop.CornerSize, Float), F_END } },
    { "BackdropBackgroundSize", { F(Backdrop.BackgroundSize, Float), F_END } },
    { "BackdropBackgroundInsets", { F(Backdrop.BackgroundInsets, Vector4), F_END } },
    { "BackdropEdgeFile", { F(Backdrop.EdgeFile, Name), NULL } },
    { "BackdropBlendAll", F_END, BackdropBlendAll },
    // End of list
    F_END
};

LPCSTR ReadUntil(LPCSTR str, LPCSTR sub) {
    static uiName_t token;
    memset(token, 0, sizeof(uiName_t));
    LPCSTR pos = strstr(str, sub);
    memcpy(token, str, pos - str);
    if (*token == '"') {
        for (LPSTR s = token; *s; s++) {
            if (*(s+1) == '"') {
                *s = 0;
                break;;
            }
            *s = *(s+1);
        }
    }
    return token;
}

LPCSTR ParseUntil(parser_t *p, LPCSTR sub) {
    while (isspace(*p->str)) p->str++;
    LPCSTR token = ReadUntil(p->str, sub);
    if (*p->str == '"') {
        p->str += strlen(token) + 3;
    } else {
        p->str += strlen(token) + 1;
    }
    while (isspace(*p->str)) p->str++;
    return token;
}

void parse_item(parser_t *parser, uiFrameDef_t *frame, parseItem_t *item) {
    for (parseArg_t *arg = item->args; arg->name; arg++) {
        LPCSTR token = ParseUntil(parser, ",");
        arg->func(token, frame, (uint8_t *)frame + arg->fofs);
    }
    if (item->func) {
        item->func(parser, frame);
    }
}

void parse_func(parser_t *parser, uiFrameDef_t *frame) {
    LPCSTR token = NULL;
    while ((token = gi.ParserGetToken(parser)) && (*token != '}')) {
        for (parseItem_t *it = items; it->name; it++) {
            if (!strcmp(it->name, token)) {
                parse_item(parser, frame, it);
                goto parse_next;
            }
        }
        fprintf(stderr, "Can't recognize token '%s'\n", token);
        parser->error = true;
        return;
    parse_next:;
    }
}

uiFrameDef_t *FindFrameTemplate(LPCSTR str) {
    uint32_t hash = fnv1a32(str);
    for (uiFrameDef_t *tmp = templates;
         (tmp - templates) < MAX_UI_CLASSES;
         tmp++)
    {
        if (tmp->Name == hash)
            return tmp;
    }
    return NULL;
}

void UI_InheritFrom(uiFrameDef_t *frame, LPCSTR inheritName) {
    uiFrameDef_t *inherit = FindFrameTemplate(inheritName);
    if (inherit && inherit->f.flags.type == frame->f.flags.type) {
        uiFrameDef_t tmp;
        memcpy(&tmp, frame, sizeof(uiFrameDef_t));
        memcpy(frame, inherit, sizeof(uiFrameDef_t));
        frame->Name = tmp.Name;
        frame->parent = tmp.parent;
        frame->f.number = tmp.f.number;
    } else if (inherit) {
        fprintf(stderr, "Can't inherit from different type %s\n", inheritName);
    } else {
        fprintf(stderr, "Can't find template %s\n", inheritName);
    }
}

void FDF_ParseFrame(parser_t *p, uiFrameDef_t *frame) {
    DWORD state = 0;
    LPCSTR tok;
    while ((tok = gi.ParserGetToken(p)) && (*tok != '{')) {
        if (!strcmp(tok, "INHERITS")) {
            LPCSTR inheritName = gi.ParserGetToken(p);
            bool withChildren = false;
            if (!strcmp(inheritName, "WITHCHILDREN")) {
                withChildren = true;
                inheritName = gi.ParserGetToken(p);
            }
            UI_InheritFrom(frame, inheritName);
            state++;
        } else if (state == 0) {
            frame->Name = fnv1a32(tok);
            state++;
        } else {
            parser_error(p);
            return;
        }
    }
    parse_func(p, frame);
}

uiFrameDef_t *UI_FindFrame(uiFrameDef_t *frames, uint32_t hash) {
    for (uiFrameDef_t *f = frames; f->inuse; f++) {
        if (f->Name == hash) {
            return f;
        }
    }
    return NULL;
}

DWORD UI_FindFrameNumber(uiFrameDef_t *frames, uint32_t hash) {
    uiFrameDef_t *f = UI_FindFrame(frames, hash);
    return f ? f->f.number : 0;
}

uiFrameDef_t *FDF_ParseScene(uiFrameDef_t const *root, parser_t *p) {
    LPCSTR tok = NULL;
    uiFrameDef_t *frame = NULL;
    while ((tok = gi.ParserGetToken(p))) {
        if (!strcmp(tok, "Frame")) {
            frame = UI_Spawn(FT_SIMPLEFRAME, NULL);
            frame->inuse = false; // to reuse it in Frame()
            Frame(p, root);
        } else if (!strcmp(tok, "Texture")) {
            Texture(p, NULL);
        } else if (!strcmp(tok, "String")) {
            String(p, NULL);
        } else if (!strcmp(tok, "IncludeFile")) {
            IncludeFile(p, NULL);
        } else {
            parser_error(p);
            return NULL;
        }
    }
    for (uiFrameDef_t *f = frames; f->inuse; f++) {
        FOR_LOOP(p, FPP_COUNT) {
            f->f.points.x[p].relativeTo = UI_FindFrameNumber(frames, f->pointNames.x[p]);
            f->f.points.y[p].relativeTo = UI_FindFrameNumber(frames, f->pointNames.y[p]);
        }
        f->f.parent = f->parent ? f->parent->f.number : 0;
    }
    return frame;
}

uiFrameDef_t *FDF_ParseFile(uiFrameDef_t const *root, LPCSTR fileName) {
    LPSTR buffer = gi.ReadFileIntoString(fileName);
    if (!buffer)
        return NULL;
    parser_t parser = { 0 };
    parser.tok = parser.token;
    parser.str = buffer;
    parser.comma_space = true;
    uiFrameDef_t *frame = FDF_ParseScene(root, &parser);
    if (parser.error) {
        fprintf(stderr, "Failed to parse %s\n", fileName);
    }
    gi.MemFree(buffer);
    return frame;
}

void UI_WriteLayout(edict_t *ent,
                    uiFrameDef_t const *frames,
                    DWORD layer)
{
    gi.WriteByte(svc_layout);
    gi.WriteByte(layer);
    for (uiFrameDef_t const *f = frames; f->inuse; f++) {
        gi.WriteUIFrame(&f->f);
    }
    gi.WriteLong(0); // end of list
    gi.unicast(ent);
}

void UI_SetPoint(uiFrameDef_t *frame,
                 uiFramePointType_t framePoint,
                 uiFrameDef_t *other,
                 uiFramePointType_t otherPoint,
                 int16_t x,
                 int16_t y)
{
    frame->SetPoint.type = framePoint;
    frame->SetPoint.relativeTo = other->f.number;
    frame->SetPoint.target = otherPoint;
    frame->SetPoint.x = x;
    frame->SetPoint.y = y;
    SetPoint(NULL, frame);
}

uiFrameDef_t *UI_FindFrameByName(uiFrameDef_t *frame, LPCSTR name) {
    DWORD ident = fnv1a32(name);
    for (uiFrameDef_t *f = frames; f->inuse; f++) {
        if (f->Name == ident) {
            return f;
        }
    }
    return NULL;
}
