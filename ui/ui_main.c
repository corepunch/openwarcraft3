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
    BOOL game_mode;
    DWORD time;
} uiState_t;

static uiState_t ui_state;
static LPFRAMEDEF resource_bar_frame;
static LPFRAMEDEF resource_bar_gold_text;
static LPFRAMEDEF resource_bar_lumber_text;
static LPFRAMEDEF resource_bar_supply_text;
static LPFRAMEDEF resource_bar_upkeep_text;

typedef struct {
    LPCSTR texture;
    BOOL decorate;
    RECT screen;
    RECT uv;
} uiConsoleBackdropPart_t;

static void UI_DrawImagePart(LPCSTR texture_name, BOOL decorate, LPCRECT screen, LPCRECT uv) {
    LPRENDERER renderer = uiimport.GetRenderer ? uiimport.GetRenderer() : NULL;
    DWORD texture_id;
    LPCTEXTURE texture;

    if (!renderer || !renderer->DrawImageEx || !texture_name || !*texture_name) {
        return;
    }

    texture_id = UI_LoadTexture(texture_name, decorate);
    texture = UI_GetTexture(texture_id);
    if (!texture) {
        return;
    }

    renderer->DrawImageEx(&MAKE(drawImage_t,
                                .texture = texture,
                                .shader = SHADER_UI,
                                .alphamode = BLEND_MODE_ALPHAKEY,
                                .screen = *screen,
                                .uv = *uv,
                                .color = COLOR32_WHITE,
                                .rotate = false));
}

static void UI_DrawConsoleBackdropPart(uiConsoleBackdropPart_t const *part) {
    if (!part) {
        return;
    }
    UI_DrawImagePart(part->texture, part->decorate, &part->screen, &part->uv);
}

static void UI_DrawConsoleBackdropOnly(void) {
    static uiConsoleBackdropPart_t const parts[] = {
        { "ConsoleTexture01", true, { 0.000f, 0.000f, 0.256f, 0.032f }, { 0.00000000f, 0.000000f, 1.00000000f, 0.125000f } },
        { "ConsoleTexture02", true, { 0.256f, 0.000f, 0.087f, 0.032f }, { 0.00000000f, 0.000000f, 0.33984375f, 0.125000f } },
        { "ConsoleTexture02", true, { 0.459f, 0.000f, 0.053f, 0.032f }, { 0.79296875f, 0.000000f, 0.20703125f, 0.125000f } },
        { "ConsoleTexture03", true, { 0.512f, 0.000f, 0.256f, 0.032f }, { 0.00000000f, 0.000000f, 1.00000000f, 0.125000f } },
        { "ConsoleTexture04", true, { 0.768f, 0.000f, 0.032f, 0.032f }, { 0.00000000f, 0.000000f, 1.00000000f, 0.125000f } },
        { "ConsoleTexture01", true, { 0.000f, 0.424f, 0.256f, 0.176f }, { 0.00000000f, 0.312500f, 1.00000000f, 0.687500f } },
        { "ConsoleTexture02", true, { 0.256f, 0.450f, 0.256f, 0.150f }, { 0.00000000f, 0.414062f, 1.00000000f, 0.585938f } },
        { "ConsoleTexture03", true, { 0.512f, 0.424f, 0.256f, 0.176f }, { 0.00000000f, 0.312500f, 1.00000000f, 0.687500f } },
        { "ConsoleTexture04", true, { 0.768f, 0.424f, 0.032f, 0.176f }, { 0.00000000f, 0.312500f, 1.00000000f, 0.687500f } },
    };

    FOR_LOOP(i, sizeof(parts) / sizeof(parts[0])) {
        UI_DrawConsoleBackdropPart(&parts[i]);
    }
}

static void UI_InitGameResourceBar(void) {
    resource_bar_frame = UI_FindFrame("ResourceBarFrame");
    resource_bar_gold_text = UI_FindFrame("ResourceBarGoldText");
    resource_bar_lumber_text = UI_FindFrame("ResourceBarLumberText");
    resource_bar_supply_text = UI_FindFrame("ResourceBarSupplyText");
    resource_bar_upkeep_text = UI_FindFrame("ResourceBarUpkeepText");

    if (resource_bar_frame) {
        UI_SetPoint(resource_bar_frame,
                    FRAMEPOINT_TOPRIGHT,
                    NULL,
                    FRAMEPOINT_TOPRIGHT,
                    0.0f,
                    0.0f);
    }
}

static void UI_DrawResourceBar(void) {
    LPCPLAYER ps = uiimport.GetPlayerState ? uiimport.GetPlayerState() : NULL;

    if (!ps || !resource_bar_frame) {
        return;
    }

    if (resource_bar_gold_text) {
        UI_SetText(resource_bar_gold_text, "%u", (unsigned)ps->stats[PLAYERSTATE_RESOURCE_GOLD]);
    }
    if (resource_bar_lumber_text) {
        UI_SetText(resource_bar_lumber_text, "%u", (unsigned)ps->stats[PLAYERSTATE_RESOURCE_LUMBER]);
    }
    if (resource_bar_supply_text) {
        UI_SetText(resource_bar_supply_text,
                   "%u/%u",
                   (unsigned)ps->stats[PLAYERSTATE_RESOURCE_FOOD_USED],
                   (unsigned)ps->stats[PLAYERSTATE_RESOURCE_FOOD_CAP]);
    }
    if (resource_bar_upkeep_text) {
        UI_SetText(resource_bar_upkeep_text, "UPKEEP_NONE");
    }

    UI_DrawFrame(resource_bar_frame);
}

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
    UI_ParseFDF("UI\\FrameDef\\UI\\ResourceBar.fdf");
    UI_InitGameResourceBar();
    
    ui_state.initialized = true;
    ui_state.active = true;
    
    /*
     * Map launches use the server-authored in-game HUD via svc_layout.  Leave
     * the client menu router idle there so no glue screen covers the game.
     */
    LPCSTR map = uiimport.Cvar_String
        ? uiimport.Cvar_String("map", "")
        : "";
    if (map && *map) {
        ui_state.game_mode = true;
        return;
    }

    /* Route to the configured first menu scene. */
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
    if (ui_state.game_mode) {
        UI_DrawConsoleBackdropOnly();
        UI_DrawResourceBar();
    } else {
        uiScreen_t *screen = UI_GetCurrentScreen();
        if (screen && screen->draw) {
            screen->draw();
        }
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
