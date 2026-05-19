/*
 * ui/screens/main_menu.c — Main menu screen controller.
 *
 * This screen displays the main menu with buttons for:
 * - Realm Select (Battle.net)
 * - Single Player
 * - Battle.net
 * - Local Area Network
 * - Options
 * - Credits
 * - Exit
 *
 * The screen loads MainMenu.fdf frame definitions and routes onClick
 * commands to UI_MenuCommand() for navigation.
 */

#include "../ui_local.h"
#include "../ui_screen.h"

/* Frame references */
static LPFRAMEDEF frame_MainMenu = NULL;
static LPFRAMEDEF frame_ControlLayer = NULL;
static LPFRAMEDEF frame_TopLeftPanel = NULL;
static LPFRAMEDEF frame_TopRightPanel = NULL;
static LPFRAMEDEF frame_RealmSelect = NULL;
static LPFRAMEDEF frame_Logo = NULL;

/* State */
static BOOL show_realm_select = false;

static void MainMenu_InitFrames(void) {
    /* Find top-level frames */
    frame_MainMenu = UI_FindFrame("MainMenuFrame");
    frame_ControlLayer = UI_FindFrame("ControlLayer");
    frame_TopLeftPanel = UI_FindFrame("TopLeftPanel");
    frame_TopRightPanel = UI_FindFrame("TopRightPanel");
    frame_RealmSelect = UI_FindFrame("RealmSelect");
    frame_Logo = UI_FindFrame("WarCraftIIILogo");

    if (!frame_MainMenu) {
        uiimport.Printf("ERROR: MainMenuFrame not found\n");
        return;
    }

    /* Set up hierarchy */
    if (frame_TopLeftPanel) {
        UI_SetParent(frame_TopLeftPanel, frame_MainMenu);
        UI_SetHidden(frame_TopLeftPanel, false);
    }
    if (frame_TopRightPanel) {
        UI_SetParent(frame_TopRightPanel, frame_MainMenu);
        UI_SetHidden(frame_TopRightPanel, false);
    }
    if (frame_RealmSelect) {
        UI_SetParent(frame_RealmSelect, frame_MainMenu);
        UI_SetHidden(frame_RealmSelect, true);
    }
    if (frame_ControlLayer) {
        UI_SetParent(frame_ControlLayer, frame_MainMenu);
        UI_SetHidden(frame_ControlLayer, false);
    }
    if (frame_Logo && frame_ControlLayer) {
        UI_SetParent(frame_Logo, frame_ControlLayer);
        /* Logo model will be set by theme or hardcoded path */
        UI_SetPoint(frame_Logo, FRAMEPOINT_TOPLEFT, frame_MainMenu, FRAMEPOINT_TOPLEFT, 0.13f, -0.08f);
    }

    /* Wire button callbacks */
    LPFRAMEDEF RealmButton = UI_FindFrame("RealmButton");
    LPFRAMEDEF SinglePlayerButton = UI_FindFrame("SinglePlayerButton");
    LPFRAMEDEF BattleNetButton = UI_FindFrame("BattleNetButton");
    LPFRAMEDEF LANButton = UI_FindFrame("LocalAreaNetworkButton");
    LPFRAMEDEF OptionsButton = UI_FindFrame("OptionsButton");
    LPFRAMEDEF CreditsButton = UI_FindFrame("CreditsButton");
    LPFRAMEDEF ExitButton = UI_FindFrame("ExitButton");

    UI_SetOnClick(RealmButton, "menu /realm-select");
    UI_SetOnClick(SinglePlayerButton, "menu /single-player");
    UI_SetOnClick(BattleNetButton, "menu /lan/refresh");
    UI_SetOnClick(LANButton, "menu /lan/refresh");
    UI_SetOnClick(OptionsButton, "menu /options");
    UI_SetOnClick(CreditsButton, "menu /credits");
    UI_SetOnClick(ExitButton, "menu /quit");

    /* Realm select buttons */
    if (frame_RealmSelect) {
        LPFRAMEDEF OKButton = UI_FindChildFrame(frame_RealmSelect, "RealmSelectOKButton");
        LPFRAMEDEF CancelButton = UI_FindChildFrame(frame_RealmSelect, "RealmSelectCancelButton");
        UI_SetOnClick(OKButton, "menu /main");
        UI_SetOnClick(CancelButton, "menu /main");
    }
}

static void MainMenu_Init(void) {
    uiimport.Printf("MainMenu_Init\n");
    MainMenu_InitFrames();
    show_realm_select = false;
}

static void MainMenu_Shutdown(void) {
    /* Nothing to clean up */
}

static void MainMenu_Refresh(int msec) {
    (void)msec;
    /* Update animations, etc. */
}

static void MainMenu_Draw(void) {
    /* Render main menu frame tree */
    if (frame_MainMenu) {
        /* Draw root frame and children */
        /* Actual rendering happens in renderer via uiexport.DrawFrame() */
    }
}

static void MainMenu_KeyEvent(int key, BOOL down) {
    /* Handle key presses */
    (void)key;
    (void)down;
}

static void MainMenu_MouseEvent(int x, int y, int buttons) {
    /* Handle mouse events */
    (void)x;
    (void)y;
    (void)buttons;
}

static void MainMenu_Route(LPCSTR path) {
    /* Handle sub-routes */
    if (!strcmp(path, "/realm-select")) {
        show_realm_select = true;
        if (frame_RealmSelect) {
            UI_SetHidden(frame_RealmSelect, false);
        }
        if (frame_ControlLayer) {
            UI_SetHidden(frame_ControlLayer, true);
        }
    } else if (!strcmp(path, "/main")) {
        show_realm_select = false;
        if (frame_RealmSelect) {
            UI_SetHidden(frame_RealmSelect, true);
        }
        if (frame_ControlLayer) {
            UI_SetHidden(frame_ControlLayer, false);
        }
    }
}

uiScreen_t mainMenuScreen = {
    .name = "main",
    .init = MainMenu_Init,
    .shutdown = MainMenu_Shutdown,
    .refresh = MainMenu_Refresh,
    .draw = MainMenu_Draw,
    .key_event = MainMenu_KeyEvent,
    .mouse_event = MainMenu_MouseEvent,
    .route = MainMenu_Route,
};
