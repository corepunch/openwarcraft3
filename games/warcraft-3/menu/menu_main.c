/*
 * menu_main.c — UI library entry point and lifecycle management.
 */

#include <stdlib.h>
#include <stdio.h>

#include "menu_local.h"
#include "common/video_modes.h"
#include "menu_screen.h"

/* Global import table filled by M_GetAPI */
menuImport_t mi;
LPCPLAYER menu_player;

void M_UpdatePlayerState(LPCPLAYER state) { menu_player = state; }

/* Internal state */
typedef struct {
    BOOL initialized;
    BOOL active;
    DWORD time;
    VECTOR2 mouse_fdf;
    uiScreen_t *transition_screen;
    void (*transition_action)(void);
} uiState_t;

static uiState_t ui_state;
static uiScreen_t *ui_current_screen = NULL;
static BOOL ui_menu_commands_registered;

/* Some classic/pre-widescreen skin tables expose only one of the paired
 * ConsoleTexture05/06 fields even though both extension tiles are installed.
 * When one symbolic key is absent, derive it from the resolved sibling path so
 * the active race/custom skin remains authoritative. */
static LPCSTR M_ConsoleExtensionSibling(LPCSTR key) {
    static PATHSTR path;
    LPCSTR sibling_key, sibling;
    char from_digit, to_digit;
    char *dot, *end;

    if (!key) return NULL;
    if (!strcmp(key, "ConsoleTexture05")) {
        sibling_key = "ConsoleTexture06";
        from_digit = '6';
        to_digit = '5';
    } else if (!strcmp(key, "ConsoleTexture06")) {
        sibling_key = "ConsoleTexture05";
        from_digit = '5';
        to_digit = '6';
    } else {
        return NULL;
    }

    sibling = Theme_String(sibling_key, "Default");
    if (!sibling || !*sibling || !strcmp(sibling, sibling_key)) return NULL;
    snprintf(path, sizeof(path), "%s", sibling);
    dot = strrchr(path, '.');
    end = dot ? dot : path + strlen(path);
    if (end - path < 2 || end[-2] != '0' || end[-1] != from_digit) return NULL;
    end[-1] = to_digit;
    return path;
}

/* Resolve symbolic server-authored WC3 image names using the local player's skin. */
LPCSTR M_ResolveImagePath(LPCSTR key) {
    LPCSTR resolved, fallback;

    if (!key || !*key || strchr(key, '\\') || strchr(key, '/')) return key;
    resolved = Theme_String(key, "Default");
    if (resolved && strcmp(resolved, key)) return resolved;
    fallback = M_ConsoleExtensionSibling(key);
    return fallback ? fallback : resolved;
}

static void UI_ClearScreen(void);

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


static void UI_SetScreen(uiScreen_t *screen) {
    uiScreen_t *previous_screen = ui_current_screen;

    if (ui_current_screen == screen) {
        return;
    }

    fprintf(stderr,
            "UI_SetScreen: %s -> %s\n",
            ui_current_screen ? ui_current_screen->name : "(null)",
            screen ? screen->name : "(null)");

    if (!screen) {
        if (ui_current_screen && ui_current_screen->shutdown) {
            fprintf(stderr,
                    "UI_SetScreen: shutting down screen '%s'\n",
                    ui_current_screen->name);
            ui_current_screen->shutdown();
        }
        ui_current_screen = NULL;
        return;
    }
    if (screen->load && !screen->load()) {
        fprintf(stderr,
                "UI_SetScreen: failed to load screen '%s', keeping '%s'\n",
                screen->name,
                previous_screen ? previous_screen->name : "(null)");
        if (mi.Printf) {
            mi.Printf("UI_SetScreen: failed to load screen '%s'\n", screen->name);
        }
        return;
    }
    if (ui_current_screen && ui_current_screen->shutdown) {
        fprintf(stderr,
                "UI_SetScreen: shutting down screen '%s'\n",
                ui_current_screen->name);
        ui_current_screen->shutdown();
    }
    ui_current_screen = screen;
    if (screen->panel != UI_GLUE_NONE)
        UI_GotoGluePanel(screen->panel, NULL);
    if (screen->init) {
        fprintf(stderr, "UI_SetScreen: initializing screen '%s'\n", screen->name);
        screen->init();
    }
}

static void UI_FinishScreenTransition(void) {
    uiScreen_t *screen = ui_state.transition_screen;

    ui_state.transition_screen = NULL;
    UI_SetScreen(screen);
}

