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
#include "../generated/main_menu.h"

/* Generated FDF frame references */
static MainMenuFdfBindings_t main_menu;
static uiDialogWar3_t quit_dialog;

/* State */
static BOOL show_realm_select = false;

static BOOL MainMenu_Load(void) {
    return MainMenuFdfBindings_Bind(&main_menu);
}

static void MainMenu_InitQuitDialog(void) {
    UI_DialogWar3Init(&quit_dialog, main_menu.MainMenuFrame, NULL);
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
    if (!main_menu.MainMenuFrame) {
        uiimport.Printf("ERROR: MainMenuFrame not found\n");
        return;
    }

    if (main_menu.RealmSelect) {
        UI_SetHidden(main_menu.RealmSelect, true);
    }
    if (main_menu.ControlLayer) {
        UI_SetHidden(main_menu.ControlLayer, false);
    }
    if (main_menu.WarCraftIIILogo) {
        DWORD logo_model = UI_LoadModel("MainMenuLogo", true);
        if (logo_model) {
            main_menu.WarCraftIIILogo->Portrait.model = logo_model;
        }
        UI_SetPoint(main_menu.WarCraftIIILogo,
                    FRAMEPOINT_TOPLEFT,
                    main_menu.MainMenuFrame,
                    FRAMEPOINT_TOPLEFT,
                    0.13f,
                    -0.08f);
    }

    UI_SetOnClick(main_menu.RealmButton, "menu /realm-select");
    UI_SetOnClick(main_menu.SinglePlayerButton, "menu /single-player");
    UI_SetOnClick(main_menu.BattleNetButton, "menu /lan/create");
    UI_SetOnClick(main_menu.LocalAreaNetworkButton, "menu /lan/create");
    UI_SetOnClick(main_menu.OptionsButton, "menu /options");
    UI_SetOnClick(main_menu.CreditsButton, "menu /credits");
    UI_SetOnClick(main_menu.ExitButton, "menu /main/quit-confirm");
    MainMenu_InitQuitDialog();

    /* Realm select buttons */
    if (main_menu.RealmSelect) {
        UI_SetOnClick(main_menu.RealmSelectOKButton, "menu /main");
        UI_SetOnClick(main_menu.RealmSelectCancelButton, "menu /main");
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
    if (main_menu.MainMenuFrame) {
        UI_DrawFrame(main_menu.MainMenuFrame);
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
        if (main_menu.RealmSelect) {
            UI_SetHidden(main_menu.RealmSelect, false);
        }
        if (main_menu.ControlLayer) {
            UI_SetHidden(main_menu.ControlLayer, true);
        }
    } else if (!strcmp(path, "/main")) {
        show_realm_select = false;
        UI_DialogWar3Hide(&quit_dialog);
        if (main_menu.RealmSelect) {
            UI_SetHidden(main_menu.RealmSelect, true);
        }
        if (main_menu.ControlLayer) {
            UI_SetHidden(main_menu.ControlLayer, false);
        }
    } else if (!strcmp(path, "/quit-confirm")) {
        MainMenu_ShowQuitDialog();
    }
}

uiScreen_t mainMenuScreen = {
    .name = "main",
    .load = MainMenu_Load,
    .init = MainMenu_Init,
    .shutdown = MainMenu_Shutdown,
    .refresh = MainMenu_Refresh,
    .draw = MainMenu_Draw,
    .key_event = MainMenu_KeyEvent,
    .mouse_event = MainMenu_MouseEvent,
    .route = MainMenu_Route,
};
