#include "ui_local.h"

static void CommandButton_Draw(uiFrameDef_t const *frame) {
    RECT const uv = { 0, 0, 1, 1 };
    uiCommandButton_t *cb = frame->typedata;
    if (!cb->Texture)
        return;
    imp.DrawImage(cb->Texture, &frame->rect, &uv);
}

static bool CommandButton_LeftMouseDown(uiFrameDef_t *frame) {
    frame->Width = COMMAND_BUTTON_SIZE * 0.95;
    frame->Height = COMMAND_BUTTON_SIZE * 0.95;
    return true;
}

static bool CommandButton_LeftMouseUp(uiFrameDef_t *frame) {
    frame->Width = COMMAND_BUTTON_SIZE;
    frame->Height = COMMAND_BUTTON_SIZE;
    uiCommandButton_t *cb = frame->typedata;
    if (cb->click) {
        cb->click(cb);
    }
    return true;
}

void CommandButton_Cancel(uiCommandButton_t const *cmd) {
    CommandBar_SetMode(CBAR_SHOW_ABILITIES);
}

void CommandButton_SelectTarget(uiCommandButton_t const *cmd) {
    CommandBar_SetMode(CBAR_SELECT_TARGET);
}

uiFrameDef_t *UI_MakeCommandButton(void) {
    uiFrameDef_t *frame = imp.MemAlloc(sizeof(uiFrameDef_t));
    frame->typedata = imp.MemAlloc(sizeof(uiCommandButton_t));
    frame->draw = CommandButton_Draw;
    frame->mouseHandler[UI_LEFT_MOUSE_DOWN] = CommandButton_LeftMouseDown;
    frame->mouseHandler[UI_LEFT_MOUSE_UP] = CommandButton_LeftMouseUp;
    return frame;
}

DWORD UI_CommandButtonPosition(BYTE item) {
    LPCSTR classname = imp.GetConfigString(CS_ITEMS + item);
    if (!classname) return -1;
    LPCSTR buttonpos = UI_FindConfigValue(classname, "buttonpos");
    if (!buttonpos) return -1;
    DWORD x = 0, y = 0;
    sscanf(buttonpos, "%d,%d", &x, &y);
    return x + y * COMMANDBAR_COLUMNS;
}

LPCTEXTURE UI_LoadItemTexture(BYTE item) {
    LPCSTR classname = imp.GetConfigString(CS_ITEMS + item);
    if (!classname) return NULL;
    LPCSTR art = UI_FindConfigValue(classname, "art");
    if (!art) return NULL;
    if (!strstr(art, "\\")) {
        art = imp.FindConfigValue(ui.theme, "Default", art);
    }
    if (!art) return NULL;
    return imp.LoadTexture(art);
}

LPCTEXTURE UI_GetItemTexture(BYTE item) {
    if (ui.item_textures[item]) {
        return ui.item_textures[item];
    } else {
        return (ui.item_textures[item] = UI_LoadItemTexture(item));
    }
}

