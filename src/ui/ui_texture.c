#include "ui_local.h"

static void DrawTexture(uiFrameDef_t const *frame) {
    uiTextureDef_t *t = frame->typedata;
    float x = (t->Anchor.corner & 1) ? 0.8 + t->Anchor.x - frame->Width : t->Anchor.x;
    float y = (t->Anchor.corner & 2) ? 0.6 + t->Anchor.y - frame->Height : t->Anchor.y;
    imp.DrawImage(t->Texture, &(const RECT) {
        .x = x,
        .y = y,
        .width = frame->Width,
        .height = frame->Height,
    }, &t->TexCoord);
}

uiFrameDef_t *UI_MakeTextureFrame(void) {
    uiFrameDef_t *frame = imp.MemAlloc(sizeof(uiFrameDef_t));
    frame->typedata = imp.MemAlloc(sizeof(uiTextureDef_t));
    frame->draw = DrawTexture;
    return frame;
}
