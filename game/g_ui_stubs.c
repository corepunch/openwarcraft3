/*
 * g_ui_stubs.c — Server-side UI stubs (Phase 8 cleanup)
 *
 * These are no-op stubs for legacy server-side UI functions.
 * All UI rendering is now handled client-side by the UI library.
 */

#include "g_local.h"

/* Legacy command card refresh - now handled client-side */
void Get_Commands_f(LPEDICT ent) {
    (void)ent;
    /* Command card updates now triggered by clc_request_unit_ui */
}

/* Legacy portrait refresh - now handled client-side */
void Get_Portrait_f(LPEDICT ent) {
    (void)ent;
    /* Portrait rendering is client-side */
}

/* Legacy quest UI - now handled client-side */
void UI_ShowQuests(LPEDICT ent) {
    (void)ent;
}

void UI_ShowQuest(LPEDICT ent, LPCQUEST quest) {
    (void)ent;
    (void)quest;
}

void UI_HideQuests(LPEDICT ent) {
    (void)ent;
}

/* Legacy UI initialization - now handled client-side */
void UI_Init(void) {
    /* UI initialization moved to client-side UI library */
}

/* Legacy menu display functions - now handled client-side */
void UI_ShowInterface(LPEDICT ent, BOOL show, FLOAT duration) {
    (void)ent;
    (void)show;
    (void)duration;
}

void UI_ShowMainMenu(LPEDICT ent) {
    (void)ent;
}

void UI_ShowGameInterface(LPEDICT ent) {
    (void)ent;
}

void UI_ShowText(LPEDICT ent, LPCVECTOR2 pos, LPCSTR text, FLOAT duration) {
    (void)ent;
    (void)pos;
    (void)text;
    (void)duration;
}

/* Legacy command button helpers */
void UI_AddCancelButton(LPEDICT ent) {
    (void)ent;
}

void UI_AddCommandButton(LPCSTR code) {
    (void)code;
}

void UI_AddCommandButtonExtended(LPCSTR code, BOOL research, DWORD level) {
    (void)code;
    (void)research;
    (void)level;
}

void UI_ClearCreateGameSlots(void) {
}

void UI_AddCreateGameSlot(DWORD slot, LPCSTR name, LPCSTR race, LPCSTR color, DWORD team) {
    (void)slot;
    (void)name;
    (void)race;
    (void)color;
    (void)team;
}

/* Legacy texture/theme functions */
DWORD UI_LoadTexture(LPCSTR path, BOOL forcewrap) {
    (void)path;
    (void)forcewrap;
    return 0;
}

LPCSTR Theme_String(LPCSTR key, LPCSTR def) {
    (void)key;
    return def;
}

FLOAT Theme_Float(LPCSTR key, LPCSTR def) {
    (void)key;
    return def ? atof(def) : 0.0f;
}

/* Legacy UI write functions */
void UI_WriteStart(DWORD layer) {
    (void)layer;
}
