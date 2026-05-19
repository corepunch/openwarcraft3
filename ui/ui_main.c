/*
 * ui_main.c — UI library entry point and lifecycle management.
 */

#include "ui_local.h"
#include "ui_screen.h"

/* Global import table filled by UI_GetAPI */
uiImport_t uiimport;

/* Internal state */
typedef struct {
    BOOL initialized;
    BOOL active;
    DWORD time;
} uiState_t;

static uiState_t ui_state;

void UI_InitLocal(void) {
    memset(&ui_state, 0, sizeof(ui_state));
    
    uiimport.Printf("UI_InitLocal: loading FDF assets\n");
    
    /* Load core FDF files */
    UI_ParseFDF("UI\\FrameDef\\UI\\EscMenuTemplates.fdf");
    UI_ParseFDF("UI\\FrameDef\\UI\\EscMenuMainPanel.fdf");
    UI_ParseFDF("UI\\FrameDef\\Glue\\StandardTemplates.fdf");
    UI_ParseFDF("UI\\FrameDef\\Glue\\MainMenu.fdf");
    
    ui_state.initialized = true;
    ui_state.active = true;
    
    /* Route to main menu */
    UI_Route("/main");
}

void UI_ShutdownLocal(void) {
    UI_ClearTemplates();
    memset(&ui_state, 0, sizeof(ui_state));
}

void UI_RefreshLocal(DWORD msec) {
    if (!ui_state.active) {
        return;
    }
    
    ui_state.time += msec;
    
    /* Call current screen refresh */
    uiScreen_t *screen = UI_GetCurrentScreen();
    if (screen && screen->refresh) {
        screen->refresh((int)msec);
    }
}

void UI_DrawFrameLocal(void) {
    if (!ui_state.active) {
        return;
    }
    
    /* Call current screen draw */
    uiScreen_t *screen = UI_GetCurrentScreen();
    if (screen && screen->draw) {
        screen->draw();
    }
}

void UI_KeyEventLocal(int key, BOOL down, DWORD time) {
    (void)time;
    if (!ui_state.active) {
        return;
    }
    
    /* Delegate to current screen */
    uiScreen_t *screen = UI_GetCurrentScreen();
    if (screen && screen->key_event) {
        screen->key_event(key, down);
    }
}

void UI_MouseEventLocal(int x, int y, int button, BOOL down) {
    (void)down;
    if (!ui_state.active) {
        return;
    }
    
    /* Delegate to current screen */
    uiScreen_t *screen = UI_GetCurrentScreen();
    if (screen && screen->mouse_event) {
        screen->mouse_event(x, y, button);
    }
}

void UI_MenuCommandLocal(LPCSTR command) {
    uiimport.Printf("UI_MenuCommandLocal: %s\n", command);
    
    /* Parse command */
    if (!strncmp(command, "menu ", 5)) {
        UI_Route(command + 5);
    } else {
        uiimport.SendCommand(command);
    }
}

/* Stub callbacks for server data updates */
void UI_UpdateMapListLocal(DWORD count, LPCSTR *names) {
    (void)names;
    uiimport.Printf("UI_UpdateMapList: %d maps\n", (int)count);
}

void UI_UpdateMapInfoLocal(DWORD index, LPCSTR title, LPCSTR description, LPCSTR preview) {
    (void)title;
    (void)description;
    (void)preview;
    uiimport.Printf("UI_UpdateMapInfo: index=%d\n", (int)index);
}

void UI_UpdateGameListLocal(DWORD count, LPCSTR *names) {
    (void)names;
    uiimport.Printf("UI_UpdateGameList: %d games\n", (int)count);
}

void UI_UpdatePlayerInfoLocal(DWORD playerCount, LPCSTR *playerNames) {
    (void)playerNames;
    uiimport.Printf("UI_UpdatePlayerInfo: %d players\n", (int)playerCount);
}

/* Export function table */
uiExport_t UI_GetAPI(uiImport_t import) {
    uiimport = import;
    
    uiExport_t exp;
    memset(&exp, 0, sizeof(exp));
    
    exp.Init = UI_InitLocal;
    exp.Shutdown = UI_ShutdownLocal;
    exp.Refresh = UI_RefreshLocal;
    exp.DrawFrame = UI_DrawFrameLocal;
    exp.KeyEvent = UI_KeyEventLocal;
    exp.MouseEvent = UI_MouseEventLocal;
    exp.MenuCommand = UI_MenuCommandLocal;
    exp.UpdateMapList = UI_UpdateMapListLocal;
    exp.UpdateMapInfo = UI_UpdateMapInfoLocal;
    exp.UpdateGameList = UI_UpdateGameListLocal;
    exp.UpdatePlayerInfo = UI_UpdatePlayerInfoLocal;
    
    return exp;
}
