#include "g_local.h"
#include "parser.h"

#include <stdlib.h> // atof()
#include <ctype.h> // isspace()

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
    "TEXTURE",
    "STRING",
    "LAYER",
    "SCREEN",
    "COMMANDBUTTON",
    "PORTRAIT",
    "STRINGLIST",
    "BUILDQUEUE",
    "MULTISELECT",
    "TOOLTIPTEXT",
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
LPCSTR FontFlags[] = {
    "FIXEDSIZE",
    "PASSWORDFIELD",
    NULL
};

#define MAX_UI_CLASSES 256
#define F(x, type) { #x,((uint8_t *)&((FRAMEDEF *)0)->x - (uint8_t *)NULL), Parse##type }
#define ITEM(x) { #x, x }
#define TARGET(TYPE) TYPE *target = (TYPE *)((uint8_t *)frame + arg->fofs);

FRAMEDEF frames[MAX_UI_CLASSES] = { 0 };

void FDF_ParseFrame(WordExtractor *p, LPFRAMEDEF frame);

void UI_PrintClasses(void) {
    FOR_LOOP(i, MAX_UI_CLASSES) {
        printf("%d  %s\n", frames[i].f.number, frames[i].Name);
    }
}

void UI_ClearTemplates(void) {
    memset(frames, 0, sizeof(frames));
}

void UI_InitFrame(LPFRAMEDEF frame, DWORD number, uiFrameType_t type) {
    memset(frame, 0, sizeof(FRAMEDEF));
    frame->inuse = true;
    frame->f.number = number;
    frame->f.flags.type = type;
    frame->f.color = COLOR32_WHITE;
    frame->f.text = frame->Text;
    if (type == FT_TEXTURE ||
        type == FT_SIMPLESTATUSBAR ||
        type == FT_COMMANDBUTTON ||
        type == FT_BACKDROP)
    {
        frame->f.tex.coord[1] = 0xff;
        frame->f.tex.coord[3] = 0xff;
    }
}

LPFRAMEDEF UI_Spawn(uiFrameType_t type, LPFRAMEDEF parent) {
    FOR_LOOP(i, MAX_UI_CLASSES) {
        if (i==0) continue;
        LPFRAMEDEF frame = &frames[i];
        if (!frame->inuse) {
            UI_InitFrame(frame, i, type);
            frame->f.parent = parent ? parent->f.number : 0;
            return frame;
        }
    }
    return NULL;
}

typedef struct {
    LPCSTR name;
    DWORD fofs;
    void (*func)(LPCSTR, LPFRAMEDEF frame, void *);
} parseArg_t;

typedef struct {
    LPCSTR name;
    parseArg_t args[16];
    void (*func)(WordExtractor *, LPFRAMEDEF);
} parseItem_t;

typedef struct {
    LPCSTR name;
    void (*func)(WordExtractor *, LPFRAMEDEF);
} parseClass_t;

