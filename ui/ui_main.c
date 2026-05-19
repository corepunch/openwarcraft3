/*
 * ui_main.c — UI library entry point and main loop integration.
 *
 * This file implements UI_GetAPI(), which returns the uiExport_t function
 * table to the client. It also contains the main UI update loop (Refresh)
 * and initialization/shutdown code.
 *
 * The UI library is stateless between Init/Shutdown cycles — all persistent
 * state lives in ui_state (defined in ui_local.h).
 */
#include "ui_local.h"

/* Global import callbacks filled by UI_GetAPI */
uiImport_t uiimport;

/* Global UI state */
uiState_t ui_state;

/* Initialize the UI library. Called by the client after UI_GetAPI().
 * Loads all FDF assets and initializes the main menu screen. */
void UI_InitLocal(void) {
    uiimport.Printf("UI library initializing...\n");
    
    memset(&ui_state, 0, sizeof(ui_state));
    ui_state.initialized = true;
    ui_state.active = true;
    ui_state.time = 0;
    
    /* Parse all FDF assets (will be implemented when ui_fdf.c is moved) */
    // UI_ClearTemplates();
    // UI_ParseFDF("UI\\FrameDef\\GlobalStrings.fdf");
    // UI_ParseFDF("UI\\FrameDef\\Glue\\StandardTemplates.fdf");
    // UI_ParseFDF("UI\\FrameDef\\Glue\\MainMenu.fdf");
    // ... (more FDF files)
    
    /* Initialize the main menu as the starting screen */
    // MainMenu_Init();
    // UI_Route("/main");
    
    uiimport.Printf("UI library initialized.\n");
}

/* Shutdown the UI library. Called by the client before exiting or reloading. */
void UI_ShutdownLocal(void) {
    if (!ui_state.initialized) {
        return;
    }
    
    uiimport.Printf("UI library shutting down...\n");
    
    /* Free all FDF frame templates */
    // UI_ClearTemplates();
    
    memset(&ui_state, 0, sizeof(ui_state));
    
    uiimport.Printf("UI library shut down.\n");
}

/* Main UI update loop. Called each client frame with elapsed time in msec.
 * Updates animations, handles screen transitions, processes input. */
void UI_RefreshLocal(DWORD msec) {
    if (!ui_state.initialized || !ui_state.active) {
        return;
    }
    
    ui_state.time += msec;
    
    /* Update current screen */
    if (ui_state.currentScreen && ui_state.currentScreen->refresh) {
        ui_state.currentScreen->refresh();
    }
}

/* Render the current UI screen. Called by the client during SCR_UpdateScreen().
 * Delegates to the active screen's draw function. */
void UI_DrawFrameLocal(void) {
    if (!ui_state.initialized || !ui_state.active) {
        return;
    }
    
    if (ui_state.currentScreen && ui_state.currentScreen->draw) {
        ui_state.currentScreen->draw();
    }
}

/* Handle keyboard input events. Called by the client from Key_Event(). */
static void UI_KeyEventLocal(int key, BOOL down, DWORD time) {
    if (!ui_state.initialized || !ui_state.active) {
        return;
    }
    
    if (ui_state.currentScreen && ui_state.currentScreen->keyEvent) {
        ui_state.currentScreen->keyEvent(key, down);
    }
}

/* Handle mouse input events. Called by the client from mouse event handlers. */
static void UI_MouseEventLocal(int x, int y, int button, BOOL down) {
    if (!ui_state.initialized || !ui_state.active) {
        return;
    }
    
    /* Mouse event handling will be implemented with screen controllers */
    (void)x;
    (void)y;
    (void)button;
    (void)down;
}

/* Navigate to a new menu route (e.g., "/main", "/single-player", "/lan/setup?map=3").
 * Called by the client when a button is clicked or a console command is entered. */
static void UI_MenuCommandLocal(LPCSTR route) {
    if (!ui_state.initialized) {
        return;
    }
    
    uiimport.Printf("UI: Navigating to route: %s\n", route ? route : "(null)");
    
    /* Route to the appropriate screen (will be implemented in ui_router.c) */
    // UI_Route(route);
    
    (void)route;
}

/* Update the map list from server response. */
static void UI_UpdateMapListLocal(DWORD count, LPCSTR *names) {
    /* Store map list for the map select screen to display */
    (void)count;
    (void)names;
}

/* Update map info from server response. */
static void UI_UpdateMapInfoLocal(DWORD index, LPCSTR title, LPCSTR description, LPCSTR preview) {
    /* Store map info for the map select screen to display */
    (void)index;
    (void)title;
    (void)description;
    (void)preview;
}

/* Update game list from server response. */
static void UI_UpdateGameListLocal(DWORD count, LPCSTR *names) {
    /* Store game list for the LAN join screen to display */
    (void)count;
    (void)names;
}

/* Update player info from server response. */
static void UI_UpdatePlayerInfoLocal(DWORD playerCount, LPCSTR *playerNames) {
    /* Store player info for the game setup screen to display */
    (void)playerCount;
    (void)playerNames;
}

/* Entry point called by the client to get the UI function table.
 * The client must fill the uiImport_t struct before calling this. */
uiExport_t UI_GetAPI(uiImport_t import) {
    uiimport = import;
    
    return (uiExport_t) {
        .Init = UI_InitLocal,
        .Shutdown = UI_ShutdownLocal,
        .Refresh = UI_RefreshLocal,
        .DrawFrame = UI_DrawFrameLocal,
        .KeyEvent = UI_KeyEventLocal,
        .MouseEvent = UI_MouseEventLocal,
        .MenuCommand = UI_MenuCommandLocal,
        .UpdateMapList = UI_UpdateMapListLocal,
        .UpdateMapInfo = UI_UpdateMapInfoLocal,
        .UpdateGameList = UI_UpdateGameListLocal,
        .UpdatePlayerInfo = UI_UpdatePlayerInfoLocal,
    };
}
