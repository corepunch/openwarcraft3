/*
 * ui_write.c — Serialize the UI frame tree for transmission to clients.
 *
 * UI_WriteLayout() traverses a frame tree depth-first and emits an
 * svc_layout message containing delta-encoded uiFrame_t structs.  For each
 * frame, UI_WriteFrame() copies the base properties (anchor points, size,
 * texture, color) and appends a small type-specific data block (backdrop
 * edges, button texture states, label font settings, etc.).
 *
 * The client receives the blob in CL_ParseLayout() (cl_parse.c) and stores
 * it verbatim.  The renderer reads it each frame to draw the UI without the
 * client needing to understand the frame hierarchy.
 *
 * Dynamic updates (e.g. command card changes) follow the same path: the
 * server calls UI_WriteLayout() again with the modified sub-tree to replace
 * a specific UI layer on the client.
 *
 * Sending a UI layer to a client
 * ------------------------------
 * Frames are written into an outgoing buffer with UI_WriteFrame().  The
 * UI_WRITE_LAYER macro wraps the complete send sequence — opening the layer
 * message, invoking a builder function, appending the end-of-list sentinel,
 * and unicasting the result to a single client:
 *
 *   UI_WRITE_LAYER(edict, ui_print_text, LAYER_MESSAGE, message);
 *
 * Equivalent explicit form:
 *
 *   UI_WriteStart(LAYER_MESSAGE);      // begin svc_layout for this layer
 *   ui_print_text(ent->client, text);  // emit frames into the buffer
 *   gi.WriteLong(0);                   // end-of-list sentinel
 *   gi.unicast(ent);                   // send to the client
 *
 * Available layers (defined in g_local.h):
 *
 *   LAYER_BACKGROUND  — Glue/menu background models
 *   LAYER_PORTRAIT    — Unit portrait
 *   LAYER_CINEMATIC   — Cinematic panel
 *   LAYER_CONSOLE     — Main HUD console
 *   LAYER_COMMANDBAR  — Ability/command buttons
 *   LAYER_INFOPANEL   — Selected-unit info panel
 *   LAYER_INVENTORY   — Inventory slot buttons
 *   LAYER_MESSAGE     — On-screen text messages
 *   LAYER_QUESTDIALOG — Quest dialog overlay
 */
#include "../g_local.h"

#define SPRINTF_ADD(TEXT, ...) \
snprintf(TEXT+strlen(TEXT), sizeof(UINAME), __VA_ARGS__);

#define CONVERT_UV(DEST, SRC) \
DEST[0] = SRC.min.x * 0xff; \
DEST[1] = SRC.max.x * 0xff; \
DEST[2] = SRC.min.y * 0xff; \
DEST[3] = SRC.max.y * 0xff;

#define MAX_FRAMES_WRITE 1024

static LPCFRAMEDEF framesWritten[MAX_FRAMES_WRITE];
LPCFRAMEDEF *frameptr = framesWritten;
DWORD layoutBytesWritten = 0;

typedef struct sizeBuf_s *LPSIZEBUF;

typedef struct sizeBuf_s {
    LPBYTE data;
    DWORD maxsize;
    DWORD cursize;
    DWORD readcount;
} sizeBuf_t;

static void MSG_Write(LPSIZEBUF buf, LPCVOID value, DWORD size) {
    if (buf->cursize + size > buf->maxsize) {
        fprintf(stderr, "Write buffer overflow (ui)\n");
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
    if (src->Tip || src->Ubertip) {
        snprintf(tooltip,
                 sizeof(tooltip),
                 "%s\n%s",
                 src->Tip ? src->Tip : "",
                 src->Ubertip ? src->Ubertip : "");
    }
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
    dest->tooltip = tooltip[0] ? tooltip : NULL;
    dest->onclick = src->OnClick;
}

static uiBackdrop_t MakeBackdrop(LPCFRAMEDEF frame) {
    if (!frame) {
        return (uiBackdrop_t) { 0 };
    }
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
        .Mirrored = frame->Backdrop.Mirrored,
    );
}

