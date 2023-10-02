#include "../g_local.h"

#define SPRINTF_ADD(TEXT, ...) \
snprintf(TEXT+strlen(TEXT), sizeof(UINAME), __VA_ARGS__);

#define CONVERT_UV(DEST, SRC) \
DEST[0] = SRC.min.x * 0xff; \
DEST[1] = SRC.max.x * 0xff; \
DEST[2] = SRC.min.y * 0xff; \
DEST[3] = SRC.max.y * 0xff;

#define MAX_FRAMES_WRITE 256

static LPCFRAMEDEF framesWritten[MAX_FRAMES_WRITE];
LPCFRAMEDEF *frameptr = framesWritten;

typedef struct sizeBuf_s *LPSIZEBUF;

typedef struct sizeBuf_s {
    LPBYTE data;
    DWORD maxsize;
    DWORD cursize;
    DWORD readcount;
} sizeBuf_t;

void MSG_Write(LPSIZEBUF buf, LPCVOID value, DWORD size) {
    if (buf->cursize + size > buf->maxsize) {
        fprintf(stderr, "Write buffer overflow\n");
        return;
    }
    memcpy(buf->data + buf->cursize, value, size);
    buf->cursize += size;
}

#define TOKEN_PASTE(x, y) x##y
#define CAT(x,y) TOKEN_PASTE(x,y)
#define WRITE(TYPE, BUFFER, VALUE) \
TYPE CAT(var,__LINE__) = VALUE; \
MSG_Write(BUFFER, &CAT(var,__LINE__), sizeof(TYPE));

static BOOL AddFrame(LPCFRAMEDEF frame) {
    if (frameptr - framesWritten < MAX_FRAMES_WRITE) {
        *(frameptr++) = frame;
        return true;
    } else {
        return false;
    }
    return false;
}

static DWORD FindFrameNumber(LPCFRAMEDEF frame, DWORD def) {
    for (LPCFRAMEDEF *it = framesWritten; it < frameptr; it++) {
        if (*it == frame) {
            def = (DWORD)(it - framesWritten) + 1;
        }
    }
    return def;
}

void UI_CopyFrameBase(LPUIFRAME dest, LPCFRAMEDEF src) {
    AddFrame(src);
    FOR_LOOP(i, FPP_COUNT*2) {
        dest->points.x[i].targetPos = src->Points.x[i].targetPos;
        dest->points.x[i].used = src->Points.x[i].used;
        dest->points.x[i].relativeTo = FindFrameNumber(src->Points.x[i].relativeTo, UI_PARENT);
        dest->points.x[i].offset = src->Points.x[i].offset * UI_FRAMEPOINT_SCALE;
    }
    static char tooltip[1024];
    memset(tooltip, 0, sizeof(tooltip));
    sprintf(tooltip, "%s\n%s", src->Tip, src->Ubertip);
    CONVERT_UV(dest->tex.coord, src->Texture.TexCoord);
    dest->number = FindFrameNumber(src, 0);
    dest->parent = FindFrameNumber(src->Parent, 0);
    dest->color = src->Color;
    dest->size.width = src->Width;
    dest->size.height = src->Height;
    dest->tex.index = src->Texture.Image;
    dest->tex.index2 = src->Texture.Image2;
    dest->flags.type = src->Type;
    dest->flags.alphaMode = src->AlphaMode;
    dest->textLength = src->TextLength;
    dest->stat = src->Stat;
    dest->text = src->Text;
    dest->tooltip = tooltip;
    dest->onclick = src->OnClick;
}

static uiBackdrop_t MakeBackdrop(LPCFRAMEDEF frame) {
    return MAKE(uiBackdrop_t,
        .CornerFlags = frame->Backdrop.CornerFlags,
        .TileBackground = frame->Backdrop.TileBackground,
        .Background = frame->Backdrop.Background,
        .CornerSize = frame->Backdrop.CornerSize,
        .BackgroundSize = frame->Backdrop.BackgroundSize,
        .BackgroundInsets = {
            frame->Backdrop.BackgroundInsets[0],
            frame->Backdrop.BackgroundInsets[1],
            frame->Backdrop.BackgroundInsets[2],
            frame->Backdrop.BackgroundInsets[3],
        },
        .EdgeFile = frame->Backdrop.EdgeFile,
        .BlendAll = frame->Backdrop.BlendAll,
    );
}

static uiLabel_t MakeLabel(LPCFRAMEDEF frame) {
    return MAKE(uiLabel_t,
        .textalignx = frame->Font.Justification.Horizontal,
        .textaligny = frame->Font.Justification.Vertical,
        .offsetx = frame->Font.Justification.Offset.x,
        .offsety = frame->Font.Justification.Offset.y,
        .font = frame->Font.Index,
    );
}

