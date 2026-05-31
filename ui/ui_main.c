/*
 * ui_main.c — UI library entry point and lifecycle management.
 */

#include <stdlib.h>

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
static BOOL ui_menu_commands_registered;
static LPFRAMEDEF resource_bar_frame;
static LPFRAMEDEF resource_bar_gold_text;
static LPFRAMEDEF resource_bar_lumber_text;
static LPFRAMEDEF resource_bar_supply_text;
static LPFRAMEDEF resource_bar_upkeep_text;
static LPFRAMEDEF cinematic_panel_frame;
static LPFRAMEDEF cinematic_speaker_text;
static LPFRAMEDEF cinematic_dialogue_text;
static LPFRAMEDEF loading_frame;
static LPFRAMEDEF loading_custom_panel;
static LPFRAMEDEF loading_melee_panel;
static LPFRAMEDEF loading_background;
static LPFRAMEDEF loading_bar;
static LPFRAMEDEF loading_title_text;
static LPFRAMEDEF loading_subtitle_text;
static LPFRAMEDEF loading_text;

static BOOL UI_IsMapCommand(LPCSTR command) {
    if (!command) {
        return false;
    }
    while (*command == ' ' || *command == '\t' || *command == '\r' || *command == '\n') {
        command++;
    }
    if (strncmp(command, "map", 3) ||
        (command[3] != ' ' && command[3] != '\t')) {
        return false;
    }
    command += 4;
    while (*command == ' ' || *command == '\t') {
        command++;
    }
    return *command != '\0';
}

typedef struct {
    PATHSTR map;
    char title[256];
    char subtitle[256];
    char text[1024];
    DWORD background_model;
    DWORD background_sequence;
    DWORD progress_model;
} uiLoadingState_t;

static uiLoadingState_t loading_state;

typedef struct {
    LPCSTR command;
    LPCSTR route;
} uiMenuCommand_t;

static uiMenuCommand_t const ui_menu_commands[] = {
    { "menu_main", "/main" },
    { "menu_game", "/single-player" },
    { "menu_multiplayer", "/lan/create" },
    { "menu_options", "/options" },
    { "menu_video", "/options/video" },
    { "menu_keys", "/options/keys" },
    { "menu_loadgame", "/loadgame" },
    { "menu_savegame", "/savegame" },
    { "menu_playerconfig", "/playerconfig" },
    { "menu_startserver", "/lan/create" },
    { "menu_joinserver", "/lan/create" },
    { "menu_credits", "/credits" },
    { "menu_quit", "/main/quit-confirm" },
    { NULL, NULL },
};

static void UI_MenuMain_f(void) {
    UI_Route("/main");
}

static void UI_MenuGame_f(void) {
    UI_Route("/single-player");
}

static void UI_MenuMultiplayer_f(void) {
    UI_Route("/lan/create");
}

static void UI_MenuOptions_f(void) {
    UI_Route("/options");
}

static void UI_MenuVideo_f(void) {
    UI_Route("/options/video");
}

static void UI_MenuKeys_f(void) {
    UI_Route("/options/keys");
}

static void UI_MenuLoadGame_f(void) {
    UI_Route("/loadgame");
}

static void UI_MenuSaveGame_f(void) {
    UI_Route("/savegame");
}

static void UI_MenuPlayerConfig_f(void) {
    UI_Route("/playerconfig");
}

static void UI_MenuStartServer_f(void) {
    UI_Route("/lan/create");
}

static void UI_MenuJoinServer_f(void) {
    UI_Route("/lan/create");
}

static void UI_MenuCredits_f(void) {
    UI_Route("/credits");
}

static void UI_MenuQuit_f(void) {
    UI_Route("/main/quit-confirm");
}

typedef struct {
    LPCSTR command;
    void (*function)(void);
} uiMenuCommandDef_t;

static uiMenuCommandDef_t const ui_menu_command_defs[] = {
    { "menu_main", UI_MenuMain_f },
    { "menu_game", UI_MenuGame_f },
    { "menu_multiplayer", UI_MenuMultiplayer_f },
    { "menu_options", UI_MenuOptions_f },
    { "menu_video", UI_MenuVideo_f },
    { "menu_keys", UI_MenuKeys_f },
    { "menu_loadgame", UI_MenuLoadGame_f },
    { "menu_savegame", UI_MenuSaveGame_f },
    { "menu_playerconfig", UI_MenuPlayerConfig_f },
    { "menu_startserver", UI_MenuStartServer_f },
    { "menu_joinserver", UI_MenuJoinServer_f },
    { "menu_credits", UI_MenuCredits_f },
    { "menu_quit", UI_MenuQuit_f },
    { NULL, NULL },
};

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

