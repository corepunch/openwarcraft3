/*
 * ui/screens/lan_create.c — LAN multiplayer create game screen.
 */

#include "../ui_local.h"
#include "../ui_screen.h"

static void LANCreate_Init(void) {
    uiimport.Printf("LANCreate_Init\n");
}

static void LANCreate_Shutdown(void) {
}

static void LANCreate_Refresh(int msec) {
    (void)msec;
}

static void LANCreate_Draw(void) {
}

static void LANCreate_KeyEvent(int key, BOOL down) {
    (void)key;
    (void)down;
}

static void LANCreate_MouseEvent(int x, int y, int buttons) {
    (void)x;
    (void)y;
    (void)buttons;
}

static void LANCreate_Route(LPCSTR path) {
    (void)path;
}

uiScreen_t lanCreateScreen = {
    .name = "lan-create",
    .init = LANCreate_Init,
    .shutdown = LANCreate_Shutdown,
    .refresh = LANCreate_Refresh,
    .draw = LANCreate_Draw,
    .key_event = LANCreate_KeyEvent,
    .mouse_event = LANCreate_MouseEvent,
    .route = LANCreate_Route,
};
