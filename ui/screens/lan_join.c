/*
 * ui/screens/lan_join.c — LAN multiplayer join screen.
 */

#include "../ui_local.h"
#include "../ui_screen.h"

static void LANJoin_Init(void) {
    uiimport.Printf("LANJoin_Init\n");
    /* Request game list from server */
    uiimport.RequestGameList();
}

static void LANJoin_Shutdown(void) {
}

static void LANJoin_Refresh(int msec) {
    (void)msec;
}

static void LANJoin_Draw(void) {
}

static void LANJoin_KeyEvent(int key, BOOL down) {
    (void)key;
    (void)down;
}

static void LANJoin_MouseEvent(int x, int y, int buttons) {
    (void)x;
    (void)y;
    (void)buttons;
}

static void LANJoin_Route(LPCSTR path) {
    (void)path;
}

uiScreen_t lanJoinScreen = {
    .name = "lan",
    .init = LANJoin_Init,
    .shutdown = LANJoin_Shutdown,
    .refresh = LANJoin_Refresh,
    .draw = LANJoin_Draw,
    .key_event = LANJoin_KeyEvent,
    .mouse_event = LANJoin_MouseEvent,
    .route = LANJoin_Route,
};
