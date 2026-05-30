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
#include "../ui_dialog.h"
#include "../ui_screen.h"

/* Frame references */
static LPFRAMEDEF frame_MainMenu = NULL;
static LPFRAMEDEF frame_ControlLayer = NULL;
static LPFRAMEDEF frame_RealmSelect = NULL;
static LPFRAMEDEF frame_Logo = NULL;
static uiDialogWar3_t quit_dialog;

/* State */
static BOOL show_realm_select = false;

static void MainMenu_InitQuitDialog(void) {
    uiDialogWar3Init_t init = {
        .modal_name = "MainMenuQuitModal",
        .cover_name = "MainMenuQuitModalCover",
        .template_name = "DialogWar3",
    };

    UI_DialogWar3Init(&quit_dialog, frame_MainMenu, &init);
}

static void MainMenu_ShowQuitDialog(void) {
    uiDialogWar3Config_t config = {
        .message = "Do you want to Quit?",
        .icon = UI_DIALOG_WAR3_ICON_QUESTION,
        .buttons = UI_DIALOG_WAR3_BUTTONS_YES_NO,
        .no_route = "menu /main/main",
        .yes_route = "menu /quit",
    };

    UI_DialogWar3Show(&quit_dialog, &config);
}

static void MainMenu_InitFrames(void) {
    /* Find top-level frames */
    UI_FRAME(MainMenuFrame);
    UI_FRAME(ControlLayer);
    UI_FRAME(RealmSelect);
    UI_FRAME(WarCraftIIILogo);

    frame_MainMenu = MainMenuFrame;
    frame_ControlLayer = ControlLayer;
    frame_RealmSelect = RealmSelect;
    frame_Logo = WarCraftIIILogo;

    if (!frame_MainMenu) {
        uiimport.Printf("ERROR: MainMenuFrame not found\n");
        return;
    }

    if (frame_RealmSelect) {
        UI_SetHidden(frame_RealmSelect, true);
    }
    if (frame_ControlLayer) {
        UI_SetHidden(frame_ControlLayer, false);
    }
    if (frame_Logo) {
        DWORD logo_model = UI_LoadModel("MainMenuLogo", true);
        if (logo_model) {
            frame_Logo->Portrait.model = logo_model;
        }
        UI_SetPoint(frame_Logo, FRAMEPOINT_TOPLEFT, frame_MainMenu, FRAMEPOINT_TOPLEFT, 0.13f, -0.08f);
    }

    /* Wire button callbacks */
    LPFRAMEDEF RealmButton = UI_FindChildFrame(frame_MainMenu, "RealmButton");
    LPFRAMEDEF SinglePlayerButton = UI_FindChildFrame(frame_MainMenu, "SinglePlayerButton");
    LPFRAMEDEF BattleNetButton = UI_FindChildFrame(frame_MainMenu, "BattleNetButton");
    LPFRAMEDEF LocalAreaNetworkButton = UI_FindChildFrame(frame_MainMenu, "LocalAreaNetworkButton");
    LPFRAMEDEF OptionsButton = UI_FindChildFrame(frame_MainMenu, "OptionsButton");
    LPFRAMEDEF CreditsButton = UI_FindChildFrame(frame_MainMenu, "CreditsButton");
    LPFRAMEDEF ExitButton = UI_FindChildFrame(frame_MainMenu, "ExitButton");

    UI_SetOnClick(RealmButton, "menu /realm-select");
    UI_SetOnClick(SinglePlayerButton, "map \"Maps\\Campaign\\Human02.w3m\"");
    UI_SetOnClick(BattleNetButton, "menu /lan/create");
    UI_SetOnClick(LocalAreaNetworkButton, "menu /lan/create");
    UI_SetOnClick(OptionsButton, "menu /options");
    UI_SetOnClick(CreditsButton, "menu /credits");
    UI_SetOnClick(ExitButton, "menu /main/quit-confirm");
    MainMenu_InitQuitDialog();

    /* Realm select buttons */
    if (frame_RealmSelect) {
        LPFRAMEDEF RealmSelectOKButton = UI_FindChildFrame(frame_RealmSelect, "RealmSelectOKButton");
        LPFRAMEDEF RealmSelectCancelButton = UI_FindChildFrame(frame_RealmSelect, "RealmSelectCancelButton");
        UI_SetOnClick(RealmSelectOKButton, "menu /main");
        UI_SetOnClick(RealmSelectCancelButton, "menu /main");
    }
}

static void MainMenu_Init(void) {
    uiimport.Printf("MainMenu_Init\n");
    UI_PreloadGlueSceneModels();
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
    UI_DrawGlueScene("MainMenu Stand");

    /* Render main menu frame tree */
    if (frame_MainMenu) {
        UI_DrawFrame(frame_MainMenu);
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
        UI_DialogWar3Hide(&quit_dialog);
        show_realm_select = true;
        if (frame_RealmSelect) {
            UI_SetHidden(frame_RealmSelect, false);
        }
        if (frame_ControlLayer) {
            UI_SetHidden(frame_ControlLayer, true);
        }
    } else if (!strcmp(path, "/main")) {
        show_realm_select = false;
        UI_DialogWar3Hide(&quit_dialog);
        if (frame_RealmSelect) {
            UI_SetHidden(frame_RealmSelect, true);
        }
        if (frame_ControlLayer) {
            UI_SetHidden(frame_ControlLayer, false);
        }
    } else if (!strcmp(path, "/quit-confirm")) {
        MainMenu_ShowQuitDialog();
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
