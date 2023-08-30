#include "../g_local.h"

#define SPRINTF_ADD(TEXT, ...) snprintf(TEXT+strlen(TEXT), sizeof(TEXT), __VA_ARGS__);

void UI_CopyFrameBase(LPUIFRAME dest, LPCFRAMEDEF src) {
    memcpy(&dest->points, &src->Points, sizeof(src->Points));
    dest->number = src->Number;
    dest->parent = src->Parent ? src->Parent->Number : 0;
    dest->color = src->Color;
    dest->size.width = src->Width;
    dest->size.height = src->Height;
    dest->tex.index = src->Texture.Image;
    dest->tex.index2 = src->Texture.Image2;
    dest->tex.coord[0] = src->UVs.min.x * 0xff;
    dest->tex.coord[1] = src->UVs.max.x * 0xff;
    dest->tex.coord[2] = src->UVs.min.y * 0xff;
    dest->tex.coord[3] = src->UVs.max.y * 0xff;
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

void UI_WriteFrame(LPCFRAMEDEF frame) {
    UINAME backdrop;
    uiFrame_t tmp;
    memset(&tmp, 0, sizeof(tmp));
    UI_CopyFrameBase(&tmp, frame);
    switch (frame->Type) {
        case FT_BACKDROP:
        case FT_TOOLTIPTEXT:
            memset(&backdrop, 0, sizeof(backdrop));
            SPRINTF_ADD(backdrop, "%d,", frame->Backdrop.CornerFlags);
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
            break;
//        case FT_SIMPLESTATUSBAR:
//            break;
        case FT_TEXT:
            tmp.color = frame->Font.Color;
            break;
        case FT_PORTRAIT:
            tmp.tex.index = frame->Portrait.model;
            break;
    }
    tmp.flags.type = frame->Type;
    gi.WriteUIFrame(&tmp);
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
