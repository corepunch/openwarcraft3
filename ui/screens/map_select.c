/*
 * ui/screens/map_select.c — Map selection screen.
 */

#include "../ui_local.h"
#include "../ui_screen.h"

static void MapSelect_Init(void) {
    uiimport.Printf("MapSelect_Init\n");
}

static void MapSelect_Shutdown(void) {
}

static void MapSelect_Refresh(int msec) {
    (void)msec;
}

static void MapSelect_Draw(void) {
}

static void MapSelect_KeyEvent(int key, BOOL down) {
    (void)key;
    (void)down;
}

static void MapSelect_MouseEvent(int x, int y, int buttons) {
    (void)x;
    (void)y;
    (void)buttons;
}

static void MapSelect_Route(LPCSTR path) {
    (void)path;
}

uiScreen_t mapSelectScreen = {
    .name = "map-select",
    .init = MapSelect_Init,
    .shutdown = MapSelect_Shutdown,
    .refresh = MapSelect_Refresh,
    .draw = MapSelect_Draw,
    .key_event = MapSelect_KeyEvent,
    .mouse_event = MapSelect_MouseEvent,
    .route = MapSelect_Route,
};