static void UI_BeginScreenTransition(void) {
    UI_SetScreen(ui_state.transition_screen);
}

static void UI_FinishActionTransition(void) {
    void (*action)(void) = ui_state.transition_action;

    ui_state.transition_action = NULL;
    action();
}

static void UI_TransitionToScreen(uiScreen_t *screen) {
    if (ui_state.transition_screen || ui_state.transition_action) return;
    if (screen->panel == UI_GLUE_NONE) { UI_SetScreen(screen); return; }
    ui_state.transition_screen = screen;
    UI_GotoGluePanelTransition(screen->panel, UI_BeginScreenTransition, UI_FinishScreenTransition);
}

void M_TransitionToAction(void (*action)(void)) {
    if (ui_state.transition_screen || ui_state.transition_action) return;
    ui_state.transition_action = action;
    UI_CloseGluePanel(UI_FinishActionTransition);
}

uiScreen_t *UI_GetCurrentScreen(void) {
    return ui_current_screen;
}

__attribute__((visibility("hidden"))) void M_ShowMainMenu(void) {
    UI_SetScreen(&mainMenuScreen);
    MainMenu_ShowMainPanel();
}

void M_ShowSinglePlayerMenu(void) {
    UI_SetScreen(&singlePlayerMenuScreen);
    SinglePlayerMenu_ShowMain();
}

void M_ShowOptionsMenu(void) {
    UI_TransitionToScreen(&optionsMenuScreen);
}

void M_ShowCreditsMenu(void) {
    UI_SetScreen(&creditsMenuScreen);
}

void M_ShowLanCreateMenu(void) {
    LAN_ShowCreate();
    UI_SetScreen(&lanJoinScreen);
}

static void M_ShowSinglePlayerSkirmishMenu(void) {
    LAN_ShowSinglePlayerCreate();
    UI_SetScreen(&lanJoinScreen);
}

void M_ShowLanBrowserMenu(void) {
    LAN_ShowBrowser();
    UI_SetScreen(&lanJoinScreen);
}

void M_ShowGameSetupMenu(void) {
    UI_SetScreen(&gameSetupScreen);
}

static void UI_MenuMain_f(void) {
    if (UI_GetCurrentScreen() != &mainMenuScreen) {
        UI_TransitionToScreen(&mainMenuScreen);
        return;
    }
    M_ShowMainMenu();
}

static void UI_MenuGame_f(void) {
    if (UI_GetCurrentScreen() != &singlePlayerMenuScreen) {
        UI_TransitionToScreen(&singlePlayerMenuScreen);
        return;
    }
    M_ShowSinglePlayerMenu();
}

static void UI_MenuMultiplayer_f(void) {
    M_ShowLanBrowserMenu();
}

static void UI_MenuOptions_f(void) {
    M_ShowOptionsMenu();
}

static void UI_MenuVideo_f(void) {
    UI_SetScreen(&optionsMenuScreen);
    OptionsMenu_ShowVideo();
}

static void UI_MenuKeys_f(void) {
    UI_SetScreen(&optionsMenuScreen);
    OptionsMenu_ShowKeys();
}

static void UI_MenuLoadGame_f(void) {
    mi.Cmd_ExecuteText("load quick\n");
}

static void UI_MenuSaveGame_f(void) {
    mi.Cmd_ExecuteText("save quick\n");
}

static void UI_MenuPlayerConfig_f(void) {
}

static void UI_MenuStartServer_f(void) {
    LAN_ApplyPlayerName();
    M_ShowLanCreateMenu();
}

static void UI_MenuJoinServer_f(void) {
    M_ShowLanBrowserMenu();
}

static void UI_MenuCredits_f(void) {
    M_ShowCreditsMenu();
}

static void UI_MenuQuit_f(void) {
    UI_SetScreen(&mainMenuScreen);
    MainMenu_ShowQuitConfirm();
}

static void UI_MenuDisconnected_f(void) {
    UI_SetScreen(&mainMenuScreen);
    MainMenu_ShowDisconnected();
}

static void UI_MenuRealmSelect_f(void) {
    UI_SetScreen(&mainMenuScreen);
    MainMenu_ShowRealmSelect();
}

static void UI_MenuOptionsGameplay_f(void) {
    UI_SetScreen(&optionsMenuScreen);
    OptionsMenu_ShowGameplay();
}

static void UI_MenuOptionsSound_f(void) {
    UI_SetScreen(&optionsMenuScreen);
    OptionsMenu_ShowSound();
}