void parser_error(WordExtractor *parser) {
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

int ParseEnum(WordExtractor *p, LPCSTR const *values) {
    LPCSTR token = getFirstWord(p);
    int value = ParseEnumString(token, values);
    if (value == -1) {
        parser_error(p);
    }
    return value;
}

LPCSTR UI_ApplySkin(LPCSTR entry, BOOL force) {
    LPCSTR filename = gi.FindSheetCell(game.config.theme, "Default", entry);
    if (filename) {
        return filename;
    } else {
//        fprintf(stderr, "Can't find %s in skin\n", entry);
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

DWORD UI_LoadTexture(LPCSTR file, BOOL decorate) {
    file = UI_ApplySkin(file, decorate);
    file = EnsureExtension(file, ".blp");
    return gi.ImageIndex(file);
}

#define MAKE_PARSER(TYPE) \
void Parse##TYPE(LPCSTR token, LPFRAMEDEF frame, void *out)

#define MAKE_PARSERCALL(TYPE) \
void TYPE(WordExtractor *parser, LPFRAMEDEF frame)

#define MAKE_ENUMPARSER(TYPE, FIELD) \
MAKE_PARSER(TYPE) { \
    frame->f.flags.FIELD = ParseEnumString(token, TYPE); \
}

static SHORT Float2Short(float f) {
    return ceilf(UI_SCALE(f));
}

MAKE_PARSER(Float) { *((FLOAT *)out) = atof(token); }
MAKE_PARSER(Float8) { *((BYTE *)out) = atof(token) * 0xff; }
MAKE_PARSER(Float16) { *((SHORT *)out) = Float2Short(atof(token)); }
MAKE_PARSER(Integer) { *((LONG *)out) = atoi(token); }
MAKE_PARSER(Vector2) { FLOAT *v = out; sscanf(token, "%f %f", v+0, v+1); }
MAKE_PARSER(Vector3) { FLOAT *v = out; sscanf(token, "%f %f %f", v+0, v+1, v+2); }
MAKE_PARSER(Vector4) { FLOAT *v = out; sscanf(token, "%f %f %f %f", v+0, v+1, v+2, v+3); }
MAKE_PARSER(Float16_4) {
    VECTOR4 vec4;
    ParseVector4(token, frame, &vec4);
    SHORT *v = out;
    v[0] = Float2Short(vec4.x);
    v[1] = Float2Short(vec4.y);
    v[2] = Float2Short(vec4.z);
    v[3] = Float2Short(vec4.w);
}
MAKE_PARSER(Color) {
    VECTOR4 vec4;
    ParseVector4(token, frame, &vec4);
    ((COLOR32 *)out)->r = vec4.x * 0xff;
    ((COLOR32 *)out)->g = vec4.y * 0xff;
    ((COLOR32 *)out)->b = vec4.z * 0xff;
    ((COLOR32 *)out)->a = vec4.w * 0xff;
}

MAKE_PARSER(FrameNumber) {
    *((DWORD *)out) = 0xffff;
    FOR_LOOP(i, MAX_UI_CLASSES) {
        int dist1 = abs((int)i - (int)frame->f.number);
        int dist2 = abs(*((int *)out) - (int)frame->f.number);
        if (dist1 > dist2)
            return;
        if ( !strcmp(frames[i].Name, token)) {
            *((DWORD *)out) = i;
        }
    }
}

MAKE_PARSER(Name) {
    memset(out, 0, sizeof(UINAME));
    memcpy(out, token, strlen(token));
}

MAKE_PARSER(Text) {
    LPCSTR str = UI_GetString(token);
    frame->f.text = frame->Text;
    memset(out, 0, sizeof(UINAME));
    memcpy(out, str, strlen(str));
}

typedef struct stringListItem_s {
    struct stringListItem_s *next;
    UINAME name;
    LPSTR value;
} stringListItem_t;

stringListItem_t *strings = NULL;

MAKE_PARSER(StringListItem) {
    strings->value = strdup(token);
}

MAKE_ENUMPARSER(AlphaMode, alphaMode);
MAKE_ENUMPARSER(FontJustificationH, textalignx);
MAKE_ENUMPARSER(FontJustificationV, textaligny);

MAKE_PARSER(FontFlags) {
    frame->Font.FontFlags = ParseEnumString(token, FontFlags);
}

MAKE_PARSER(FramePointType) {
    *((UIFRAMEPOINT *)out) = ParseEnumString(token, FramePointType);
}

MAKE_PARSER(File) {
    BOOL decorate = frame->DecorateFileNames | frames[frame->f.parent].DecorateFileNames;
    *((DWORD *)out) = UI_LoadTexture(token, decorate);
}

MAKE_PARSERCALL(Font) {
    LPCSTR file = UI_ApplySkin(frame->Font.Name, true);
    frame->f.font.index = gi.FontIndex(file, frame->Font.Size / 10);
}

MAKE_PARSERCALL(SetPoint) {
    if (!frame->AnyPointsSet) { // clear any template points
        memset(&frame->f.points, 0, sizeof(frame->f.points));
        frame->AnyPointsSet = true;
    }
    DWORD const x = frame->SetPoint.type & 3;
    uiFramePoint_t *xp = frame->f.points.x;
    if (x != FPP_MID || (!xp[FPP_MIN].used && !xp[FPP_MAX].used)) {
        xp[FPP_MID].used = false;
        xp[x].used = true;
        xp[x].offset = frame->SetPoint.x;
        xp[x].targetPos = frame->SetPoint.target & 3;
        xp[x].relativeTo = frame->SetPoint.relativeTo;
    }
    DWORD y = (frame->SetPoint.type >> 2) & 3;
    uiFramePoint_t *yp = frame->f.points.y;
    if (y != FPP_MID || (!yp[FPP_MIN].used && !yp[FPP_MAX].used)) {
        yp[FPP_MID].used = false;
        yp[y].used = true;
        yp[y].offset = frame->SetPoint.y;
        yp[y].targetPos = (frame->SetPoint.target >> 2) & 3;
        yp[y].relativeTo = frame->SetPoint.relativeTo;
    }
}

MAKE_PARSERCALL(Anchor) {
    frame->SetPoint.type = frame->Anchor.corner;
    frame->SetPoint.target = frame->Anchor.corner;
    frame->SetPoint.relativeTo = UI_PARENT;
    frame->SetPoint.x = frame->Anchor.x;
    frame->SetPoint.y = frame->Anchor.y;
    SetPoint(parser, frame);
}

MAKE_PARSERCALL(DecorateFileNames) {
    frame->DecorateFileNames = true;
}

MAKE_PARSERCALL(BackdropTileBackground) {
    frame->Backdrop.TileBackground = true;
}

MAKE_PARSERCALL(UseActiveContext) {
//    frame->Backdrop.TileBackground = true;
}

MAKE_PARSERCALL(BackdropBlendAll) {
    frame->Backdrop.BlendAll = true;
}

MAKE_PARSERCALL(SetAllPoints) {
    UI_SetAllPoints(frame);
}

MAKE_PARSERCALL(Texture) {
    FDF_ParseFrame(parser, UI_Spawn(FT_TEXTURE, frame));
}

MAKE_PARSERCALL(Layer) {
    FDF_ParseFrame(parser, UI_Spawn(FT_LAYER, frame));
}

MAKE_PARSERCALL(String) {
    FDF_ParseFrame(parser, UI_Spawn(FT_STRING, frame));
}

MAKE_PARSERCALL(Frame) {
    LPCSTR stype = getFirstWord(parser);
    uiFrameType_t type = ParseEnumString(stype, FrameType);
    LPFRAMEDEF current = UI_Spawn(type, frame);
    FDF_ParseFrame(parser, current);
}

MAKE_PARSERCALL(IncludeFile) {
    LPCSTR filename = getFirstWord(parser);
    UI_ParseFDF(filename);
}

MAKE_PARSERCALL(StringList) {
    FRAMEDEF string_list;
    memset(&string_list, 0, sizeof(FRAMEDEF));
    string_list.f.flags.type = FT_STRINGLIST;
    FDF_ParseFrame(parser, &string_list);
}

#define F_END { NULL }

parseClass_t classes[] = {
    { "Frame", Frame },
    { "Texture", Texture },
    { "String", String },
    { "Layer", Layer },
    { "StringList", StringList },
    { "IncludeFile", IncludeFile },
    F_END,
};

parseItem_t items[] = {
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
    { "Text", { F(Text, Text), F_END } },
    { "ButtonText", { F(Text, Text), F_END } },
    { "TextLength", { F(f.textLength, Integer), F_END } },
    { "FrameFont", { F(Font.Name, Name), F(Font.Size, Float16), F(Font.Unknown, Name), F_END }, Font },
    { "FontJustificationH", { F(f.flags, FontJustificationH), F_END } },
    { "FontJustificationV", { F(f.flags, FontJustificationV), F_END } },
    { "FontFlags", { F(Font.FontFlags, FontFlags), F_END } },
    { "FontColor", { F(Font.Color, Color), F_END } },
    { "FontHighlightColor", { F(Font.HighlightColor, Color), F_END } },
    { "FontDisabledColor", { F(Font.DisabledColor, Color), F_END } },
    { "FontShadowColor", { F(Font.ShadowColor, Color), F_END } },
    { "FontShadowOffset", { F(Font.ShadowOffset, Vector2), F_END } },
    { "SetPoint", { F(SetPoint.type, FramePointType), F(SetPoint.relativeTo, FrameNumber), F(SetPoint.target, FramePointType), F(SetPoint.x, Float16), F(SetPoint.y, Float16), F_END }, SetPoint },
    { "UseActiveContext", F_END, UseActiveContext },
    { "DialogBackdrop", { F(DialogBackdrop, FrameNumber), F_END } },
    { "BackdropTileBackground", F_END, BackdropTileBackground },
    { "BackdropBackground", { F(Backdrop.Background, File), F_END } },
    { "BackdropCornerFlags", { F(Backdrop.CornerFlags, Name), F_END } },
    { "BackdropCornerSize", { F(Backdrop.CornerSize, Float16), F_END } },
    { "BackdropBackgroundSize", { F(Backdrop.BackgroundSize, Float16), F_END } },
    { "BackdropBackgroundInsets", { F(Backdrop.BackgroundInsets, Float16_4), F_END } },
    { "BackdropEdgeFile", { F(Backdrop.EdgeFile, File), NULL } },
    { "BackdropBlendAll", F_END, BackdropBlendAll },
    { "SetAllPoints", F_END, SetAllPoints },
    // End of list
    F_END
};

void parse_item(WordExtractor *parser, LPFRAMEDEF frame, parseItem_t *item) {
    for (parseArg_t *arg = item->args; arg->name; arg++) {
        LPCSTR token = getNextSegment(parser);
        arg->func(token, frame, (uint8_t *)frame + arg->fofs);
    }
    if (!item->args->name) { // eat trailing comma
        getNextSegment(parser);
    }
    if (item->func) {
        item->func(parser, frame);
    }
}

void parse_func(WordExtractor *parser, LPFRAMEDEF frame) {
    LPCSTR token = NULL;
    while ((token = getFirstWord(parser)) && (*token != '}')) {
        if (frame->f.flags.type == FT_STRINGLIST) {
            static parseItem_t stringitem = { "", { F(f, StringListItem), F_END } };
            stringListItem_t *str = gi.MemAlloc(sizeof(stringListItem_t));
            ADD_TO_LIST(str, strings);
            strcpy(str->name, token);
            parse_item(parser, frame, &stringitem);
            goto parse_next;
        } else {
            for (parseItem_t *it = items; it->name; it++) {
                if (!strcmp(it->name, token)) {
                    parse_item(parser, frame, it);
                    goto parse_next;
                }
            }
            for (parseClass_t *it = classes; it->name; it++) {
                if (!strcmp(it->name, token)) {
                    it->func(parser, frame);
                    goto parse_next;
                }
            }
        }
        fprintf(stderr, "Can't recognize token '%s'\n", token);
        parser->error = true;
        return;
    parse_next:;
    }
}

LPFRAMEDEF FindFrameTemplate(LPCSTR str) {
    FOR_LOOP(i, MAX_UI_CLASSES) {
        LPFRAMEDEF tmp = frames+i;
        if (!strcmp(tmp->Name, str))
            return tmp;
    }
    return NULL;
}

void UI_InheritFrom(LPFRAMEDEF frame, LPCSTR inheritName) {
    LPFRAMEDEF inherit = FindFrameTemplate(inheritName);
    if (inherit && inherit->f.flags.type == frame->f.flags.type) {
        FRAMEDEF tmp;
        memcpy(&tmp, frame, sizeof(FRAMEDEF));
        memcpy(frame, inherit, sizeof(FRAMEDEF));
        memcpy(frame->Name, tmp.Name, sizeof(UINAME));
        frame->f.parent = tmp.f.parent;
        frame->f.number = tmp.f.number;
        frame->AnyPointsSet = false;
    } else if (inherit) {
        fprintf(stderr, "Can't inherit from different type %s\n", inheritName);
    } else {
        fprintf(stderr, "Can't find template %s\n", inheritName);
    }
}

void FDF_ParseFrame(WordExtractor *p, LPFRAMEDEF frame) {
    DWORD state = 0;
    LPCSTR tok;
    while ((tok = getFirstWord(p)) && (*tok != '{')) {
        if (!strcmp(tok, "INHERITS")) {
            LPCSTR inheritName = getFirstWord(p);
            BOOL withChildren = false;
            if (!strcmp(inheritName, "WITHCHILDREN")) {
                withChildren = true;
                inheritName = getFirstWord(p);
            }
            UI_InheritFrom(frame, inheritName);
            state++;
        } else if (state == 0) {
            strncpy(frame->Name, tok, sizeof(UINAME));
            state++;
        } else {
            parser_error(p);
            return;
        }
    }
    parse_func(p, frame);
}

LPFRAMEDEF UI_FindFrame(LPCSTR name) {
    FOR_LOOP(i, MAX_UI_CLASSES) {
        if (!strcmp(frames[i].Name, name)) {
            return frames+i;
        }
    }
    return NULL;
}

LPFRAMEDEF UI_FindChildFrame(LPFRAMEDEF frame, LPCSTR name) {
    if (!strcmp(frame->Name, name))
        return frame;
    FOR_LOOP(i, MAX_UI_CLASSES) {
        if (frames[i].f.parent != frame->f.number)
            continue;
        LPFRAMEDEF found = UI_FindChildFrame(frames+i, name);
        if (found)
            return found;
    }
    return NULL;
}

void FDF_ParseScene(WordExtractor *parser) {
    LPCSTR token = NULL;
    LPFRAMEDEF frame = NULL;
    while (*(token = getFirstWord(parser))) {
        for (parseClass_t *it = classes; it->name; it++) {
            if (!strcmp(it->name, token)) {
                it->func(parser, frame);
                goto parse_next;
            }
        }
        parser_error(parser);
        return;
    parse_next:;
    }
}

void UI_ParseFDF_Buffer(LPCSTR fileName, LPSTR buffer2) {
    LPSTR buffer = buffer2;
    removeComments(buffer);
    removeBOM(buffer, strlen(buffer));
    WordExtractor parser = {
        .buffer = buffer,
        .delimiters = ",;{}",
    };
    FDF_ParseScene(&parser);
    if (parser.error) {
        fprintf(stderr, "Failed to parse %s\n", fileName);
    }
}

void UI_ParseFDF(LPCSTR fileName) {
    LPSTR buffer = gi.ReadFileIntoString(fileName);
    if (buffer) {
        UI_ParseFDF_Buffer(fileName, buffer);
        gi.MemFree(buffer);
    }
}

#define SPRINTF_ADD(TEXT, ...) snprintf(TEXT+strlen(TEXT), sizeof(TEXT), __VA_ARGS__);

void UI_WriteFrame(LPCFRAMEDEF frame) {
    UINAME backdrop;
    uiFrame_t tmp;
    memcpy(&tmp, &frame->f, sizeof(tmp));
    if (tmp.flags.type == FT_BACKDROP || tmp.flags.type == FT_TOOLTIPTEXT) {
//      BackdropTileBackground,
//      BackdropBackground  "ToolTipBackground",
//      BackdropCornerFlags "UL|UR|BL|BR|T|L|B|R",
//      BackdropCornerSize  0.008,
//      BackdropBackgroundSize  0.036,
//      BackdropBackgroundInsets 0.0025 0.0025 0.0025 0.0025,
//      BackdropEdgeFile  "ToolTipBorder",
//      BackdropBlendAll,
        memset(&backdrop, 0, sizeof(backdrop));
        SPRINTF_ADD(backdrop, "%s,", frame->Backdrop.CornerFlags);
        SPRINTF_ADD(backdrop, "%d,", frame->Backdrop.TileBackground);
        SPRINTF_ADD(backdrop, "%d,", frame->Backdrop.Background);
        SPRINTF_ADD(backdrop, "%d,", frame->Backdrop.CornerSize);
        SPRINTF_ADD(backdrop, "%d,", frame->Backdrop.BackgroundSize);
        SPRINTF_ADD(backdrop, "%d ", frame->Backdrop.BackgroundInsets[0]);
        SPRINTF_ADD(backdrop, "%d ", frame->Backdrop.BackgroundInsets[1]);
        SPRINTF_ADD(backdrop, "%d ", frame->Backdrop.BackgroundInsets[2]);
        SPRINTF_ADD(backdrop, "%d,", frame->Backdrop.BackgroundInsets[3]);
        SPRINTF_ADD(backdrop, "%d,", frame->Backdrop.EdgeFile);
        SPRINTF_ADD(backdrop, "%d,", frame->Backdrop.BlendAll);
        tmp.text = backdrop;
    }
    gi.WriteUIFrame(&tmp);
}

void UI_WriteFrameWithChildren(LPCFRAMEDEF frame) {
    UI_WriteFrame(frame);
    FOR_LOOP(i, MAX_UI_CLASSES) {
        LPCFRAMEDEF it = frames+i;
        if (it->f.parent == frame->f.number && it->f.number > 0 && !it->hidden) {
            UI_WriteFrameWithChildren(it);
        }
    }
}

void UI_WriteLayout(LPEDICT ent,
                    LPCFRAMEDEF root,
                    DWORD layer)
{
    gi.WriteByte(svc_layout);
    gi.WriteByte(layer);
    UI_WriteFrameWithChildren(root);
    gi.WriteLong(0); // end of list
    gi.unicast(ent);
}

void UI_WriteLayout2(LPEDICT ent,
                     void (*BuildUI)(LPGAMECLIENT),
                     DWORD layer)
{
    gi.WriteByte(svc_layout);
    gi.WriteByte(layer);
    BuildUI(ent->client);
    gi.WriteLong(0); // end of list
    gi.unicast(ent);
}

void UI_SetPointByNumber(LPFRAMEDEF frame,
                         UIFRAMEPOINT framePoint,
                         DWORD otherNumber,
                         UIFRAMEPOINT otherPoint,
                         SHORT x,
                         SHORT y)
{
    frame->SetPoint.type = framePoint;
    frame->SetPoint.relativeTo = otherNumber;
    frame->SetPoint.target = otherPoint;
    frame->SetPoint.x = x;
    frame->SetPoint.y = y;
    SetPoint(NULL, frame);
}

void UI_SetPoint(LPFRAMEDEF frame,
                 UIFRAMEPOINT framePoint,
                 LPFRAMEDEF other,
                 UIFRAMEPOINT otherPoint,
                 SHORT x,
                 SHORT y)
{
    UI_SetPointByNumber(frame, framePoint, other ? other->f.number : 0, otherPoint, x, y);
}

void UI_SetAllPoints(LPFRAMEDEF frame) {
    UI_SetPointByNumber(frame, FRAMEPOINT_TOPLEFT, UI_PARENT, FRAMEPOINT_TOPLEFT, 0, 0);
    UI_SetPointByNumber(frame, FRAMEPOINT_BOTTOMRIGHT, UI_PARENT, FRAMEPOINT_BOTTOMRIGHT, 0, 0);
}

void UI_SetParent(LPFRAMEDEF frame, LPFRAMEDEF parent) {
    frame->f.parent = parent ? parent->f.number : 0;
}

void UI_SetText(LPFRAMEDEF frame, LPCSTR format, ...) {
    va_list argptr;
    static char text[1024];
    va_start(argptr, format);
    vsprintf(text, format,argptr);
    va_end(argptr);
    strcpy(frame->Text, UI_GetString(text));
    frame->f.text = frame->Text;
}

void UI_SetSize(LPFRAMEDEF frame, DWORD width, DWORD height) {
    frame->f.size.width = width;
    frame->f.size.height = height;
}

LPCSTR UI_GetString(LPCSTR textID) {
    FOR_EACH_LIST(stringListItem_t, it, strings) {
        if (!strcmp(textID, it->name)) {
            return it->value;
        }
    }
    return textID;
}

void UI_SetTexture(LPFRAMEDEF frame, LPCSTR name, BOOL decorate) {
    frame->f.tex.index = UI_LoadTexture(name, decorate);
}

void UI_SetTexture2(LPFRAMEDEF frame, LPCSTR name, BOOL decorate) {
    frame->f.tex.index2 = UI_LoadTexture(name, decorate);
}

void UI_SetHidden(LPFRAMEDEF frame, BOOL value) {
    frame->hidden = value;
}
