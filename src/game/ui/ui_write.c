#include "../g_local.h"

#define SPRINTF_ADD(TEXT, ...) \
snprintf(TEXT+strlen(TEXT), sizeof(UINAME), __VA_ARGS__);

#define CONVERT_UV(DEST, SRC) \
DEST[0] = SRC.min.x * 0xff; \
DEST[1] = SRC.max.x * 0xff; \
DEST[2] = SRC.min.y * 0xff; \
DEST[3] = SRC.max.y * 0xff;

void UI_CopyFrameBase(LPUIFRAME dest, LPCFRAMEDEF src) {
    memcpy(&dest->points, &src->Points, sizeof(src->Points));
    CONVERT_UV(dest->tex.coord, src->Texture.TexCoord);
    dest->number = src->Number;
    dest->parent = src->Parent ? src->Parent->Number : 0;
    dest->color = src->Color;
    dest->size.width = src->Width;
    dest->size.height = src->Height;
    dest->tex.index = src->Texture.Image;
    dest->tex.index2 = src->Texture.Image2;
    dest->flags.type = src->Type;
    dest->flags.alphaMode = src->AlphaMode;
    dest->flags.textalignx = src->Font.Justification.Horizontal;
    dest->flags.textaligny = src->Font.Justification.Vertical;
    dest->font.index = src->Font.Index;
    dest->textLength = src->TextLength;
    dest->stat = src->Stat;
    dest->text = src->Text;
    dest->tooltip = src->Tooltip;
}

static void WriteBackdrop(LPCFRAMEDEF frame, LPSTR buffer) {
    SPRINTF_ADD(buffer, "%d,", frame->Backdrop.CornerFlags);
    SPRINTF_ADD(buffer, "%d,", frame->Backdrop.TileBackground);
    SPRINTF_ADD(buffer, "%d,", frame->Backdrop.Background);
    SPRINTF_ADD(buffer, "%d,", frame->Backdrop.CornerSize);
    SPRINTF_ADD(buffer, "%d,", frame->Backdrop.BackgroundSize);
    SPRINTF_ADD(buffer, "%d ", frame->Backdrop.BackgroundInsets[0]);
    SPRINTF_ADD(buffer, "%d ", frame->Backdrop.BackgroundInsets[1]);
    SPRINTF_ADD(buffer, "%d ", frame->Backdrop.BackgroundInsets[2]);
    SPRINTF_ADD(buffer, "%d,", frame->Backdrop.BackgroundInsets[3]);
    SPRINTF_ADD(buffer, "%d,", frame->Backdrop.EdgeFile);
    SPRINTF_ADD(buffer, "%d,", frame->Backdrop.BlendAll);
}

static void WriteButton(LPCFRAMEDEF frame, LPSTR buffer) {
    LPCFRAMEDEF NormalTexture = UI_FindFrame(frame->Button.NormalTexture);
    LPCFRAMEDEF PushedTexture = UI_FindFrame(frame->Button.PushedTexture);
    LPCFRAMEDEF DisabledTexture = UI_FindFrame(frame->Button.DisabledTexture);
    LPCFRAMEDEF UseHighlight = UI_FindFrame(frame->Button.UseHighlight);

    LPCFRAMEDEF NormalText = UI_FindFrame(frame->Button.NormalText.frame);
    LPCFRAMEDEF DisabledText = UI_FindFrame(frame->Button.DisabledText.frame);
    LPCFRAMEDEF HighlightText = UI_FindFrame(frame->Button.HighlightText.frame);

    BYTE NormalTexCoord[4];
    BYTE PushedTexCoord[4];
    BYTE DisabledTexCoord[4];
    BYTE HighlightTexCoord[4];
    
    CONVERT_UV(NormalTexCoord, NormalTexture->Texture.TexCoord);
    CONVERT_UV(PushedTexCoord, PushedTexture->Texture.TexCoord);
    CONVERT_UV(DisabledTexCoord, DisabledTexture->Texture.TexCoord);
    CONVERT_UV(HighlightTexCoord, UseHighlight->Texture.TexCoord);
    
    SPRINTF_ADD(buffer, "%02x", NormalTexture->Texture.Image);
    SPRINTF_ADD(buffer, "%08x", *(LPDWORD)NormalTexCoord);
    SPRINTF_ADD(buffer, "%02x", NormalText->Font.Index);
    SPRINTF_ADD(buffer, "%08x", *(LPDWORD)&NormalText->Font.Color);
    SPRINTF_ADD(buffer, "%s", UI_GetString(frame->Button.NormalText.text));
}

void UI_WriteFrame(LPCFRAMEDEF frame) {
    UINAME buffer;
    uiFrame_t tmp;
    memset(&tmp, 0, sizeof(tmp));
    memset(&buffer, 0, sizeof(buffer));
    UI_CopyFrameBase(&tmp, frame);
    switch (frame->Type) {
        case FT_BACKDROP:
        case FT_TOOLTIPTEXT:
            WriteBackdrop(frame, buffer);
            tmp.text = buffer;
            break;
        case FT_SIMPLEBUTTON:
            WriteButton(frame, buffer);
            tmp.text = buffer;
            break;
//        case FT_SIMPLESTATUSBAR:
//            break;
        case FT_TEXT:
        case FT_STRING:
            tmp.color = frame->Font.Color;
            if (*frame->Text == '\0') {
                tmp.text = frame->Name;
            }
            break;
        case FT_PORTRAIT:
            tmp.tex.index = frame->Portrait.model;
            break;
        default:
            break;
    }
    tmp.flags.type = frame->Type;
    gi.WriteUIFrame(&tmp);
}

void UI_WriteLayout(LPEDICT ent, LPCFRAMEDEF root, DWORD layer) {
    gi.WriteByte(svc_layout);
    gi.WriteByte(layer);
    UI_WriteFrameWithChildren(root);
    gi.WriteLong(0); // end of list
    gi.unicast(ent);
}

//void UI_WRITE_LAYER(LPEDICT ent,
//                     void (*BuildUI)(LPGAMECLIENT),
//                     DWORD layer)
//{
//    gi.WriteByte(svc_layout);
//    gi.WriteByte(layer);
//    BuildUI(ent->client);
//    gi.WriteLong(0); // end of list
//    gi.unicast(ent);
//}