static void UI_MenuOptionsApply_f(void) {
    OptionsMenu_Apply();
    UI_MenuMain_f();
}

static void UI_MenuSinglePlayerCampaign_f(void) {
    if (UI_GetCurrentScreen() == &singlePlayerMenuScreen) {
        M_TransitionToAction(SinglePlayerMenu_ShowCampaign);
        return;
    }
    UI_SetScreen(&singlePlayerMenuScreen);
    SinglePlayerMenu_ShowCampaign();
}

static void UI_MenuSinglePlayerSkirmish_f(void) {
    M_ShowSinglePlayerSkirmishMenu();
}

static void UI_MenuLANRefresh_f(void) {
    LAN_RefreshMaps();
}

static void UI_MenuLANStart_f(void) {
    LAN_StartSelectedMap();
}

static void UI_MenuLANJoin_f(void) {
    LAN_JoinSelectedGame();
}

static void UI_MenuGameSetupStart_f(void) {
    if (GameSetup_StartGame()) {
        UI_ClearScreen();
    }
}

static void UI_MenuInGame_f(void) {
    UI_ClearScreen();
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
    { "menu_disconnected", UI_MenuDisconnected_f },
    { "menu_realm_select", UI_MenuRealmSelect_f },
    { "menu_options_gameplay", UI_MenuOptionsGameplay_f },
    { "menu_options_sound", UI_MenuOptionsSound_f },
    { "menu_options_apply", UI_MenuOptionsApply_f },
    { "menu_single_player_campaign", UI_MenuSinglePlayerCampaign_f },
    { "menu_single_player_skirmish", UI_MenuSinglePlayerSkirmish_f },
    { "menu_lan_refresh", UI_MenuLANRefresh_f },
    { "menu_lan_start", UI_MenuLANStart_f },
    { "menu_lan_join", UI_MenuLANJoin_f },
    { "menu_game_setup_start", UI_MenuGameSetupStart_f },
    { "menu_ingame", UI_MenuInGame_f },
    { NULL, NULL },
};

static void UI_RegisterMenuCommands(void) {
    if (ui_menu_commands_registered || !mi.Cmd_AddCommand) {
        return;
    }
    for (uiMenuCommandDef_t const *cmd = ui_menu_command_defs; cmd->command; cmd++) {
        mi.Cmd_AddCommand(cmd->command, cmd->function);
    }
    ui_menu_commands_registered = true;
}

static void UI_ClearScreen(void) {
    UI_SetScreen(NULL);
}

/* Refresh frame state flags before dispatch so draw never asks for mouse position. */
static void UI_UpdateMouseFrameFlags(LPCFRAMEDEF hit, BOOL clear_pressed) {
    FOR_LOOP(i, MAX_UI_CLASSES) {
        LPFRAMEDEF frame = &frames[i];
        if (!frame->inuse) {
            continue;
        }
        frame->ui_flags &= ~(UIFLAG_HOVERED | UIFLAG_ACTIVE);
        if (clear_pressed) {
            frame->ui_flags &= ~UIFLAG_PRESSED;
        }
        if (frame->hidden) frame->ui_flags &= ~UIFLAG_VISIBLE;
        else frame->ui_flags |= UIFLAG_VISIBLE;
        if (frame->disabled) frame->ui_flags |= UIFLAG_DISABLED;
        else frame->ui_flags &= ~UIFLAG_DISABLED;
    }
    if (hit) {
        ((LPFRAMEDEF)hit)->ui_flags |= UIFLAG_HOVERED | UIFLAG_ACTIVE;
    }
}

void M_Init(void) {
    memset(&ui_state, 0, sizeof(ui_state));
    UI_ResetGlueSceneModels();
    UI_RegisterMenuCommands();
    
    mi.Printf("M_Init: loading FDF assets\n");

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
    UI_ParseFDF("UI\\FrameDef\\Glue\\LocalMultiplayerJoin.fdf");
    UI_ParseFDF("UI\\FrameDef\\Glue\\LocalMultiplayerCreate.fdf");
    UI_ParseFDF("UI\\FrameDef\\Glue\\TeamSetup.fdf");
    UI_ParseFDF("UI\\FrameDef\\Glue\\PlayerSlot.fdf");
    UI_ParseFDF("UI\\FrameDef\\Glue\\GameChatroom.fdf");
    
    ui_state.initialized = true;
    ui_state.active = true;
    
    /*
     * Map launches use the server-authored in-game HUD via svc_layout.  Leave
     * the client-side menu screen idle there so no glue screen covers the game.
     */
    LPCSTR map = mi.Cvar_String
        ? mi.Cvar_String("map", "")
        : "";
    if (map && *map) {
        UI_ClearScreen();
        return;
    }

}

