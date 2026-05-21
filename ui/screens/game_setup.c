/*
 * ui/screens/game_setup.c — Game setup screen with 16-player slots.
 *
 * This screen handles:
 * - Up to 16 player slots (open, closed, human, AI)
 * - Player color/race/team selection
 * - Map settings
 * - Game start
 */

#include "../ui_local.h"
#include "../ui_screen.h"

#define MAX_PLAYER_SLOTS 16

typedef struct {
    BOOL open;
    BOOL ai;
    char name[64];
    int color;
    int race;
    int team;
} playerSlot_t;

static playerSlot_t slots[MAX_PLAYER_SLOTS];

static void GameSetup_Init(void) {
    uiimport.Printf("GameSetup_Init\n");
    
    /* Initialize slots */
    for (int i = 0; i < MAX_PLAYER_SLOTS; i++) {
        slots[i].open = (i == 0); /* First slot open for host */
        slots[i].ai = false;
        slots[i].name[0] = '\0';
        slots[i].color = i;
        slots[i].race = 0; /* Random */
        slots[i].team = 0;
    }
}

static void GameSetup_Shutdown(void) {
}

static void GameSetup_Refresh(int msec) {
    (void)msec;
}

static void GameSetup_Draw(void) {
    /* Draw 16 player slots */
}

static void GameSetup_KeyEvent(int key, BOOL down) {
    (void)key;
    (void)down;
}

static void GameSetup_MouseEvent(int x, int y, int buttons) {
    (void)x;
    (void)y;
    (void)buttons;
}

static void GameSetup_Route(LPCSTR path) {
    (void)path;
}

uiScreen_t gameSetupScreen = {
    .name = "game-setup",
    .init = GameSetup_Init,
    .shutdown = GameSetup_Shutdown,
    .refresh = GameSetup_Refresh,
    .draw = GameSetup_Draw,
    .key_event = GameSetup_KeyEvent,
    .mouse_event = GameSetup_MouseEvent,
    .route = GameSetup_Route,
};
