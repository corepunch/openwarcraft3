/*
 * ui_main.c — UI library entry point and lifecycle management.
 */

#include "ui_local.h"
#include "ui_screen.h"

/* Global import table filled by UI_GetAPI */
uiImport_t uiimport;
uiMouseState_t ui_mouse;

/* Internal state */
typedef struct {
    BOOL initialized;
    BOOL active;
    DWORD time;
} uiState_t;

static uiState_t ui_state;

VECTOR2 UI_MouseToFdf(void) {
    LPRENDERER renderer = uiimport.GetRenderer ? uiimport.GetRenderer() : NULL;
    size2_t window = renderer && renderer->GetWindowSize ? renderer->GetWindowSize() : MAKE(size2_t, 0, 0);
    FLOAT window_aspect = UI_MIN_ASPECT;
    FLOAT x_scale = 1.0f;
    FLOAT y_scale = 1.0f;
    RECT scene;
    FLOAT nx = 0;
    FLOAT ny = 0;

    if (window.width > 0 && window.height > 0) {
        window_aspect = (FLOAT)window.width / (FLOAT)window.height;
        nx = (FLOAT)ui_mouse.x / (FLOAT)window.width;
        ny = (FLOAT)ui_mouse.y / (FLOAT)window.height;
    }
    if (window_aspect > UI_MIN_ASPECT) {
        x_scale = window_aspect / UI_MIN_ASPECT;
    } else if (window_aspect < UI_MIN_ASPECT) {
        y_scale = UI_MIN_ASPECT / window_aspect;
    }

    scene.w = UI_BASE_WIDTH * x_scale;
    scene.h = UI_BASE_HEIGHT * y_scale;
    scene.x = (UI_BASE_WIDTH - scene.w) * 0.5f;
    scene.y = (UI_BASE_HEIGHT - scene.h) * 0.5f;

    return MAKE(VECTOR2,
                scene.x + nx * scene.w,
                scene.y + ny * scene.h);
}

BOOL UI_MouseContains(LPCRECT rect) {
    VECTOR2 const mouse = UI_MouseToFdf();
    return Rect_contains(rect, &mouse);
}

void UI_ClearMouseTransient(void) {
    ui_mouse.event = UI_MOUSE_EVENT_NONE;
}

void UI_InitLocal(void) {
    memset(&ui_state, 0, sizeof(ui_state));
    memset(&ui_mouse, 0, sizeof(ui_mouse));
    UI_ResetGlueSceneModels();
    
    uiimport.Printf("UI_InitLocal: loading FDF assets\n");

    UI_LoadTheme("UI\\war3skins.txt");
    UI_ParseFDF("UI\\FrameDef\\GlobalStrings.fdf");
    
    /* Load core menu FDF files */
    UI_ParseFDF("UI\\FrameDef\\UI\\EscMenuTemplates.fdf");
    UI_ParseFDF("UI\\FrameDef\\UI\\EscMenuMainPanel.fdf");
    UI_ParseFDF("UI\\FrameDef\\Glue\\StandardTemplates.fdf");
    UI_ParseFDF("UI\\FrameDef\\Glue\\MainMenu.fdf");
    UI_ParseFDF("UI\\FrameDef\\Glue\\MapListBox.fdf");
    UI_ParseFDF("UI\\FrameDef\\Glue\\MapInfoPane.fdf");
    UI_ParseFDF("UI\\FrameDef\\Glue\\LocalMultiplayerCreate.fdf");
    UI_ParseFDF("UI\\FrameDef\\Glue\\TeamSetup.fdf");
    UI_ParseFDF("UI\\FrameDef\\Glue\\PlayerSlot.fdf");
    UI_ParseFDF("UI\\FrameDef\\Glue\\GameChatroom.fdf");
    
    /* Load in-game HUD FDF files */
    UI_ParseFDF("UI\\FrameDef\\UI\\ConsoleUI.fdf");
    UI_ParseFDF("UI\\FrameDef\\UI\\ResourceBar.fdf");
    UI_ParseFDF("UI\\FrameDef\\UI\\UpperButtonBar.fdf");
    UI_ParseFDF("UI\\FrameDef\\UI\\SimpleInfoPanel.fdf");
    UI_ParseFDF("UI\\FrameDef\\UI\\CinematicPanel.fdf");
    
    ui_state.initialized = true;
    ui_state.active = true;
    
    /* Route to the configured first UI scene. */
    LPCSTR start_route = uiimport.Cvar_String
        ? uiimport.Cvar_String("ui_start_route", "/main")
        : "/main";
    UI_Route(start_route && *start_route ? start_route : "/main");
}

void UI_ShutdownLocal(void) {
    UI_ResetGlueSceneModels();
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
    UI_ClearMouseTransient();
}

void UI_KeyEventLocal(int key, BOOL down, DWORD time) {
    (void)time;
    if (!ui_state.active) {
        return;
    }

    if (down && UI_EditKey(key)) {
        return;
    }
    
    /* Delegate to current screen */
    uiScreen_t *screen = UI_GetCurrentScreen();
    if (screen && screen->key_event) {
        screen->key_event(key, down);
    }
}

void UI_MouseEventLocal(int x, int y, int button, BOOL down) {
    if (!ui_state.active) {
        return;
    }

    ui_mouse.x = x;
    ui_mouse.y = y;
    if (button) {
        ui_mouse.button = button;
        ui_mouse.down = down;
        if (button == 1) {
            ui_mouse.event = down ? UI_MOUSE_LEFT_DOWN : UI_MOUSE_LEFT_UP;
        } else if (button == 2) {
            ui_mouse.event = down ? UI_MOUSE_RIGHT_DOWN : UI_MOUSE_RIGHT_UP;
        } else if (button == 4) {
            ui_mouse.event = UI_MOUSE_WHEEL_UP;
            ui_mouse.button = 0;
            ui_mouse.down = false;
        } else if (button == 5) {
            ui_mouse.event = UI_MOUSE_WHEEL_DOWN;
            ui_mouse.button = 0;
            ui_mouse.down = false;
        }
        if (!down) {
            ui_mouse.button = 0;
        }
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
        uiimport.Cmd_ExecuteText(command);
    }
}

/* Stub callbacks for server data updates */
/* Forward unit UI data to active screen (Phase 8) */
void UI_UpdateUnitUILocal(DWORD num_units, uiUnitData_t *units) {
    uiimport.Printf("UI_UpdateUnitUI: %d units\n", (int)num_units);
    
    /* Forward to current screen if it implements unit UI handling */
    uiScreen_t *screen = UI_GetCurrentScreen();
    if (screen && screen->update_unit_ui) {
        screen->update_unit_ui(num_units, units);
    }
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
    exp.TextInput = UI_TextInputLocal;
    exp.MouseEvent = UI_MouseEventLocal;
    exp.MenuCommand = UI_MenuCommandLocal;
    exp.UpdateUnitUI = UI_UpdateUnitUILocal;
    
    return exp;
}