static uiHighlight_t MakeHighlight(LPCFRAMEDEF frame) {
    if (!frame) {
        return (uiHighlight_t) { 0 };
    }
    return MAKE(uiHighlight_t,
        .alphaFile = frame->Highlight.AlphaFile,
        .alphaMode = frame->Highlight.AlphaMode);
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
    LPCFRAMEDEF NormalBackdrop = UI_FindFrameNear(frame, frame->Control.Backdrop.Normal);
    LPCFRAMEDEF PushedBackdrop = UI_FindFrameNear(frame, frame->Control.Backdrop.Pushed);
    LPCFRAMEDEF DisabledBackdrop = UI_FindFrameNear(frame, frame->Control.Backdrop.Disabled);
    LPCFRAMEDEF DisabledPushedBackdrop = UI_FindFrameNear(frame, frame->Control.Backdrop.DisabledPushed);
    LPCFRAMEDEF MouseOverBackdrop = UI_FindFrameNear(frame, frame->Control.Backdrop.MouseOver);
    uiGlueTextButton_t data = {
        .normal = MakeBackdrop(NormalBackdrop),
        .pushed = MakeBackdrop(PushedBackdrop),
        .disabled = MakeBackdrop(DisabledBackdrop),
        .disabledPushed = MakeBackdrop(DisabledPushedBackdrop),
        .highlight = MakeHighlight(MouseOverBackdrop),
        .pushedTextOffset = frame->Button.PushedTextOffset,
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

static void WriteEditBox(LPCFRAMEDEF frame, sizeBuf_t *sb, uiFrame_t *tmp) {
    LPCFRAMEDEF Backdrop = UI_FindFrameNear(frame, frame->Control.Backdrop.Normal);
    LPCFRAMEDEF TextFrame = UI_FindFrameNear(frame, frame->Edit.TextFrame);
    COLOR32 text_color = frame->Edit.TextColor.a ? frame->Edit.TextColor :
                         TextFrame && TextFrame->Font.Color.a ? TextFrame->Font.Color :
                         frame->Font.Color;
    COLOR32 cursor_color = frame->Edit.CursorColor.a ? frame->Edit.CursorColor : COLOR32_WHITE;
    uiEditBox_t data = {
        .background = MakeBackdrop(Backdrop),
        .font = TextFrame ? TextFrame->Font.Index : frame->Font.Index,
        .borderSize = frame->Edit.BorderSize,
        .textColor = text_color.a ? text_color : COLOR32_WHITE,
        .cursorColor = cursor_color,
        .maxChars = frame->Edit.MaxChars,
    };

    if (frame->Edit.Text[0]) {
        tmp->text = frame->Edit.Text;
    } else if (!tmp->text || !*tmp->text) {
        tmp->text = "";
    }
    MSG_Write(sb, &data, sizeof(data));
}

static void WriteListBox(LPCFRAMEDEF frame, sizeBuf_t *sb, uiFrame_t *tmp) {
    LPCFRAMEDEF Backdrop = UI_FindFrameNear(frame, frame->Control.Backdrop.Normal);
    uiListBox_t data = {
        .background = MakeBackdrop(Backdrop),
        .text = MakeLabel(frame),
        .border = frame->ListBox.Border,
        .itemHeight = frame->Menu.Item.Height,
        .selectedIndex = -1,
    };
    (void)tmp;
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
        tmp->points.x[anchor].used = 1;
    }
    if(!tmp->points.y[FPP_MIN].used &&
       !tmp->points.y[FPP_MID].used &&
       !tmp->points.y[FPP_MAX].used)
    {
        DWORD anchor = frame->Font.Justification.Vertical ^ 1; // to map FontJustification to FramePointPos
        tmp->points.y[anchor].targetPos = anchor;
        tmp->points.y[anchor].relativeTo = UI_PARENT;
        tmp->points.y[anchor].used = 1;
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

/* Build a serialized uiFrame_t + typedata payload for a single frame without
 * writing to the network message stream. This is a deterministic seam used by
 * tests and by UI_WriteFrame itself. */
BOOL UI_BuildFrameForWrite(LPCFRAMEDEF frame,
                           LPUIFRAME out,
                           LPBYTE typedata,
                           DWORD typedata_max,
                           LPSTR textbuf,
                           DWORD textbuf_max)
{
    sizeBuf_t buf;

    if (!frame || !out || !typedata || typedata_max == 0 || !textbuf || textbuf_max == 0) {
        return false;
    }

    memset(out, 0, sizeof(*out));
    memset(typedata, 0, typedata_max);
    memset(textbuf, 0, textbuf_max);

    buf = (sizeBuf_t) {
        .data = typedata,
        .maxsize = typedata_max,
    };

    UI_CopyFrameBase(out, frame);
    switch (frame->Type) {
        case FT_BACKDROP:
            WriteBackdrop(frame, &buf, textbuf);
            break;
        case FT_TOOLTIPTEXT:
            WriteTooltipText(frame, &buf, textbuf);
            break;
        case FT_SIMPLEBUTTON:
            WriteSimpleButton(frame, &buf, textbuf);
            out->text = textbuf;
            break;
        case FT_BUILDQUEUE:
            WriteBuildQueue(frame, &buf);
            break;
        case FT_MULTISELECT:
            WriteMultiselect(frame, &buf);
            break;
        case FT_GLUEBUTTON:
        case FT_GLUETEXTBUTTON:
            WriteGlueTextButton(frame, &buf, textbuf);
            out->text = textbuf;
            break;
        case FT_HIGHLIGHT:
            WriteHighlight(frame, &buf);
            break;
        case FT_STRING:
        case FT_TEXT:
            WriteLabel(frame, &buf, out);
            break;
        case FT_TEXTAREA:
            WriteTextArea(frame, &buf, out);
            break;
        case FT_EDITBOX:
        case FT_GLUEEDITBOX:
        case FT_SLASHCHATBOX:
            WriteEditBox(frame, &buf, out);
            break;
        case FT_LISTBOX:
            WriteListBox(frame, &buf, out);
            break;
        case FT_MODEL:
        case FT_SPRITE:
        case FT_PORTRAIT:
            out->tex.index = frame->Portrait.model;
            break;
        default:
            break;
    }

    out->buffer.size = buf.cursize;
    out->buffer.data = buf.data;
    out->flags.type = frame->Type;

    return true;
}

/* Serialize a single frame and its type-specific data block into the outgoing
 * svc_layout message.  The frame number is encoded relative to the list of
 * frames already written in this layout pass so parent references stay valid. */
void UI_WriteFrame(LPCFRAMEDEF frame) {
    UINAME textbuf;
    uiFrame_t tmp;
    BYTE typedata[256] = { 0};

    if (!UI_BuildFrameForWrite(frame, &tmp, typedata, sizeof(typedata), textbuf, sizeof(textbuf))) {
        return;
    }

    gi.WriteUIFrame(&tmp);
}

void UI_WriteStart(DWORD layer) {
    gi.WriteByte(svc_layout);
    gi.WriteByte(layer);
    frameptr = framesWritten;
    layoutBytesWritten = 0;
}

/* Serialize the complete frame tree rooted at root and unicast the resulting
 * svc_layout message to ent.  layer identifies which client-side UI layer
 * (LAYER_CONSOLE, LAYER_CINEMATIC, etc.) will be replaced. */
void UI_WriteLayout(LPEDICT ent, LPCFRAMEDEF root, DWORD layer) {
    UI_WriteStart(layer);
    UI_WriteFrameWithChildren(root, NULL);
    gi.WriteLong(0); // end of list
    gi.unicast(ent);
    DIAGF("UI_WriteLayout: layer=%u root=%s bytes=%u frames=%td\n",
          (unsigned)layer,
          root ? root->Name : "<null>",
          (unsigned)layoutBytesWritten,
          frameptr - framesWritten);
}