void M_Shutdown(void) {
    UI_SetScreen(NULL);
    UI_ReleaseGlueSceneModels();
    UI_ReleaseAssets();
    UI_ClearTemplates();
    memset(&ui_state, 0, sizeof(ui_state));
}

void M_SetActive(BOOL active) {
    ui_state.active = active;
}

DWORD M_Time(void) {
    return ui_state.time;
}

BOOL M_IsTransitioning(void) {
    return ui_state.transition_screen || ui_state.transition_action;
}

void M_Refresh(DWORD time) {
    if (!ui_state.active) {
        return;
    }

    ui_state.time = time;

    /* Call current screen refresh */
    uiScreen_t *screen = UI_GetCurrentScreen();
    if (screen && screen->refresh) {
        screen->refresh((int)time);
    }

    UI_DrawGlueScene();
    screen = UI_GetCurrentScreen();
    if (screen && screen->draw)
        screen->draw();
}

void M_KeyEvent(int key, BOOL down, DWORD time) {
    (void)time;

    if (!ui_state.active || ui_state.transition_screen || ui_state.transition_action) {
        return;
    }

    if (down && M_EditKey(key)) {
        return;
    }
    
    /* Delegate to current screen */
    uiScreen_t *screen = UI_GetCurrentScreen();
    if (screen && screen->key_event) {
        screen->key_event(key, down);
    }
}

/* Convert pixel coordinates to FDF/UI space for hit testing */
static VECTOR2 UI_PixelToFdf(int px, int py) {
    LPRENDERER renderer = mi.GetRenderer();
    size2_t window = renderer && renderer->GetWindowSize ? renderer->GetWindowSize() : MAKE(size2_t, 0, 0);
    RECT scene = UI_GetSceneRect();
    FLOAT nx = 0;
    FLOAT ny = 0;

    if (window.width > 0 && window.height > 0) {
        nx = (FLOAT)px / (FLOAT)window.width;
        ny = (FLOAT)py / (FLOAT)window.height;
    }
    return MAKE(VECTOR2, scene.x + nx * scene.w, scene.y + ny * scene.h);
}

/* All UI mouse work starts here so draw code only consumes event-updated state. */
BOOL M_MouseEvent(menuMouseEvent_t event, int x, int y, int32_t param) {
    BOOL const down = event == MENU_MOUSE_DOWN;
    BOOL const up = event == MENU_MOUSE_UP;
    BOOL const left = param == 1;
    int const wheel_y = event == MENU_MOUSE_SCROLL ? MENU_MOUSE_PARAM_Y(param) : 0;
    /* In the initialized runtime, a current screen is the ownership token for
     * standalone FDF input. Gameplay clears the screen, but menu_render.c keeps
     * the previous layout cache; never hit-test those stale invisible frames.
     * Uninitialized unit tests may exercise the low-level FDF event path
     * directly without installing a screen controller. */
    if (!ui_state.active || ui_state.transition_screen || ui_state.transition_action ||
        (ui_state.initialized && !UI_GetCurrentScreen())) {
        return false;
    }

    VECTOR2 fdf = UI_PixelToFdf(x, y);
    ui_state.mouse_fdf = fdf;
    LPCFRAMEDEF hit = UI_HitTest(fdf.x, fdf.y);
    UI_UpdateMouseFrameFlags(hit, up && left);

    /* Dispatch to per-type event handler */
    if (hit && hit->event_handler) {
        hit->event_handler((LPFRAMEDEF)hit, event, fdf.x, fdf.y, param);
    }

    /* Global: editbox clear focus on miss (LEFT_DOWN outside any editbox) */
    if (down && left) {
        BOOL hit_editbox = hit && (hit->Type == FT_EDITBOX || hit->Type == FT_GLUEEDITBOX ||
                                   hit->Type == FT_SLASHCHATBOX);
        if (!hit_editbox) {
            UI_EditboxClearFocusOnMiss();
        }
    }

    /* Global: slider drag tracking (motion when no frame hit) */
    if (UI_SliderIsDragging() && event == MENU_MOUSE_MOVE) {
        UI_SliderUpdateDrag(UI_SliderActiveFrame(), fdf.x, fdf.y);
    }
    if (up && left) {
        UI_SliderEndDrag(NULL);
    }

    /* Global: popup close on outside click */
    if (down && left && UI_HasActivePopup() && !UI_PopupPointInside(fdf.x, fdf.y)) {
        UI_PopupCloseOnMiss();
    }

    /* Global: popup menu wheel scroll */
    if (UI_HasActivePopup() && wheel_y > 0) {
        UI_PopupMenuScroll(true);
    }
    if (UI_HasActivePopup() && wheel_y < 0) {
        UI_PopupMenuScroll(false);
    }
    if (UI_HasActivePopup() && up && left) {
        UI_PopupSelectItem(fdf.x, fdf.y);
    }

    UI_PopupMenuHover(fdf.x, fdf.y);

    return hit != NULL;
}

