/*
 * ui/screens/options_menu.c — Main-menu options screen controller.
 */

#include "../ui_local.h"
#include "../ui_screen.h"
#include "../generated/options_menu.h"

typedef enum {
    OPTIONS_PANEL_GAMEPLAY,
    OPTIONS_PANEL_VIDEO,
    OPTIONS_PANEL_SOUND,
} optionsPanel_t;

static OptionsMenu_t options_menu;
static optionsPanel_t current_panel = OPTIONS_PANEL_GAMEPLAY;

static BOOL OptionsMenu_LoadScreen(void) {
    return OptionsMenu_Load(&options_menu);
}

static void OptionsMenu_SetHidden(LPFRAMEDEF frame, BOOL hidden) {
    if (frame) {
        UI_SetHidden(frame, hidden);
    }
}

static void OptionsMenu_SetPanel(optionsPanel_t panel) {
    current_panel = panel;

    OptionsMenu_SetHidden(options_menu.GameplayPanel, panel != OPTIONS_PANEL_GAMEPLAY);
    OptionsMenu_SetHidden(options_menu.VideoPanel, panel != OPTIONS_PANEL_VIDEO);
    OptionsMenu_SetHidden(options_menu.SoundPanel, panel != OPTIONS_PANEL_SOUND);
}

static void OptionsMenu_Init(void) {
    uiimport.Printf("OptionsMenu_Init\n");
    UI_PreloadGlueSceneModels();
    current_panel = OPTIONS_PANEL_GAMEPLAY;

    UI_SetOnClick(options_menu.GameplayButton, "menu_options_gameplay");
    UI_SetOnClick(options_menu.VideoButton, "menu_video");
    UI_SetOnClick(options_menu.SoundButton, "menu_options_sound");
    UI_SetOnClick(options_menu.OKButton, "menu_main");
    UI_SetOnClick(options_menu.CancelButton, "menu_main");
    UI_SetOnClick(options_menu.ConfirmOKButton, "menu_main");
    UI_SetOnClick(options_menu.ConfirmCancelButton, "menu_options");

    OptionsMenu_SetHidden(options_menu.OptionsConfirmDialog, true);
    OptionsMenu_SetPanel(current_panel);
}

static void OptionsMenu_Shutdown(void) {
}

static void OptionsMenu_Refresh(int msec) {
    (void)msec;
}

static void OptionsMenu_Draw(void) {
    UI_DrawGlueSceneLayers("Options Stand Alternate", "Options Stand");
    if (options_menu.OptionsMenu) {
        UI_DrawFrame(options_menu.OptionsMenu);
    }
}

static void OptionsMenu_KeyEvent(int key, BOOL down) {
    (void)key;
    (void)down;
}

static void OptionsMenu_MouseEvent(int x, int y, int buttons) {
    (void)x;
    (void)y;
    (void)buttons;
}

void OptionsMenu_ShowGameplay(void) {
    OptionsMenu_SetPanel(OPTIONS_PANEL_GAMEPLAY);
}

void OptionsMenu_ShowVideo(void) {
    OptionsMenu_SetPanel(OPTIONS_PANEL_VIDEO);
}

void OptionsMenu_ShowSound(void) {
    OptionsMenu_SetPanel(OPTIONS_PANEL_SOUND);
}

void OptionsMenu_ShowKeys(void) {
    OptionsMenu_SetPanel(OPTIONS_PANEL_GAMEPLAY);
}

uiScreen_t optionsMenuScreen = {
    .name = "options",
    .load = OptionsMenu_LoadScreen,
    .init = OptionsMenu_Init,
    .shutdown = OptionsMenu_Shutdown,
    .refresh = OptionsMenu_Refresh,
    .draw = OptionsMenu_Draw,
    .key_event = OptionsMenu_KeyEvent,
    .mouse_event = OptionsMenu_MouseEvent,
};
