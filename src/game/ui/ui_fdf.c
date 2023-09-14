#include "../g_local.h"
#include "../parser.h"

#define UINAME_FMT "\"%79[^\"]\""
#define PATHSTR_FMT "\"%255[^\"]\""

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
LPCSTR HighlightType[] = {
    "FILETEXTURE",
    NULL
};
LPCSTR AlphaMode[] = {
    "ALPHAKEY",
    "ADD",
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
LPCSTR ControlStyle[] = {
    "AUTOTRACK",
    "HIGHLIGHTONFOCUS",
    "HIGHLIGHTONMOUSEOVER",
    NULL
};
LPCSTR CornerFlags[] = {
    "UL",
    "T",
    "UR",
    "L",
    "-",
    "R",
    "BL",
    "B",
    "BR",
    NULL
};

#define MAX_UI_CLASSES MAX_LAYOUT_OBJECTS
#define F(x, type) { #x,((uint8_t *)&((FRAMEDEF *)0)->x - (uint8_t *)NULL), Parse##type }
#define ITEM(x) { #x, x }
#define TARGET(TYPE) TYPE *target = (TYPE *)((uint8_t *)frame + arg->fofs);

FRAMEDEF frames[MAX_UI_CLASSES] = { 0 };

void FDF_ParseFrame(LPPARSER p, LPFRAMEDEF frame);

void UI_PrintClasses(void) {
    FOR_LOOP(i, MAX_UI_CLASSES) {
        printf("%d  %s\n", frames[i].Number, frames[i].Name);
    }
}

void UI_ClearTemplates(void) {
    memset(frames, 0, sizeof(frames));
}

void UI_InitFrame(LPFRAMEDEF frame, DWORD number, FRAMETYPE type) {
    memset(frame, 0, sizeof(FRAMEDEF));
    frame->inuse = true;
    frame->Number = number;
    frame->Type = type;
    frame->Color = COLOR32_WHITE;
    frame->Text = frame->TextStorage;
    switch (type) {
        case FT_TEXTURE:
        case FT_SIMPLESTATUSBAR:
        case FT_COMMANDBUTTON:
        case FT_BACKDROP:
            frame->Texture.TexCoord.max.x = 1;
            frame->Texture.TexCoord.max.y = 1;
            break;
        case FT_TEXT:
            frame->Font.Color = COLOR32_WHITE;
            break;
        default:
            break;
    }
}

LPFRAMEDEF UI_Spawn(FRAMETYPE type, LPFRAMEDEF parent) {
    FOR_LOOP(i, MAX_UI_CLASSES) {
        if (i==0) continue;
        LPFRAMEDEF frame = &frames[i];
        if (!frame->inuse) {
            UI_InitFrame(frame, i, type);
            frame->Parent = parent;
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
    void (*func)(LPPARSER , LPFRAMEDEF);
} parseItem_t;

typedef struct {
    LPCSTR name;
    void (*func)(LPPARSER , LPFRAMEDEF);
} parseClass_t;

int ParseEnumString(LPCSTR token, LPCSTR const *values) {
    for (int i = 0; *values; i++, values++) {
        if (!strcmp(token, *values)) {
            return i;
        }
    }
    return -1;
}

int ParseEnum(LPPARSER p, LPCSTR const *values) {
    LPCSTR token = parse_token(p);
    int value = ParseEnumString(token, values);
    if (value == -1) {
        parser_error(p);
    }
    return value;
}

LPCSTR Theme_String(LPCSTR entry, LPCSTR category) {
    LPCSTR filename = gi.FindSheetCell(game.config.theme, category, entry);
    if (filename) {
        return filename;
    } else {
//        fprintf(stderr, "Can't find %s in skin\n", entry);
        return entry;
    }
}

FLOAT Theme_Float(LPCSTR entry, LPCSTR category) {
    return atof(Theme_String(entry, category));
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
    file = decorate ? Theme_String(file, "Default") : file;
    file = EnsureExtension(file, ".blp");
    return gi.ImageIndex(file);
}

#define MAKE_PARSER(TYPE) \
void Parse##TYPE(LPCSTR token, LPFRAMEDEF frame, void *out)

#define MAKE_PARSERCALL(TYPE) \
void TYPE(LPPARSER parser, LPFRAMEDEF frame)

#define MAKE_ENUMPARSER(TYPE) \
MAKE_PARSER(TYPE) { \
    *((DWORD *)out) = ParseEnumString(token, TYPE); \
}

#define MAKE_FLAGSPARSER(TYPE) \
MAKE_PARSER(TYPE) { \
    PATHSTR b; \
    sscanf(token, PATHSTR_FMT, b); \
    for (LPSTR s = b; *s; s++) *s = *s == '|' ? ',' : *s; \
    PARSE_LIST(b, flag, parse_segment) { \
        *((DWORD *)out) |= 1 << ParseEnumString(flag, TYPE); \
    } \
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
    vec4.w = 1;
    ParseVector4(token, frame, &vec4);
    ((COLOR32 *)out)->r = vec4.x * 0xff;
    ((COLOR32 *)out)->g = vec4.y * 0xff;
    ((COLOR32 *)out)->b = vec4.z * 0xff;
    ((COLOR32 *)out)->a = vec4.w * 0xff;
}

MAKE_PARSER(FrameNumber) {
    *(DWORD *)out = 0;
    UINAME name = {0};
    sscanf(token, UINAME_FMT, name);
    FOR_LOOP(i, MAX_UI_CLASSES) {
        int dist1 = abs((int)i - (int)frame->Number);
        int dist2 = abs(*((int *)out) - (int)frame->Number);
        if (dist1 > dist2)
            return;
        if (!strcmp(frames[i].Name, name)) {
            *(DWORD *)out = frames[i].Number;
        }
    }
}

MAKE_PARSER(Name) {
    memset(out, 0, sizeof(UINAME));
    sscanf(token, UINAME_FMT, (LPSTR)out);
}

MAKE_PARSER(ButtonText) {
    BUTTONTEXT *bt = out;
    memset(bt, 0, sizeof(BUTTONTEXT));
    assert(sscanf(token, UINAME_FMT " " UINAME_FMT, bt->frame, bt->text) == 2);
}

MAKE_PARSER(Text) {
    UINAME key = { 0 };
    sscanf(token, UINAME_FMT, key);
    LPCSTR str = UI_GetString(key);
    memset(out, 0, sizeof(UINAME));
    memcpy(out, str, strlen(str));
    frame->Text = out;
}

typedef struct stringListItem_s {
    struct stringListItem_s *next;
    UINAME name;
    LPSTR value;
} stringListItem_t;

stringListItem_t *strings = NULL;

MAKE_PARSER(StringListItem) {
    strings->value = strdup(token+1);
    strings->value[strlen(strings->value)-1] = 0;
}

MAKE_ENUMPARSER(AlphaMode);
MAKE_ENUMPARSER(FontJustificationH);
MAKE_ENUMPARSER(FontJustificationV);
MAKE_ENUMPARSER(HighlightType);
MAKE_ENUMPARSER(FontFlags);
MAKE_ENUMPARSER(FramePointType);
MAKE_FLAGSPARSER(CornerFlags);
MAKE_FLAGSPARSER(ControlStyle);

MAKE_PARSER(TextureFile) {
    PATHSTR path = { 0 };
    sscanf(token, PATHSTR_FMT, path);
//    BOOL decorate = frame->DecorateFileNames | (frame->Parent ? frame->Parent->DecorateFileNames : false);
    *((DWORD *)out) = UI_LoadTexture(path, true);
}

MAKE_PARSERCALL(Font) {
    LPCSTR file = Theme_String(frame->Font.Name, "Default");
    frame->Font.Index = gi.FontIndex(file, frame->Font.Size / 10);
}

MAKE_PARSERCALL(SetPoint) {
    if (!frame->AnyPointsSet) { // clear any template points
        memset(&frame->Points, 0, sizeof(frame->Points));
        frame->AnyPointsSet = true;
    }
    DWORD const x = frame->SetPoint.type & 3;
    uiFramePoint_t *xp = frame->Points.x;
    if (x != FPP_MID || (!xp[FPP_MIN].used && !xp[FPP_MAX].used)) {
        xp[FPP_MID].used = false;
        xp[x].used = true;
        xp[x].offset = frame->SetPoint.x;
        xp[x].targetPos = frame->SetPoint.target & 3;
        xp[x].relativeTo = frame->SetPoint.relativeTo;
    }
    DWORD y = (frame->SetPoint.type >> 2) & 3;
    uiFramePoint_t *yp = frame->Points.y;
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

MAKE_PARSERCALL(SliderLayoutHorizontal) {
    frame->Slider.Layout = LAYOUT_HORIZONTAL;
}

MAKE_PARSERCALL(SliderLayoutVertical) {
    frame->Slider.Layout = LAYOUT_VERTICAL;
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
    LPCSTR stype = parse_token(parser);
    FRAMETYPE type = ParseEnumString(stype, FrameType);
    LPFRAMEDEF current = UI_Spawn(type, frame);
    FDF_ParseFrame(parser, current);
}

MAKE_PARSERCALL(IncludeFile) {
    LPCSTR filename = parse_token(parser);
    UI_ParseFDF(filename);
}

MAKE_PARSERCALL(StringList) {
    FRAMEDEF string_list;
    memset(&string_list, 0, sizeof(FRAMEDEF));
    string_list.Type = FT_STRINGLIST;
    FDF_ParseFrame(parser, &string_list);
}

MAKE_PARSERCALL(EditHighlightInitial) {
    frame->Edit.HighlightInitial = true;
}

MAKE_PARSERCALL(EditSetFocus) {
    frame->Edit.Focus = true;
}

#define F_END { NULL }

static parseClass_t classes[] = {
    { "Frame", Frame },
    { "Texture", Texture },
    { "String", String },
    { "Layer", Layer },
    { "StringList", StringList },
    { "IncludeFile", IncludeFile },
    F_END,
};

static parseItem_t items[] = {
    // Flags
    { "DecorateFileNames", { F_END }, DecorateFileNames },
    { "SetAllPoints", F_END, SetAllPoints },
    { "SetPoint", { F(SetPoint.type, FramePointType), F(SetPoint.relativeTo, FrameNumber), F(SetPoint.target, FramePointType), F(SetPoint.x, Float16), F(SetPoint.y, Float16), F_END }, SetPoint },
    { "UseActiveContext", F_END, UseActiveContext },
    { "DialogBackdrop", { F(DialogBackdrop, FrameNumber), F_END } },
    // Fields
    { "Width", { F(Width, Float16), F_END } },
    { "Height", { F(Height, Float16), F_END } },
    { "File", { F(Texture.Image, TextureFile), F_END } },
    { "TexCoord", { F(Texture.TexCoord.min.x, Float), F(Texture.TexCoord.max.x, Float), F(Texture.TexCoord.min.y, Float), F(Texture.TexCoord.max.y, Float), F_END } },
    { "AlphaMode", { F(AlphaMode, AlphaMode), F_END } },
    { "Anchor", { F(Anchor.corner, FramePointType), F(Anchor.x, Float16), F(Anchor.y, Float16), F_END }, Anchor },
    { "Font", { F(Font.Name, Name), F(Font.Size, Float16), F_END }, Font },
    { "Text", { F(TextStorage, Text), F_END } },
    { "TextLength", { F(TextLength, Integer), F_END } },
    { "FrameFont", { F(Font.Name, Name), F(Font.Size, Float16), F(Font.Unknown, Name), F_END }, Font },
    // Font
    { "FontJustificationH", { F(Font.Justification.Horizontal, FontJustificationH), F_END } },
    { "FontJustificationV", { F(Font.Justification.Vertical, FontJustificationV), F_END } },
    { "FontJustificationOffset", { F(Font.Justification.Offset, Vector2), F_END } },
    { "FontFlags", { F(Font.FontFlags, FontFlags), F_END } },
    { "FontColor", { F(Font.Color, Color), F_END } },
    { "FontHighlightColor", { F(Font.HighlightColor, Color), F_END } },
    { "FontDisabledColor", { F(Font.DisabledColor, Color), F_END } },
    { "FontShadowColor", { F(Font.ShadowColor, Color), F_END } },
    { "FontShadowOffset", { F(Font.ShadowOffset, Vector2), F_END } },
    // Backdrop
    { "BackdropTileBackground", F_END, BackdropTileBackground },
    { "BackdropBackground", { F(Backdrop.Background, TextureFile), F_END } },
    { "BackdropCornerFlags", { F(Backdrop.CornerFlags, CornerFlags), F_END } },
    { "BackdropCornerSize", { F(Backdrop.CornerSize, Float16), F_END } },
    { "BackdropBackgroundSize", { F(Backdrop.BackgroundSize, Float16), F_END } },
    { "BackdropBackgroundInsets", { F(Backdrop.BackgroundInsets, Float16_4), F_END } },
    { "BackdropEdgeFile", { F(Backdrop.EdgeFile, TextureFile), NULL } },
    { "BackdropBlendAll", F_END, BackdropBlendAll },
    // Highlight
    { "HighlightType", { F(Highlight.Type, HighlightType), F_END } },
    { "HighlightAlphaFile", { F(Highlight.AlphaFile, TextureFile), F_END } },
    { "HighlightAlphaMode", { F(Highlight.AlphaMode, AlphaMode), F_END } },
    // Control
    { "ControlStyle", { F(Control.Style, ControlStyle), F_END } },
    { "ControlBackdrop", { F(Control.Backdrop.Normal, Name), F_END } },
    { "ControlPushedBackdrop", { F(Control.Backdrop.Pushed, Name), F_END } },
    { "ControlDisabledBackdrop", { F(Control.Backdrop.Disabled, Name), F_END } },
    { "ControlMouseOverHighlight", { F(Control.Backdrop.MouseOver, Name), F_END } },
    { "ControlDisabledPushedBackdrop", { F(Control.Backdrop.DisabledPushed, Name), F_END } },
    // Slider
    { "SliderInitialValue", { F(Slider.InitialValue, Float), F_END } },
    { "SliderLayoutHorizontal", { F_END }, SliderLayoutHorizontal },
    { "SliderLayoutVertical", { F_END }, SliderLayoutVertical },
    { "SliderMaxValue", { F(Slider.MaxValue, Float), F_END } },
    { "SliderMinValue", { F(Slider.MinValue, Float), F_END } },
    { "SliderStepSize", { F(Slider.StepSize, Float), F_END } },
    { "SliderThumbButtonFrame", { F(Slider.ThumbButtonFrame, Name), F_END } },
    // Menu
    { "MenuBorder", { F(Menu.Border, Float), F_END } },
    { "MenuItem", { F(Menu.Item.Text, Name), F(Menu.Item.Value, Integer), F_END } },
    { "MenuItemHeight", { F(Menu.Item.Height, Float), F_END } },
    { "MenuTextHighlightColor", { F(Menu.TextHighlightColor, Color), F_END } },
    // Edit
    { "EditBorderSize", { F(Edit.BorderSize, Float), F_END } },
    { "EditCursorColor", { F(Edit.CursorColor, Color), F_END } },
    { "EditHighlightColor", { F(Edit.HighlightColor, Color), F_END } },
    { "EditHighlightInitial", { F_END }, EditHighlightInitial },
    { "EditMaxChars", { F(Edit.MaxChars, Integer), F_END } },
    { "EditSetFocus", { F_END }, EditSetFocus },
    { "EditText", { F(Edit.Text, Text), F_END } },
    { "EditTextColor", { F(Edit.TextColor, Color), F_END } },
    { "EditTextFrame", { F(Edit.TextFrame, Name), F_END } },
    { "EditTextOffset", { F(Edit.TextOffset, Vector2), F_END } },
    // Popup
    { "PopupButtonInset", { F(Popup.ButtonInset, Float), F_END } },
    { "PopupArrowFrame", { F(Popup.ArrowFrame, Name), F_END } },
    { "PopupMenuFrame", { F(Popup.MenuFrame, Name), F_END } },
    { "PopupTitleFrame", { F(Popup.TitleFrame, Name), F_END } },
    // TextArea
    { "TextAreaLineHeight", { F(TextArea.LineHeight, Float), F_END } },
    { "TextAreaLineGap", { F(TextArea.LineGap, Float), F_END } },
    { "TextAreaInset", { F(TextArea.Inset, Float), F_END } },
    { "TextAreaScrollBar", { F(TextArea.ScrollBar, Name), F_END } },
    { "TextAreaMaxLines", { F(TextArea.MaxLines, Integer), F_END } },
    // CheckBox
    { "CheckBoxCheckHighlight", { F(CheckBox.CheckHighlight, Name), F_END } },
    { "CheckBoxDisabledCheckHighlight", { F(CheckBox.DisabledCheckHighlight, Name), F_END } },
    // Button
    { "ButtonText", { F(TextStorage, Text), F_END } },
    { "ButtonPushedTextOffset", { F(Button.PushedTextOffset, Vector2), F_END } },
    { "NormalTexture", { F(Button.NormalTexture, Name), F_END } },
    { "PushedTexture", { F(Button.PushedTexture, Name), F_END } },
    { "DisabledTexture", { F(Button.DisabledTexture, Name), F_END } },
    { "NormalText", { F(Button.NormalText, ButtonText), F_END } },
    { "DisabledText", { F(Button.DisabledText, ButtonText), F_END } },
    { "HighlightText", { F(Button.HighlightText, ButtonText), F_END } },
    { "UseHighlight", { F(Button.UseHighlight, Name), F_END } },
    // End of list
    F_END
};

LPCSTR parse_segment2(LPPARSER p);

void parse_item(LPPARSER parser, LPFRAMEDEF frame, parseItem_t *item) {
    for (parseArg_t *arg = item->args; arg->name; arg++) {
        LPCSTR token = parse_segment2(parser);
        arg->func(token, frame, (uint8_t *)frame + arg->fofs);
    }
    if (!item->args->name) { // eat trailing comma
        parse_segment(parser);
    }
    if (item->func) {
        item->func(parser, frame);
    }
}

void parse_func(LPPARSER parser, LPFRAMEDEF frame) {
    LPCSTR token = NULL;
    while ((token = parse_token(parser)) && (*token != '}')) {
        if (frame->Type == FT_STRINGLIST) {
            static parseItem_t stringitem = { "", { F(Name, StringListItem), F_END } };
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
    if (inherit && inherit->Type == frame->Type) {
        FRAMEDEF tmp;
        memcpy(&tmp, frame, sizeof(FRAMEDEF));
        memcpy(frame, inherit, sizeof(FRAMEDEF));
        memcpy(frame->Name, tmp.Name, sizeof(UINAME));
        frame->Parent = tmp.Parent;
        frame->Number = tmp.Number;
        frame->AnyPointsSet = false;
    } else if (inherit) {
        fprintf(stderr, "Can't inherit from different type %s\n", inheritName);
    } else {
        fprintf(stderr, "Can't find template %s\n", inheritName);
    }
}

void FDF_ParseFrame(LPPARSER p, LPFRAMEDEF frame) {
    DWORD state = 0;
    LPCSTR tok;
    while ((tok = parse_token(p)) && (*tok != '{')) {
        if (!strcmp(tok, "INHERITS")) {
            LPCSTR inheritName = parse_token(p);
            BOOL withChildren = false;
            if (!strcmp(inheritName, "WITHCHILDREN")) {
                withChildren = true;
                inheritName = parse_token(p);
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
        if (frames[i].Parent != frame)
            continue;
        LPFRAMEDEF found = UI_FindChildFrame(frames+i, name);
        if (found)
            return found;
    }
    return NULL;
}

void FDF_ParseScene(LPPARSER parser) {
    LPCSTR token = NULL;
    LPFRAMEDEF frame = NULL;
    while (*(token = parse_token(parser))) {
        for (parseClass_t *it = classes; it->name; it++) {
            if (!strcmp(it->name, token)) {
                it->func(parser, frame);
                goto parse_next;
            }
        }
        fprintf(stderr, "Unknown token %s\n", token);
        parser_error(parser);
        return;
    parse_next:;
        while (*parser->buffer == ',') {
            ++parser->buffer;
        }
    }
}

void UI_ParseFDF_Buffer(LPCSTR fileName, LPSTR buffer2) {
    LPSTR buffer = buffer2;
    gi.TextRemoveComments(buffer);
    gi.TextRemoveBom(buffer);
    PARSER parser = {
        .buffer = buffer,
        .delimiters = ",;{}",
        .eat_quotes = true,
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

void UI_WriteFrameWithChildren(LPCFRAMEDEF frame) {
    UI_WriteFrame(frame);
    FOR_LOOP(i, MAX_UI_CLASSES) {
        LPCFRAMEDEF it = frames+i;
        if (it->Parent == frame && it->Number > 0 && !it->hidden) {
            UI_WriteFrameWithChildren(it);
        }
    }
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
    UI_SetPointByNumber(frame, framePoint, other ? other->Number : 0, otherPoint, x, y);
}

void UI_SetAllPoints(LPFRAMEDEF frame) {
    UI_SetPointByNumber(frame, FRAMEPOINT_TOPLEFT, UI_PARENT, FRAMEPOINT_TOPLEFT, 0, 0);
    UI_SetPointByNumber(frame, FRAMEPOINT_BOTTOMRIGHT, UI_PARENT, FRAMEPOINT_BOTTOMRIGHT, 0, 0);
}

void UI_SetParent(LPFRAMEDEF frame, LPFRAMEDEF parent) {
    frame->Parent = parent;
}

void UI_SetText(LPFRAMEDEF frame, LPCSTR format, ...) {
    va_list argptr;
    static char text[1024];
    va_start(argptr, format);
    vsprintf(text, format,argptr);
    va_end(argptr);
    strcpy(frame->TextStorage, UI_GetString(text));
    frame->Text = frame->TextStorage;
}

void UI_SetSize(LPFRAMEDEF frame, DWORD width, DWORD height) {
    frame->Width = width;
    frame->Height = height;
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
    frame->Texture.Image = UI_LoadTexture(name, decorate);
}

void UI_SetTexture2(LPFRAMEDEF frame, LPCSTR name, BOOL decorate) {
    frame->Texture.Image2 = UI_LoadTexture(name, decorate);
}

void UI_SetHidden(LPFRAMEDEF frame, BOOL value) {
    frame->hidden = value;
}