void M_MenuCommand(LPCSTR command) {
    DWORD index;
    DWORD slot;
    DWORD value;
    char map_path[MAX_PATHLEN];

    mi.Printf("M_MenuCommand: %s\n", command);

    if (!command || !*command) {
        return;
    }

    if (!strcmp(command, "menu_main")) {
        UI_MenuMain_f();
        return;
    }
    if (!strcmp(command, "menu_game")) {
        UI_MenuGame_f();
        return;
    }
    if (!strcmp(command, "menu_multiplayer")) {
        UI_MenuMultiplayer_f();
        return;
    }
    if (!strcmp(command, "menu_startserver")) {
        UI_MenuStartServer_f();
        return;
    }
    if (!strcmp(command, "menu_joinserver")) {
        UI_MenuJoinServer_f();
        return;
    }
    if (!strcmp(command, "menu_options")) {
        UI_MenuOptions_f();
        return;
    }
    if (!strcmp(command, "menu_video")) {
        UI_MenuVideo_f();
        return;
    }
    if (!strcmp(command, "menu_keys")) {
        UI_MenuKeys_f();
        return;
    }
    if (!strcmp(command, "menu_credits")) {
        UI_MenuCredits_f();
        return;
    }
    if (!strcmp(command, "menu_quit")) {
        UI_MenuQuit_f();
        return;
    }
    if (!strcmp(command, "menu_realm_select")) {
        UI_MenuRealmSelect_f();
        return;
    }
    if (!strcmp(command, "menu_edition")) {
        MainMenu_BeginEditionSwitch();
        return;
    }
    if (!strcmp(command, "menu_options_gameplay")) {
        UI_MenuOptionsGameplay_f();
        return;
    }
    if (!strcmp(command, "menu_options_sound")) {
        UI_MenuOptionsSound_f();
        return;
    }
    if (!strcmp(command, "menu_options_apply")) {
        UI_MenuOptionsApply_f();
        return;
    }
    if (sscanf(command, "menu_video_mode %u", &value) == 1) {
        char video_command[96];

        if (!mi.Cmd_ExecuteText) {
            return;
        }
        if (value == video_mode_count()) {
            mi.Cmd_ExecuteText("seta vid_native 1\nseta vid_fullscreen 1\n");
        } else if (value < video_mode_count()) {
            snprintf(video_command,
                     sizeof(video_command),
                     "seta vid_native 0\nseta vid_mode %u\n",
                     (unsigned)value);
            mi.Cmd_ExecuteText(video_command);
        }
        return;
    }
    if (!strcmp(command, "menu_single_player_campaign")) {
        UI_MenuSinglePlayerCampaign_f();
        return;
    }
    if (!strcmp(command, "menu_single_player_skirmish")) {
        UI_MenuSinglePlayerSkirmish_f();
        return;
    }
    if (!strcmp(command, "menu_single_player_campaign_back")) {
        SinglePlayerMenu_BackCampaign();
        return;
    }
    if (!strcmp(command, "menu_single_player_campaign_human")) {
        SinglePlayerMenu_LaunchCampaign("human");
        return;
    }
    if (!strcmp(command, "menu_single_player_campaign_orc")) {
        SinglePlayerMenu_LaunchCampaign("orc");
        return;
    }
    if (!strcmp(command, "menu_single_player_campaign_undead")) {
        SinglePlayerMenu_LaunchCampaign("undead");
        return;
    }
    if (!strcmp(command, "menu_single_player_campaign_night_elf")) {
        SinglePlayerMenu_LaunchCampaign("night-elf");
        return;
    }
    if (!strcmp(command, "menu_single_player_campaign_tutorial")) {
        SinglePlayerMenu_LaunchCampaign("tutorial");
        return;
    }
    if (sscanf(command, "menu_single_player_campaign_select %u", &value) == 1) {
        SinglePlayerMenu_LaunchCampaignIndex(value);
        return;
    }
    if (sscanf(command, "menu_single_player_mission_select %u", &value) == 1) {
        SinglePlayerMenu_LaunchMissionIndex(value);
        return;
    }
    if (sscanf(command, "menu_single_player_difficulty %u", &value) == 1) {
        SinglePlayerMenu_SetDifficulty(value);
        return;
    }
    if (!strcmp(command, "menu_lan_refresh")) {
        UI_MenuLANRefresh_f();
        return;
    }
    if (!strcmp(command, "menu_lan_start")) {
        UI_MenuLANStart_f();
        return;
    }
    if (!strcmp(command, "menu_lan_join")) {
        UI_MenuLANJoin_f();
        return;
    }
    if (sscanf(command, "menu_lan_select %u", &index) == 1) {
        LAN_SelectMapIndex(index);
        return;
    }
    if (!strcmp(command, "menu_game_setup_start")) {
        UI_MenuGameSetupStart_f();
        return;
    }
    if (!strcmp(command, "menu_ingame")) {
        UI_MenuInGame_f();
        return;
    }
    if (sscanf(command, "menu_game_setup_slot_type %u %u", &slot, &value) == 2) {
        GameSetup_SetSlotType(slot, value);
        return;
    }
    if (sscanf(command, "menu_game_setup_slot_race %u %u", &slot, &value) == 2) {
        GameSetup_SetSlotRace(slot, value);
        return;
    }
    if (sscanf(command, "menu_game_setup_slot_team_next %u", &slot) == 1) {
        GameSetup_CycleSlotTeam(slot);
        return;
    }
    if (sscanf(command, "menu_game_setup_slot_color_next %u", &slot) == 1) {
        GameSetup_CycleSlotColor(slot);
        return;
    }
    if (sscanf(command, "menu_game_setup_map %255[^\n]", map_path) == 1) {
        UI_SetScreen(&gameSetupScreen);
        GameSetup_LoadMap(map_path);
        return;
    }
    if (!strncmp(command, "menu_game_setup_chat ", 21)) {
        DWORD own = 0;
        LPCSTR text = command + 21;

        if (sscanf(text, "%u", &own) == 1) {
            while (*text && *text != ' ' && *text != '\t') {
                text++;
            }
            while (*text == ' ' || *text == '\t') {
                text++;
            }
        }
        GameSetup_AddChatMessage(text, own != 0);
        return;
    }

    if (UI_IsMapCommand(command)) {
        UI_ClearScreen();
    }
    mi.Cmd_ExecuteText(command);
}

