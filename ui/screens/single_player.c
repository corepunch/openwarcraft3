/*
 * ui/screens/single_player.c — Single player menu screen.
 */

#include "../ui_local.h"
#include "../ui_screen.h"

static void SinglePlayerMenu_Init(void) {
    uiimport.Printf("SinglePlayerMenu_Init\n");
}

static void SinglePlayerMenu_Shutdown(void) {
}

static void SinglePlayerMenu_Refresh(int msec) {
    (void)msec;
}

static void SinglePlayerMenu_Draw(void) {
}

static void SinglePlayerMenu_KeyEvent(int key, BOOL down) {
    (void)key;
    (void)down;
}

static void SinglePlayerMenu_MouseEvent(int x, int y, int buttons) {
    (void)x;
    (void)y;
    (void)buttons;
}

static void SinglePlayerMenu_Route(LPCSTR path) {
    (void)path;
}

uiScreen_t singlePlayerMenuScreen = {
    .name = "single-player",
    .init = SinglePlayerMenu_Init,
    .shutdown = SinglePlayerMenu_Shutdown,
    .refresh = SinglePlayerMenu_Refresh,
    .draw = SinglePlayerMenu_Draw,
    .key_event = SinglePlayerMenu_KeyEvent,
    .mouse_event = SinglePlayerMenu_MouseEvent,
    .route = SinglePlayerMenu_Route,
};
