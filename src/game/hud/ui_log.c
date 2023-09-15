#include "g_local.h"

#define PRINT_POSITION 0.0500, -0.3000

void ui_print_text(LPGAMECLIENT client, LPCSTR message) {
    FRAMEDEF text;
    UI_InitFrame(&text, FT_TEXT);
    LPCSTR fontfile = Theme_String("MessageFont", "Default");
    FLOAT fontheight = Theme_Float("WorldFrameMessage", "FontHeights");
    text.Font.Index = gi.FontIndex(fontfile, fontheight * 1000);
    text.Text = message;
    UI_SetPoint(&text, FRAMEPOINT_TOPLEFT, NULL, FRAMEPOINT_TOPLEFT, PRINT_POSITION);
    UI_WriteFrame(&text);
}

void UI_ShowText(LPEDICT ent, LPCVECTOR2 pos, LPCSTR text, FLOAT duration) {
    UI_WRITE_LAYER(ent, ui_print_text, LAYER_MESSAGE, G_LevelString(text));
}
