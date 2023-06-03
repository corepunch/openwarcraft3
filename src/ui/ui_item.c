#include "ui_local.h"

DWORD UI_CommandButtonPosition(LPCSTR classname) {
    LPCSTR buttonpos = UI_FindConfigValue(classname, STR_BUTTONPOS);
    if (!buttonpos) return -1;
    DWORD x = 0, y = 0;
    sscanf(buttonpos, "%d,%d", &x, &y);
    return x + y * COMMANDBAR_COLUMNS;
}

LPCTEXTURE UI_LoadItemTexture(LPCSTR classname) {
    LPCSTR art = UI_FindConfigValue(classname, STR_ART);
    if (!art) return NULL;
    if (!strstr(art, "\\")) {
        art = imp.FindConfigValue(ui.theme, STR_DEFAULT, art);
    }
    if (!art) return NULL;
    return imp.LoadTexture(art);
}

LPCTEXTURE UI_GetItemTexture(LPCSTR classname) {
    DWORD texid = 1;
    for (; texid < MAX_ITEMS && *ui.item_textures->name; texid++) {
        if (!strcmp(classname, ui.item_textures[texid].name)) {
            return ui.item_textures[texid].texture;
        }
    }
    if (texid >= MAX_ITEMS) {
        imp.error("Item textures exceed %d\n", MAX_ITEMS);
        return NULL;
    }
    strcpy(ui.item_textures[texid].name, classname);
    return (ui.item_textures[texid].texture = UI_LoadItemTexture(classname));
}


