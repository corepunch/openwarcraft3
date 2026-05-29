/*
 * ui/screens/quit_confirm.c — Main-menu quit confirmation screen.
 *
 * This reuses the existing Esc menu confirm-quit panel so the quit flow is
 * modal and consistent with the in-game confirm dialog styling.
 */

#include "../ui_local.h"
#include "../ui_screen.h"

static LPFRAMEDEF frame_root = NULL;
static LPFRAMEDEF frame_main_panel = NULL;
static LPFRAMEDEF frame_end_game_panel = NULL;
static LPFRAMEDEF frame_confirm_panel = NULL;
static LPFRAMEDEF frame_help_panel = NULL;
static LPFRAMEDEF frame_tips_panel = NULL;
static LPFRAMEDEF frame_quit_button = NULL;
static LPFRAMEDEF frame_cancel_button = NULL;

static void QuitConfirm_UpdateVisibility(void) {
    if (frame_root) {
        UI_SetHidden(frame_root, false);
    }
    if (frame_main_panel) {
        UI_SetHidden(frame_main_panel, true);
    }
    if (frame_end_game_panel) {
        UI_SetHidden(frame_end_game_panel, true);
    }
    if (frame_help_panel) {
        UI_SetHidden(frame_help_panel, true);
    }
    if (frame_tips_panel) {
        UI_SetHidden(frame_tips_panel, true);
    }
    if (frame_confirm_panel) {
        UI_SetHidden(frame_confirm_panel, false);
    }
}

static void QuitConfirm_Init(void) {
    uiimport.Printf("QuitConfirm_Init\n");
    UI_PreloadGlueSceneModels();

    UI_FRAME(EscMenuMainPanel);
    UI_FRAME(MainPanel);
    UI_FRAME(EndGamePanel);
    UI_FRAME(ConfirmQuitPanel);
    UI_FRAME(HelpPanel);
    UI_FRAME(TipsPanel);
    UI_FRAME(ConfirmQuitQuitButton);
    UI_FRAME(ConfirmQuitCancelButton);

    frame_root = EscMenuMainPanel;
    frame_main_panel = MainPanel;
    frame_end_game_panel = EndGamePanel;
    frame_confirm_panel = ConfirmQuitPanel;
    frame_help_panel = HelpPanel;
    frame_tips_panel = TipsPanel;
    frame_quit_button = ConfirmQuitQuitButton;
    frame_cancel_button = ConfirmQuitCancelButton;

    if (!frame_root) {
        uiimport.Printf("ERROR: EscMenuMainPanel not found\n");
        return;
    }
    if (!frame_confirm_panel) {
        uiimport.Printf("ERROR: ConfirmQuitPanel not found\n");
        return;
    }

    if (frame_quit_button) {
        UI_SetOnClick(frame_quit_button, "menu /quit");
    }
    if (frame_cancel_button) {
        UI_SetOnClick(frame_cancel_button, "menu /back");
    }

    QuitConfirm_UpdateVisibility();
}

static void QuitConfirm_Shutdown(void) {
    if (frame_root) {
        UI_SetHidden(frame_root, true);
    }
    if (frame_confirm_panel) {
        UI_SetHidden(frame_confirm_panel, true);
    }
}

static void QuitConfirm_Refresh(int msec) {
    (void)msec;
}

static void QuitConfirm_Draw(void) {
    UI_DrawGlueScene("MainMenu Stand");
    if (frame_root) {
        UI_DrawFrame(frame_root);
    }
}

static void QuitConfirm_KeyEvent(int key, BOOL down) {
    if (down && key == 27) {
        UI_Route("/back");
    }
}

static void QuitConfirm_MouseEvent(int x, int y, int buttons) {
    (void)x;
    (void)y;
    (void)buttons;
}

static void QuitConfirm_Route(LPCSTR path) {
    (void)path;
}

uiScreen_t quitConfirmScreen = {
    .name = "quit-confirm",
    .init = QuitConfirm_Init,
    .shutdown = QuitConfirm_Shutdown,
    .refresh = QuitConfirm_Refresh,
    .draw = QuitConfirm_Draw,
    .key_event = QuitConfirm_KeyEvent,
    .mouse_event = QuitConfirm_MouseEvent,
    .route = QuitConfirm_Route,
};
