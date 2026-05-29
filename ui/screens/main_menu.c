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
static LPFRAMEDEF frame_RealmSelect = NULL;
static LPFRAMEDEF frame_Logo = NULL;
static LPFRAMEDEF frame_QuitModal = NULL;
static LPFRAMEDEF frame_QuitDialog = NULL;
static LPFRAMEDEF frame_QuitDialogText = NULL;
static LPFRAMEDEF frame_QuitDialogIcon = NULL;
static LPFRAMEDEF frame_QuitDialogOkBackdrop = NULL;
static LPFRAMEDEF frame_QuitDialogNoBackdrop = NULL;
static LPFRAMEDEF frame_QuitDialogYesBackdrop = NULL;
static LPFRAMEDEF frame_QuitDialogNoButton = NULL;
static LPFRAMEDEF frame_QuitDialogYesButton = NULL;

/* State */
static BOOL show_realm_select = false;

static void MainMenu_SetQuitModalVisible(BOOL visible) {
    if (frame_QuitModal) {
        UI_SetHidden(frame_QuitModal, !visible);
    }
    if (frame_QuitDialog) {
        UI_SetHidden(frame_QuitDialog, !visible);
    }
}

static void MainMenu_InitQuitDialog(void) {
    LPFRAMEDEF template_dialog;
    LPFRAMEDEF cover;

    frame_QuitModal = UI_FindFrame("MainMenuQuitModal");
    if (!frame_QuitModal) {
        frame_QuitModal = UI_Spawn(FT_DIALOG, frame_MainMenu);
        if (!frame_QuitModal) {
            uiimport.Printf("ERROR: failed to create MainMenuQuitModal\n");
            return;
        }
        snprintf(frame_QuitModal->Name, sizeof(frame_QuitModal->Name), "%s", "MainMenuQuitModal");
        UI_SetPoint(frame_QuitModal, FRAMEPOINT_TOPLEFT, frame_MainMenu, FRAMEPOINT_TOPLEFT, 0.0f, 0.0f);
        UI_SetPoint(frame_QuitModal, FRAMEPOINT_BOTTOMRIGHT, frame_MainMenu, FRAMEPOINT_BOTTOMRIGHT, 0.0f, 0.0f);

        /* DialogWar3 provides the frame; the black cover is code-owned like WarSmash's modal veil. */
        cover = UI_Spawn(FT_TEXTURE, frame_QuitModal);
        if (cover) {
            snprintf(cover->Name, sizeof(cover->Name), "%s", "MainMenuQuitModalCover");
            UI_SetPoint(cover, FRAMEPOINT_TOPLEFT, frame_QuitModal, FRAMEPOINT_TOPLEFT, 0.0f, 0.0f);
            UI_SetPoint(cover, FRAMEPOINT_BOTTOMRIGHT, frame_QuitModal, FRAMEPOINT_BOTTOMRIGHT, 0.0f, 0.0f);
            cover->Texture.Image = UI_LoadTexture("Textures\\Black32.blp", false);
            cover->Color = MAKE(COLOR32, 255, 255, 255, 128);
        }
    } else {
        UI_SetParent(frame_QuitModal, frame_MainMenu);
    }

    frame_QuitDialog = UI_FindChildFrame(frame_QuitModal, "DialogWar3");
    if (!frame_QuitDialog) {
        template_dialog = UI_FindFrame("DialogWar3");
        if (!template_dialog) {
            uiimport.Printf("ERROR: DialogWar3 not found\n");
            MainMenu_SetQuitModalVisible(false);
            return;
        }
        frame_QuitDialog = UI_CloneFrameTree(template_dialog, frame_QuitModal);
        if (!frame_QuitDialog) {
            uiimport.Printf("ERROR: failed to clone DialogWar3\n");
            MainMenu_SetQuitModalVisible(false);
            return;
        }
    }
    UI_SetParent(frame_QuitDialog, frame_QuitModal);
    UI_SetPoint(frame_QuitDialog, FRAMEPOINT_CENTER, frame_QuitModal, FRAMEPOINT_CENTER, 0.0f, 0.0f);

    frame_QuitDialogText = UI_FindChildFrame(frame_QuitDialog, "DialogText");
    frame_QuitDialogIcon = UI_FindChildFrame(frame_QuitDialog, "DialogIcon");
    frame_QuitDialogOkBackdrop = UI_FindChildFrame(frame_QuitDialog, "DialogButtonOKBackdrop");
    frame_QuitDialogNoBackdrop = UI_FindChildFrame(frame_QuitDialog, "DialogButtonNoBackdrop");
    frame_QuitDialogYesBackdrop = UI_FindChildFrame(frame_QuitDialog, "DialogButtonYesBackdrop");
    frame_QuitDialogNoButton = UI_FindChildFrame(frame_QuitDialog, "DialogButtonNo");
    frame_QuitDialogYesButton = UI_FindChildFrame(frame_QuitDialog, "DialogButtonYes");

    if (frame_QuitDialogText) {
        UI_SetText(frame_QuitDialogText, "CONFIRM_EXIT_MESSAGE");
    }
    if (frame_QuitDialogIcon) {
        frame_QuitDialogIcon->Backdrop.Background = UI_LoadTexture("UI\\Widgets\\Glues\\dialogbox-question.blp", false);
    }
    UI_SetHidden(frame_QuitDialogOkBackdrop, true);
    UI_SetHidden(frame_QuitDialogNoBackdrop, false);
    UI_SetHidden(frame_QuitDialogYesBackdrop, false);
    if (frame_QuitDialogNoButton) {
        UI_SetOnClick(frame_QuitDialogNoButton, "menu /main/main");
    }
    if (frame_QuitDialogYesButton) {
        UI_SetOnClick(frame_QuitDialogYesButton, "menu /quit");
    }
    MainMenu_SetQuitModalVisible(false);
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
    UI_FRAME(RealmButton);
    UI_FRAME(SinglePlayerButton);
    UI_FRAME(BattleNetButton);
    UI_FRAME(LocalAreaNetworkButton);
    UI_FRAME(OptionsButton);
    UI_FRAME(CreditsButton);
    UI_FRAME(ExitButton);

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
        MainMenu_SetQuitModalVisible(false);
        show_realm_select = true;
        if (frame_RealmSelect) {
            UI_SetHidden(frame_RealmSelect, false);
        }
        if (frame_ControlLayer) {
            UI_SetHidden(frame_ControlLayer, true);
        }
    } else if (!strcmp(path, "/main")) {
        show_realm_select = false;
        MainMenu_SetQuitModalVisible(false);
        if (frame_RealmSelect) {
            UI_SetHidden(frame_RealmSelect, true);
        }
        if (frame_ControlLayer) {
            UI_SetHidden(frame_ControlLayer, false);
        }
    } else if (!strcmp(path, "/quit-confirm")) {
        MainMenu_SetQuitModalVisible(true);
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