/* Stub callbacks for server data updates */
/* Forward unit UI data to active screen (Phase 8) */
void M_UpdateUnitUI(DWORD num_units, menuUnitData_t *units) {
    mi.Printf("UI_UpdateUnitUI: %d units\n", (int)num_units);
    
    /* Forward to current screen if it implements unit UI handling */
    uiScreen_t *screen = UI_GetCurrentScreen();
    if (screen && screen->update_unit_ui) {
        screen->update_unit_ui(num_units, units);
    }
}

static void M_UpdateLobbySetup(lobbyState_t const *state) {
    /* No current standalone screen means loading/gameplay owns presentation;
     * late lobby packets must not resurrect the game-setup glue screen. */
    if (!UI_GetCurrentScreen()) {
        return;
    }
    UI_SetScreen(&gameSetupScreen);
    GameSetup_UpdateLobbySetup(state);
}

/* Export function table */
menuExport_t M_GetAPI(menuImport_t import) {
    mi = import;
    
    menuExport_t exp;
    memset(&exp, 0, sizeof(exp));
    
    exp.Init = M_Init;
    exp.Shutdown = M_Shutdown;
    exp.Refresh = M_Refresh;
    exp.KeyEvent = M_KeyEvent;
    exp.TextInput = M_TextInput;
    exp.MouseEvent = M_MouseEvent;
    exp.UpdateUnitUI = M_UpdateUnitUI;
    exp.UpdatePlayerState = M_UpdatePlayerState;
    exp.UpdateLobbySetup = M_UpdateLobbySetup;
    exp.ResolveImagePath = M_ResolveImagePath;
    
    return exp;
}