static void UI_DrawConsoleMinimap(void) {
    LPRENDERER renderer = uiimport.GetRenderer ? uiimport.GetRenderer() : NULL;
    RECT const rect = { 0.0070f, 0.4525f, 0.1395f, 0.1395f };

    if (!renderer || !renderer->DrawMinimap) {
        return;
    }
    renderer->DrawMinimap(&rect);
}

static LPCSTR UI_MenuCommandRoute(LPCSTR command) {
    for (uiMenuCommand_t const *menu = ui_menu_commands; menu->command; menu++) {
        if (!strcmp(menu->command, command)) {
            return menu->route;
        }
    }
    return NULL;
}

static void UI_RegisterMenuCommands(void) {
    if (ui_menu_commands_registered || !uiimport.Cmd_AddCommand) {
        return;
    }
    for (uiMenuCommandDef_t const *cmd = ui_menu_command_defs; cmd->command; cmd++) {
        uiimport.Cmd_AddCommand(cmd->command, cmd->function);
    }
    ui_menu_commands_registered = true;
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

static BOOL UI_CinematicActive(LPCPLAYER ps) {
    return ps && ps->client_ui_state == CLIENT_UI_CINEMATIC;
}

static BOOL UI_LoadingActive(LPCPLAYER ps) {
    return ps && ps->client_ui_state == CLIENT_UI_LOADING;
}

static void UI_InitCinematicPanel(void) {
    cinematic_panel_frame = UI_FindFrame("CinematicPanel");
    cinematic_speaker_text = UI_FindFrame("CinematicSpeakerText");
    cinematic_dialogue_text = UI_FindFrame("CinematicDialogueText");
}

static void UI_DrawCinematicPanel(LPCPLAYER ps) {
    if (!ps || !cinematic_panel_frame) {
        return;
    }

    if (cinematic_speaker_text) {
        UI_SetTextPointer(cinematic_speaker_text, ps->texts[PLAYERTEXT_SPEAKER] ? ps->texts[PLAYERTEXT_SPEAKER] : "");
    }
    if (cinematic_dialogue_text) {
        UI_SetTextPointer(cinematic_dialogue_text, ps->texts[PLAYERTEXT_DIALOGUE] ? ps->texts[PLAYERTEXT_DIALOGUE] : "");
    }

    UI_DrawFrame(cinematic_panel_frame);
}

static LPCSTR UI_CsvField(LPCSTR text, DWORD index, LPSTR out, DWORD out_size) {
    DWORD field = 0;
    DWORD len = 0;
    LPCSTR p = text;

    if (!out || out_size == 0) {
        return "";
    }
    out[0] = '\0';
    if (!text) {
        return out;
    }

    while (*p && field < index) {
        if (*p++ == ',') {
            field++;
        }
    }
    while (p[len] && p[len] != ',' && len + 1 < out_size) {
        len++;
    }
    memcpy(out, p, len);
    out[len] = '\0';
    return out;
}

static LPCSTR UI_LoadingMapPath(void) {
    LPCSTR path = uiimport.GetLoadingMap ? uiimport.GetLoadingMap() : NULL;

    if (path && *path) {
        return path;
    }
    return uiimport.Cvar_String ? uiimport.Cvar_String("map", "") : "";
}

static DWORD UI_LoadCampaignLoadingModel(DWORD campaign_background, DWORD *sequence_index) {
    sheetRow_t *world_edit_data;
    char key[8];
    char sequence[16];
    char model[MAX_PATHLEN];
    LPCSTR row;

    if (sequence_index) {
        *sequence_index = 0;
    }
    if (!uiimport.ReadConfig || !uiimport.FindSheetCell) {
        return 0;
    }

    world_edit_data = uiimport.ReadConfig("UI\\WorldEditData.txt");
    snprintf(key, sizeof(key), "%02u", (unsigned)campaign_background);
    row = uiimport.FindSheetCell(world_edit_data, "LoadingScreens", key);
    UI_CsvField(row, 1, sequence, sizeof(sequence));
    UI_CsvField(row, 2, model, sizeof(model));
    if (sequence_index && sequence[0]) {
        *sequence_index = (DWORD)atoi(sequence);
    }
    return model[0] ? UI_LoadModel(model, false) : 0;
}

static DWORD UI_DefaultLoadingModel(void) {
    return UI_LoadModel("LoadingMeleeBackground", true);
}

static void UI_UpdateLoadingMapInfo(void) {
    MAPINFO info;
    LPCSTR map_path = UI_LoadingMapPath();
    DWORD background_model = 0;
    DWORD background_sequence = 0;

    if (!map_path || !*map_path || !strcmp(loading_state.map, map_path)) {
        return;
    }

    memset(&info, 0, sizeof(info));
    memset(&loading_state, 0, sizeof(loading_state));
    snprintf(loading_state.map, sizeof(loading_state.map), "%s", map_path);

    if (uiimport.ReadMapInfo && uiimport.ReadMapInfo(map_path, &info)) {
        if (uiimport.ResolveMapInfoString) {
            uiimport.ResolveMapInfoString(&info, info.loadingScreenTitle, loading_state.title, sizeof(loading_state.title));
            uiimport.ResolveMapInfoString(&info, info.loadingScreenSubtitle, loading_state.subtitle, sizeof(loading_state.subtitle));
            uiimport.ResolveMapInfoString(&info, info.loadingScreenText, loading_state.text, sizeof(loading_state.text));
        }
        if (uiimport.SanitizeMapInfoText) {
            uiimport.SanitizeMapInfoText(loading_state.title);
            uiimport.SanitizeMapInfoText(loading_state.subtitle);
            uiimport.SanitizeMapInfoText(loading_state.text);
        }
        if (info.campaignBackgroundNumber != (DWORD)-1) {
            background_model = UI_LoadCampaignLoadingModel(info.campaignBackgroundNumber, &background_sequence);
        }
        if (uiimport.FreeMapInfo) {
            uiimport.FreeMapInfo(&info);
        }
    }

    if (!loading_state.title[0] && uiimport.DefaultMapName) {
        uiimport.DefaultMapName(map_path, loading_state.title, sizeof(loading_state.title));
    }

    loading_state.background_model = background_model ? background_model : UI_DefaultLoadingModel();
    loading_state.background_sequence = background_sequence;
    loading_state.progress_model = UI_LoadModel("LoadingProgressBar", true);
}

static void UI_InitLoadingScreen(void) {
    loading_frame = UI_FindFrame("Loading");
    loading_custom_panel = UI_FindFrame("LoadingCustomPanel");
    loading_melee_panel = UI_FindFrame("LoadingMeleePanel");
    loading_background = UI_FindFrame("LoadingBackground");
    loading_bar = UI_FindFrame("LoadingBar");
    loading_title_text = UI_FindFrame("LoadingTitleText");
    loading_subtitle_text = UI_FindFrame("LoadingSubtitleText");
    loading_text = UI_FindFrame("LoadingText");

    if (loading_custom_panel) {
        UI_SetHidden(loading_custom_panel, false);
    }
    if (loading_melee_panel) {
        UI_SetHidden(loading_melee_panel, true);
    }
}

static void UI_DrawLoadingScreen(void) {
    FLOAT loading_progress = uiimport.GetLoadingProgress ? uiimport.GetLoadingProgress() : 0.0f;

    UI_UpdateLoadingMapInfo();

    if (!loading_frame) {
        return;
    }
    if (loading_background) {
        snprintf(loading_background->TextStorage, sizeof(loading_background->TextStorage), "#!%u",
                 (unsigned)loading_state.background_sequence);
        loading_background->Text = loading_background->TextStorage;
        loading_background->Portrait.model = loading_state.background_model;
    }
    if (loading_bar) {
        snprintf(loading_bar->TextStorage, sizeof(loading_bar->TextStorage), "#0@%.4f", loading_progress);
        loading_bar->Text = loading_bar->TextStorage;
        loading_bar->Portrait.model = loading_state.progress_model;
    }
    if (loading_title_text) {
        UI_SetTextPointer(loading_title_text, loading_state.title);
    }
    if (loading_subtitle_text) {
        UI_SetTextPointer(loading_subtitle_text, loading_state.subtitle);
    }
    if (loading_text) {
        UI_SetTextPointer(loading_text, loading_state.text);
    }

    UI_DrawFrame(loading_frame);
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
    UI_RegisterMenuCommands();
    
    uiimport.Printf("UI_InitLocal: loading FDF assets\n");

    UI_LoadTheme("UI\\war3skins.txt");
    UI_ParseFDF("UI\\FrameDef\\GlobalStrings.fdf");
    
    /* Load core menu FDF files */
    UI_ParseFDF("UI\\FrameDef\\UI\\EscMenuTemplates.fdf");
    UI_ParseFDF("UI\\FrameDef\\UI\\EscMenuMainPanel.fdf");
    UI_ParseFDF("UI\\FrameDef\\Glue\\StandardTemplates.fdf");
    UI_ParseFDF("UI\\FrameDef\\Glue\\MainMenu.fdf");
    UI_ParseFDF("UI\\FrameDef\\Glue\\SinglePlayerMenu.fdf");
    UI_ParseFDF("UI\\FrameDef\\Glue\\CampaignMenu.fdf");
    UI_ParseFDF("UI\\FrameDef\\Glue\\DialogWar3.fdf");
    UI_ParseFDF("UI\\FrameDef\\Glue\\MapListBox.fdf");
    UI_ParseFDF("UI\\FrameDef\\Glue\\MapInfoPane.fdf");
    UI_ParseFDF("UI\\FrameDef\\Glue\\LocalMultiplayerCreate.fdf");
    UI_ParseFDF("UI\\FrameDef\\Glue\\TeamSetup.fdf");
    UI_ParseFDF("UI\\FrameDef\\Glue\\PlayerSlot.fdf");
    UI_ParseFDF("UI\\FrameDef\\Glue\\GameChatroom.fdf");
    UI_ParseFDF("UI\\FrameDef\\Glue\\Loading.fdf");
    UI_ParseFDF("UI\\FrameDef\\UI\\ResourceBar.fdf");
    UI_ParseFDF("UI\\FrameDef\\UI\\CinematicPanel.fdf");
    UI_InitLoadingScreen();
    UI_InitGameResourceBar();
    UI_InitCinematicPanel();
    
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
    UI_ResetRouter();
    LPCSTR start_route = uiimport.Cvar_String
        ? uiimport.Cvar_String("ui_start_route", "/main")
        : "/main";
    UI_Route(start_route && *start_route ? start_route : "/main");
}

void UI_ShutdownLocal(void) {
    UI_ResetGlueSceneModels();
    UI_ResetRouter();
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
        LPCPLAYER ps = uiimport.GetPlayerState ? uiimport.GetPlayerState() : NULL;

        if (UI_LoadingActive(ps)) {
            UI_DrawLoadingScreen();
        } else if (UI_CinematicActive(ps)) {
            UI_DrawCinematicPanel(ps);
        } else {
            UI_DrawConsoleBackdropOnly();
            UI_DrawConsoleMinimap();
            UI_DrawResourceBar();
        }
    } else {
        uiScreen_t *screen = UI_GetCurrentScreen();
        if (screen && screen->draw) {
            screen->draw();
        }
    }
    UI_LayoutDrawOverlays();
    UI_ClearMouseTransient();
}

void UI_KeyEventLocal(int key, BOOL down, DWORD time) {
    (void)time;
    if (!ui_state.active) {
        return;
    }

    if (down && UI_LayoutEditKey(key)) {
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
    LPCSTR route;

    uiimport.Printf("UI_MenuCommandLocal: %s\n", command);
    
    /* Parse command */
    route = UI_MenuCommandRoute(command);
    if (route) {
        UI_Route(route);
    } else if (!strncmp(command, "menu ", 5)) {
        UI_Route(command + 5);
    } else {
        if (UI_IsMapCommand(command)) {
            ui_state.game_mode = true;
        }
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
    exp.SetLayoutLayer = UI_LayoutSetLayer;
    exp.ClearLayoutLayer = UI_LayoutClearLayer;
    exp.HitTestLayout = UI_LayoutHitTest;
    
    return exp;
}
