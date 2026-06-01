/*
 * ui/screens/credits_menu.c — Main-menu credits screen controller.
 */

#include "../ui_local.h"
#include "../ui_screen.h"
#include "../generated/credits_menu.h"

static CreditsMenu_t credits_menu;

static char const credits_menu_fdf[] =
    "IncludeFile \"UI\\FrameDef\\Glue\\StandardTemplates.fdf\",\n"
    "Frame \"FRAME\" \"CreditsMenu\" INHERITS \"StandardFrameTemplate\" {\n"
    "    SetAllPoints,\n"
    "    Frame \"FRAME\" \"CreditsControlLayer\" {\n"
    "        SetAllPoints,\n"
    "        Frame \"BACKDROP\" \"CreditsPanelBackdrop\" INHERITS \"StandardHeavyBackdropTemplate\" {\n"
    "            Width 0.58,\n"
    "            Height 0.31,\n"
    "            SetPoint CENTER, \"CreditsMenu\", CENTER, 0.0, 0.025,\n"
    "        }\n"
    "        Frame \"TEXT\" \"CreditsTitleText\" INHERITS \"StandardTitleTextTemplate\" {\n"
    "            SetPoint TOPLEFT, \"CreditsPanelBackdrop\", TOPLEFT, 0.035, -0.035,\n"
    "            FontColor 1.0 1.0 1.0 1.0,\n"
    "            Text \"KEY_CREDITS\",\n"
    "        }\n"
    "        Frame \"TEXT\" \"CreditsSubtitleText\" INHERITS \"StandardInfoTextTemplate\" {\n"
    "            Width 0.51,\n"
    "            SetPoint TOPLEFT, \"CreditsTitleText\", BOTTOMLEFT, 0.0, -0.035,\n"
    "            FontJustificationH JUSTIFYLEFT,\n"
    "            Text \"OpenWarcraft3\",\n"
    "        }\n"
    "        Frame \"TEXT\" \"CreditsBodyText\" INHERITS \"StandardInfoTextTemplate\" {\n"
    "            Width 0.51,\n"
    "            SetPoint TOPLEFT, \"CreditsSubtitleText\", BOTTOMLEFT, 0.0, -0.018,\n"
    "            FontJustificationH JUSTIFYLEFT,\n"
    "            Text \"A Quake-style RTS runtime inspired by Warcraft III data.\",\n"
    "        }\n"
    "        Frame \"TEXT\" \"CreditsFooterText\" INHERITS \"StandardInfoTextTemplate\" {\n"
    "            Width 0.51,\n"
    "            SetPoint TOPLEFT, \"CreditsBodyText\", BOTTOMLEFT, 0.0, -0.018,\n"
    "            FontJustificationH JUSTIFYLEFT,\n"
    "            Text \"Original Warcraft III assets and names belong to Blizzard Entertainment.\",\n"
    "        }\n"
    "        Frame \"BACKDROP\" \"CreditsBackBackdrop\" INHERITS \"StandardMenuButtonBaseBackdrop\" {\n"
    "            Width 0.205,\n"
    "            SetPoint BOTTOMRIGHT, \"CreditsPanelBackdrop\", BOTTOMRIGHT, -0.025, 0.025,\n"
    "            Frame \"GLUETEXTBUTTON\" \"CreditsBackButton\" INHERITS WITHCHILDREN \"StandardButtonTemplate\" {\n"
    "                Width 0.145,\n"
    "                SetPoint TOPRIGHT, \"CreditsBackBackdrop\", TOPRIGHT, -0.012, -0.0165,\n"
    "                ControlShortcutKey \"B\",\n"
    "                ButtonText \"CreditsBackButtonText\",\n"
    "                Frame \"TEXT\" \"CreditsBackButtonText\" INHERITS \"StandardButtonTextTemplate\" {\n"
    "                    Text \"Back\",\n"
    "                }\n"
    "            }\n"
    "        }\n"
    "    }\n"
    "}\n";

static BOOL CreditsMenu_LoadScreen(void) {
    if (!UI_FindFrame("CreditsMenu")) {
        DWORD size = (DWORD)strlen(credits_menu_fdf) + 1;
        LPSTR buffer;

        if (!uiimport.MemAlloc || !uiimport.MemFree) {
            return false;
        }
        buffer = uiimport.MemAlloc(size);
        if (!buffer) {
            return false;
        }
        memcpy(buffer, credits_menu_fdf, size);
        UI_ParseFDF_Buffer("UI\\FrameDef\\Glue\\CreditsMenu.fdf", buffer);
        uiimport.MemFree(buffer);
    }
    return CreditsMenu_Load(&credits_menu);
}

static void CreditsMenu_Init(void) {
    uiimport.Printf("CreditsMenu_Init\n");
    UI_PreloadGlueSceneModels();
    UI_SetOnClick(credits_menu.CreditsBackButton, "menu_main");
}

static void CreditsMenu_Shutdown(void) {
}

static void CreditsMenu_Refresh(int msec) {
    (void)msec;
}

static void CreditsMenu_Draw(void) {
    UI_DrawGlueScene("MainMenu Stand");
    if (credits_menu.CreditsMenu) {
        UI_DrawFrame(credits_menu.CreditsMenu);
    }
}

static void CreditsMenu_KeyEvent(int key, BOOL down) {
    (void)key;
    (void)down;
}

static void CreditsMenu_MouseEvent(int x, int y, int buttons) {
    (void)x;
    (void)y;
    (void)buttons;
}

uiScreen_t creditsMenuScreen = {
    .name = "credits",
    .load = CreditsMenu_LoadScreen,
    .init = CreditsMenu_Init,
    .shutdown = CreditsMenu_Shutdown,
    .refresh = CreditsMenu_Refresh,
    .draw = CreditsMenu_Draw,
    .key_event = CreditsMenu_KeyEvent,
    .mouse_event = CreditsMenu_MouseEvent,
};