static void WriteBackdrop(LPCFRAMEDEF frame, sizeBuf_t *sb, LPSTR buffer) {
    uiBackdrop_t data = MakeBackdrop(frame);
    MSG_Write(sb, &data, sizeof(data));
}

static void WriteTooltipText(LPCFRAMEDEF frame, sizeBuf_t *sb, LPSTR buffer) {
    uiTooltip_t data = {
        .background = MakeBackdrop(frame),
        .text = MakeLabel(frame),
    };
    MSG_Write(sb, &data, sizeof(data));
}

static void WriteSimpleButton(LPCFRAMEDEF frame, sizeBuf_t *sb, LPSTR buffer) {
    LPCFRAMEDEF NormalTexture = UI_FindFrame(frame->Button.NormalTexture);
    LPCFRAMEDEF PushedTexture = UI_FindFrame(frame->Button.PushedTexture);
    LPCFRAMEDEF DisabledTexture = UI_FindFrame(frame->Button.DisabledTexture);
    LPCFRAMEDEF UseHighlight = UI_FindFrame(frame->Button.UseHighlight);

    LPCFRAMEDEF NormalText = UI_FindFrame(frame->Button.NormalText.frame);
    LPCFRAMEDEF DisabledText = UI_FindFrame(frame->Button.DisabledText.frame);
    LPCFRAMEDEF HighlightText = UI_FindFrame(frame->Button.HighlightText.frame);

    uiSimpleButton_t data = { 0 };
    
    CONVERT_UV(data.normal.texcoord, NormalTexture->Texture.TexCoord);
    CONVERT_UV(data.pushed.texcoord, PushedTexture->Texture.TexCoord);
    CONVERT_UV(data.disabled.texcoord, DisabledTexture->Texture.TexCoord);
    CONVERT_UV(data.highlight.texcoord, UseHighlight->Texture.TexCoord);
    
    data.normal.texture = NormalTexture->Texture.Image;
    data.pushed.texture = PushedTexture->Texture.Image;
    data.disabled.texture = DisabledTexture->Texture.Image;
    data.highlight.texture = UseHighlight->Texture.Image;

    data.normal.font = NormalText->Font.Index;
    data.pushed.font = NormalText->Font.Index;
    data.disabled.font = DisabledText->Font.Index;
    data.highlight.font = UseHighlight->Font.Index;

    data.normal.fontcolor = NormalText->Font.Color;
    data.pushed.fontcolor = NormalText->Font.Color;
    data.disabled.fontcolor = DisabledText->Font.Color;
    data.highlight.fontcolor = UseHighlight->Font.Color;

    strcpy(buffer, UI_GetString(frame->Button.NormalText.text));

    MSG_Write(sb, &data, sizeof(data));
}

static void WriteGlueTextButton(LPCFRAMEDEF frame, sizeBuf_t *sb, LPSTR buffer) {
    LPCFRAMEDEF NormalBackdrop = UI_FindFrame(frame->Control.Backdrop.Normal);
    LPCFRAMEDEF PushedBackdrop = UI_FindFrame(frame->Control.Backdrop.Pushed);
    LPCFRAMEDEF DisabledBackdrop = UI_FindFrame(frame->Control.Backdrop.Disabled);
    LPCFRAMEDEF DisabledPushedBackdrop = UI_FindFrame(frame->Control.Backdrop.DisabledPushed);
    LPCFRAMEDEF MouseOverBackdrop = UI_FindFrame(frame->Control.Backdrop.MouseOver);
    uiGlueTextButton_t data = {
        .normal = MakeBackdrop(NormalBackdrop),
        .pushed = MakeBackdrop(PushedBackdrop),
        .disabled = MakeBackdrop(DisabledBackdrop),
        .disabledPushed = MakeBackdrop(DisabledPushedBackdrop),
        .highlight = MakeBackdrop(MouseOverBackdrop),
    };
    MSG_Write(sb, &data, sizeof(data));
}

static void WriteTextArea(LPCFRAMEDEF frame, sizeBuf_t *sb, uiFrame_t *tmp) {
    uiTextArea_t data = {
        .font = frame->Font.Index,
        .inset = frame->TextArea.Inset,
    };
    MSG_Write(sb, &data, sizeof(data));
}

static void WriteLabel(LPCFRAMEDEF frame, sizeBuf_t *sb, uiFrame_t *tmp) {
    uiLabel_t data = MakeLabel(frame);
    if(!tmp->points.x[FPP_MIN].used &&
       !tmp->points.x[FPP_MID].used &&
       !tmp->points.x[FPP_MAX].used)
    {
        DWORD anchor = frame->Font.Justification.Horizontal ^ 1; // to map FontJustification to FramePointPos
        tmp->points.x[anchor].targetPos = anchor;
        tmp->points.x[anchor].relativeTo = UI_PARENT;
        tmp->points.x[anchor].used = true;
    }
    if(!tmp->points.y[FPP_MIN].used &&
       !tmp->points.y[FPP_MID].used &&
       !tmp->points.y[FPP_MAX].used)
    {
        DWORD anchor = frame->Font.Justification.Vertical ^ 1; // to map FontJustification to FramePointPos
        tmp->points.y[anchor].targetPos = anchor;
        tmp->points.y[anchor].relativeTo = UI_PARENT;
        tmp->points.y[anchor].used = true;
    }
    tmp->color = frame->Font.Color;
    if (*frame->Text == '\0') {
        tmp->text = frame->Name;
    }
    MSG_Write(sb, &data, sizeof(data));
}

static void WriteBuildQueue(LPCFRAMEDEF frame, sizeBuf_t *sb) {
    uiBuildQueue_t data = {
        .firstitem = FindFrameNumber(frame->BuildQueue.FirstItem, 0),
        .buildtimer = FindFrameNumber(frame->BuildQueue.BuildTimer, 0),
        .itemoffset = frame->BuildQueue.ItemOffset,
        .numitems = frame->BuildQueue.NumQueue,
    };
    MSG_Write(sb, &data, sizeof(data));
    MSG_Write(sb, &frame->BuildQueue.Queue, sizeof(uiBuildQueueItem_t) * data.numitems);
}

static void WriteMultiselect(LPCFRAMEDEF frame, sizeBuf_t *sb) {
    uiMultiselect_t data = {
        .hp_bar = frame->Multiselect.HpBar,
        .mana_bar = frame->Multiselect.ManaBar,
        .numcolumns = frame->Multiselect.NumColumns,
        .numitems = frame->Multiselect.NumItems,
        .offset = frame->Multiselect.Offset,
    };
    MSG_Write(sb, &data, sizeof(data));
    MSG_Write(sb, &frame->Multiselect.Items, sizeof(uiBuildQueueItem_t) * data.numitems);
}

static void WriteHighlight(LPCFRAMEDEF frame, sizeBuf_t *sb) {
    uiHighlight_t data = {
        .alphaFile = frame->Highlight.AlphaFile,
        .alphaMode = frame->Highlight.AlphaMode,
    };
    MSG_Write(sb, &data, sizeof(data));
}

void UI_WriteFrame(LPCFRAMEDEF frame) {
    UINAME buffer;
    uiFrame_t tmp;
    memset(&tmp, 0, sizeof(tmp));
    memset(&buffer, 0, sizeof(buffer));
    BYTE typedata[256] = { 0};
    sizeBuf_t buf = {
        .data = typedata,
        .maxsize = sizeof(typedata),
    };
    UI_CopyFrameBase(&tmp, frame);
    switch (frame->Type) {
        case FT_BACKDROP:
            WriteBackdrop(frame, &buf, buffer);
            break;
        case FT_TOOLTIPTEXT:
            WriteTooltipText(frame, &buf, buffer);
            break;
        case FT_SIMPLEBUTTON:
            WriteSimpleButton(frame, &buf, buffer);
            tmp.text = buffer;
            break;
//        case FT_SIMPLESTATUSBAR:
//            break;
        case FT_BUILDQUEUE:
            WriteBuildQueue(frame, &buf);
            break;
        case FT_MULTISELECT:
            WriteMultiselect(frame, &buf);
            break;
        case FT_GLUEBUTTON:
        case FT_GLUETEXTBUTTON:
            WriteGlueTextButton(frame, &buf, buffer);
            tmp.text = buffer;
            break;
        case FT_HIGHLIGHT:
            WriteHighlight(frame, &buf);
            break;
        case FT_STRING:
        case FT_TEXT:
            WriteLabel(frame, &buf, &tmp);
            break;
        case FT_TEXTAREA:
            WriteTextArea(frame, &buf, &tmp);
            break;
        case FT_PORTRAIT:
            tmp.tex.index = frame->Portrait.model;
            break;
        default:
            break;
    }
    tmp.buffer.size = buf.cursize;
    tmp.buffer.data = buf.data;
    tmp.flags.type = frame->Type;
    gi.WriteUIFrame(&tmp);
}

void UI_WriteStart(DWORD layer) {
    gi.WriteByte(svc_layout);
    gi.WriteByte(layer);
    frameptr = framesWritten;
}

void UI_WriteLayout(LPEDICT ent, LPCFRAMEDEF root, DWORD layer) {
    UI_WriteStart(layer);
    UI_WriteFrameWithChildren(root, NULL);
    gi.WriteLong(0); // end of list
    gi.unicast(ent);
}
