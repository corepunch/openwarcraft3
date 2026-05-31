/*
 * ui/screens/game_setup.c - LAN game lobby/player setup screen.
 */

#include <stdlib.h>

#ifndef _WIN32
#include <strings.h>
#endif

#include "../ui_local.h"
#include "../ui_screen.h"

typedef struct {
    LPFRAMEDEF row;
    LPFRAMEDEF name_menu;
    LPFRAMEDEF race_menu;
    LPFRAMEDEF name_menu_items;
    LPFRAMEDEF race_menu_items;
    LPFRAMEDEF name_text;
    LPFRAMEDEF race_text;
    LPFRAMEDEF team_text;
    LPFRAMEDEF color_value;
} gameSetupSlotRow_t;

typedef enum {
    GAME_SETUP_SLOT_OPEN,
    GAME_SETUP_SLOT_HUMAN,
    GAME_SETUP_SLOT_COMPUTER,
    GAME_SETUP_SLOT_CLOSED,
} gameSetupSlotType_t;

typedef struct {
    BOOL visible;
    gameSetupSlotType_t type;
    playerRace_t race;
    DWORD team;
    DWORD color;
    UINAME name;
} gameSetupSlotConfig_t;

typedef struct {
    BOOL ready;
    PATHSTR map_path;
    UINAME map_name;
    MAPINFO map_info;
    BOOL have_map_info;
    LPFRAMEDEF root;
    LPFRAMEDEF game_name;
    LPFRAMEDEF start_button;
    LPFRAMEDEF cancel_button;
    LPFRAMEDEF team_container;
    LPFRAMEDEF info_root;
    LPFRAMEDEF info_players;
    LPFRAMEDEF info_name;
    LPFRAMEDEF info_suggested;
    LPFRAMEDEF info_size;
    LPFRAMEDEF info_tileset;
    LPFRAMEDEF info_description;
    LPFRAMEDEF info_minimap;
    gameSetupSlotRow_t slots[MAX_PLAYERS];
    gameSetupSlotConfig_t configs[MAX_PLAYERS];
} gameSetupState_t;

static gameSetupState_t setup;

static LPCSTR GameSetup_BaseName(LPCSTR path) {
    LPCSTR base = path;

    if (!path) {
        return "";
    }
    for (LPCSTR p = path; *p; p++) {
        if (*p == '\\' || *p == '/') {
            base = p + 1;
        }
    }
    return base;
}

static LPFRAMEDEF GameSetup_FindPopupTitleText(LPFRAMEDEF popup) {
    LPFRAMEDEF title;
    LPFRAMEDEF text;

    if (!popup) {
        return NULL;
    }
    title = UI_FindChildFrame(popup, popup->Popup.TitleFrame);
    text = title ? UI_FindChildFrame(title, "StandardPopupMenuTitleTextTemplate") : NULL;
    return text ? text : title;
}

static void GameSetup_PositionPopupMenuParts(LPFRAMEDEF popup) {
    LPFRAMEDEF title;
    LPFRAMEDEF arrow;
    FLOAT const inset = popup ? popup->Popup.ButtonInset : 0.0f;
    FLOAT arrow_width;
    FLOAT title_width;

    if (!popup) {
        return;
    }

    title = UI_FindChildFrame(popup, popup->Popup.TitleFrame);
    arrow = UI_FindChildFrame(popup, popup->Popup.ArrowFrame);
    arrow_width = arrow && arrow->Width > 0.0f ? arrow->Width : 0.011f;
    title_width = popup->Width - arrow_width - inset * 2.0f;

    if (title) {
        UI_SetSize(title, MAX(0.0f, title_width), popup->Height);
        UI_SetPoint(title, FRAMEPOINT_LEFT, popup, FRAMEPOINT_LEFT, inset, 0.0f);
    }
    if (arrow) {
        UI_SetPoint(arrow, FRAMEPOINT_RIGHT, popup, FRAMEPOINT_RIGHT, -inset, 0.0f);
    }
}

static void GameSetup_SetBackdropTexture(LPFRAMEDEF frame, LPCSTR name, BOOL decorate) {
    if (!frame) {
        return;
    }
    frame->Backdrop.Background = UI_LoadTexture(name, decorate);
    frame->Backdrop.BlendAll = true;
    frame->Color = COLOR32_WHITE;
}

static void GameSetup_SetTextIfPresent(LPFRAMEDEF frame, LPCSTR format, ...) {
    va_list argptr;
    char text[1024];

    if (!frame || !format) {
        return;
    }
    va_start(argptr, format);
    vsnprintf(text, sizeof(text), format, argptr);
    va_end(argptr);
    UI_SetText(frame, "%s", text);
}

static void GameSetup_HideSlotChild(LPFRAMEDEF row, LPCSTR name) {
    LPFRAMEDEF frame = row ? UI_FindChildFrame(row, name) : NULL;

    if (frame) {
        UI_SetHidden(frame, true);
    }
}

static void GameSetup_FitSlotRow(LPFRAMEDEF row) {
    LPFRAMEDEF download_value;
    LPFRAMEDEF name_menu;
    LPFRAMEDEF race_menu;
    LPFRAMEDEF team_button;
    LPFRAMEDEF color_button;

    if (!row) {
        return;
    }

    download_value = UI_FindChildFrame(row, "DownloadValue");
    name_menu = UI_FindChildFrame(row, "NameMenu");
    race_menu = UI_FindChildFrame(row, "RaceMenu");
    team_button = UI_FindChildFrame(row, "TeamButton");
    color_button = UI_FindChildFrame(row, "ColorButton");

    if (download_value) {
        UI_SetHidden(download_value, true);
    }
    if (name_menu) {
        UI_SetSize(name_menu, 0.168f, name_menu->Height);
        UI_SetPoint(name_menu, FRAMEPOINT_LEFT, row, FRAMEPOINT_LEFT, 0.0f, 0.0f);
    }
    if (race_menu && name_menu) {
        UI_SetSize(race_menu, 0.085f, race_menu->Height);
        UI_SetPoint(race_menu, FRAMEPOINT_LEFT, name_menu, FRAMEPOINT_RIGHT, 0.0f, 0.0f);
    }
    if (team_button && race_menu) {
        UI_SetSize(team_button, 0.085f, team_button->Height);
        UI_SetPoint(team_button, FRAMEPOINT_LEFT, race_menu, FRAMEPOINT_RIGHT, 0.0f, 0.0f);
    }
    if (color_button && team_button) {
        UI_SetSize(color_button, 0.038f, color_button->Height);
        UI_SetPoint(color_button, FRAMEPOINT_LEFT, team_button, FRAMEPOINT_RIGHT, 0.0f, 0.0f);
    }
}

static LPFRAMEDEF GameSetup_EnsureColorValue(LPFRAMEDEF row) {
    LPFRAMEDEF color_button = row ? UI_FindChildFrame(row, "ColorButton") : NULL;
    LPFRAMEDEF value = row ? UI_FindChildFrame(row, "ColorButtonValue") : NULL;

    if (value || !color_button) {
        return value;
    }

    value = UI_Spawn(FT_BACKDROP, color_button);
    if (!value) {
        return NULL;
    }
    snprintf(value->Name, sizeof(value->Name), "ColorButtonValue");
    value->Parent = color_button;
    UI_SetSize(value, 0.018f, 0.018f);
    UI_SetPoint(value, FRAMEPOINT_CENTER, color_button, FRAMEPOINT_CENTER, 0.0f, 0.0f);
    return value;
}

static void GameSetup_BindSlotRow(gameSetupSlotRow_t *slot) {
    LPFRAMEDEF handicap_menu;
    DWORD const index = (DWORD)(slot - setup.slots);

    if (!slot || !slot->row) {
        return;
    }

    slot->name_menu = UI_FindChildFrame(slot->row, "NameMenu");
    slot->race_menu = UI_FindChildFrame(slot->row, "RaceMenu");
    slot->name_menu_items = UI_FindChildFrame(slot->row, "NamePopupMenuMenu");
    slot->race_menu_items = UI_FindChildFrame(slot->row, "RacePopupMenuMenu");
    handicap_menu = UI_FindChildFrame(slot->row, "HandicapMenu");

    GameSetup_FitSlotRow(slot->row);
    slot->name_text = GameSetup_FindPopupTitleText(slot->name_menu);
    slot->race_text = GameSetup_FindPopupTitleText(slot->race_menu);
    slot->team_text = UI_FindChildFrame(slot->row, "TeamButtonTitle");
    slot->color_value = GameSetup_EnsureColorValue(slot->row);

    GameSetup_PositionPopupMenuParts(slot->name_menu);
    GameSetup_PositionPopupMenuParts(slot->race_menu);
    if (slot->name_menu_items) {
        UI_MenuClearItems(slot->name_menu_items);
        UI_MenuAddItem(slot->name_menu_items, "Open", GAME_SETUP_SLOT_OPEN);
        UI_MenuAddItem(slot->name_menu_items, "Player", GAME_SETUP_SLOT_HUMAN);
        UI_MenuAddItem(slot->name_menu_items, "Computer", GAME_SETUP_SLOT_COMPUTER);
        UI_MenuAddItem(slot->name_menu_items, "Closed", GAME_SETUP_SLOT_CLOSED);
        UI_SetOnClick(slot->name_menu_items, "menu /game-setup/slot/%u/type/%%u", (unsigned)index);
        UI_SetHidden(slot->name_menu_items, true);
    }
    if (slot->race_menu_items) {
        UI_MenuClearItems(slot->race_menu_items);
        UI_MenuAddItem(slot->race_menu_items, "Random", kPlayerRaceNone);
        UI_MenuAddItem(slot->race_menu_items, "Human", kPlayerRaceHuman);
        UI_MenuAddItem(slot->race_menu_items, "Orc", kPlayerRaceOrc);
        UI_MenuAddItem(slot->race_menu_items, "Undead", kPlayerRaceUndead);
        UI_MenuAddItem(slot->race_menu_items, "Night Elf", kPlayerRaceNightElf);
        UI_SetOnClick(slot->race_menu_items, "menu /game-setup/slot/%u/race/%%u", (unsigned)index);
        UI_SetHidden(slot->race_menu_items, true);
    }
    UI_SetOnClick(UI_FindChildFrame(slot->row, "TeamButton"),
                  "menu /game-setup/slot/%u/team/next",
                  (unsigned)index);
    UI_SetOnClick(UI_FindChildFrame(slot->row, "ColorButton"),
                  "menu /game-setup/slot/%u/color/next",
                  (unsigned)index);
    GameSetup_HideSlotChild(slot->row, "HandicapPopupMenuMenu");
    if (handicap_menu) {
        UI_SetHidden(handicap_menu, true);
    }
}

static void GameSetup_BuildSlotRows(void) {
    UI_FRAME(PlayerSlot);
    LPFRAMEDEF previous = NULL;

    if (!setup.team_container || !PlayerSlot || setup.slots[0].row) {
        return;
    }

    FOR_LOOP(i, MAX_PLAYERS) {
        gameSetupSlotRow_t *slot = &setup.slots[i];

        slot->row = UI_CloneFrameTree(PlayerSlot, setup.team_container);
        if (!slot->row) {
            return;
        }
        snprintf(slot->row->Name, sizeof(slot->row->Name), "CreateGamePlayerSlot%u", (unsigned)i);
        UI_SetSize(slot->row, 0.46375f, slot->row->Height > 0.0f ? slot->row->Height : 0.025f);
        if (previous) {
            UI_SetPoint(slot->row, FRAMEPOINT_TOPLEFT, previous, FRAMEPOINT_BOTTOMLEFT, 0.0f, 0.0f);
        } else {
            UI_SetPoint(slot->row,
                        FRAMEPOINT_TOPLEFT,
                        setup.team_container,
                        FRAMEPOINT_TOPLEFT,
                        0.0f,
                        0.0f);
        }
        previous = slot->row;
        GameSetup_BindSlotRow(slot);
        UI_SetHidden(slot->row, true);
    }
}

static void GameSetup_ClearSlots(void) {
    FOR_LOOP(i, MAX_PLAYERS) {
        if (setup.slots[i].row) {
            UI_SetHidden(setup.slots[i].row, true);
        }
    }
}

static LPCSTR GameSetup_RaceName(playerRace_t race) {
    switch (race) {
        case kPlayerRaceHuman: return "Human";
        case kPlayerRaceOrc: return "Orc";
        case kPlayerRaceUndead: return "Undead";
        case kPlayerRaceNightElf: return "Night Elf";
        case kPlayerRaceNone:
        default:
            return "Random";
    }
}

static LPCSTR GameSetup_SlotTypeName(gameSetupSlotType_t type, LPCSTR human_name) {
    switch (type) {
        case GAME_SETUP_SLOT_HUMAN:
            return human_name && human_name[0] ? human_name : "Player";
        case GAME_SETUP_SLOT_COMPUTER:
            return "Computer";
        case GAME_SETUP_SLOT_CLOSED:
            return "Closed";
        case GAME_SETUP_SLOT_OPEN:
        default:
            return "Open";
    }
}

static void GameSetup_ResolveMapString(LPCSTR raw, LPSTR out, DWORD out_size) {
    if (!out || out_size == 0) {
        return;
    }
    out[0] = '\0';
    if (!raw || !raw[0]) {
        return;
    }
    if (uiimport.ResolveMapInfoString) {
        uiimport.ResolveMapInfoString(&setup.map_info, raw, out, out_size);
    } else {
        snprintf(out, out_size, "%s", raw);
    }
    if (uiimport.SanitizeMapInfoText) {
        uiimport.SanitizeMapInfoText(out);
    }
}

static void GameSetup_UseResolvedMapTitle(void) {
    UINAME title;

    if (!setup.have_map_info || !setup.map_info.mapName) {
        return;
    }
    GameSetup_ResolveMapString(setup.map_info.mapName, title, sizeof(title));
    if (title[0]) {
        snprintf(setup.map_name, sizeof(setup.map_name), "%s", title);
    }
}

static void GameSetup_SlotName(LPCMAPPLAYER player, BOOL first_human, LPSTR out, DWORD out_size) {
    if (!player) {
        snprintf(out, out_size, "Open");
        return;
    }
    if (player->playerType == kPlayerTypeComputer) {
        snprintf(out, out_size, "Computer");
        return;
    }
    if (player->playerType == kPlayerTypeHuman && first_human) {
        snprintf(out, out_size, "Player");
        return;
    }
    GameSetup_ResolveMapString(player->playerName, out, out_size);
    if (!out[0]) {
        snprintf(out, out_size, "Open");
    }
}

static DWORD GameSetup_ForceForPlayer(LPCMAPINFO info, DWORD player_index) {
    if (!info || !info->teams) {
        return player_index;
    }

    FOR_LOOP(i, info->num_teams) {
        if (info->teams[i].playerMasks & (1u << player_index)) {
            return i;
        }
    }
    return player_index;
}

static void GameSetup_DrawSlotConfig(DWORD row_index) {
    gameSetupSlotRow_t *row;
    gameSetupSlotConfig_t *config;
    char color_path[128];
    char team[32];

    if (row_index >= MAX_PLAYERS) {
        return;
    }
    row = &setup.slots[row_index];
    config = &setup.configs[row_index];
    if (!row->row || !config->visible) {
        if (row->row) {
            UI_SetHidden(row->row, true);
        }
        return;
    }

    snprintf(team, sizeof(team), "Team %u", (unsigned)config->team + 1);
    GameSetup_SetTextIfPresent(row->name_text,
                               "%s",
                               GameSetup_SlotTypeName(config->type, config->name));
    GameSetup_SetTextIfPresent(row->race_text, "%s", GameSetup_RaceName(config->race));
    GameSetup_SetTextIfPresent(row->team_text, "%s", team);
    if (row->color_value) {
        snprintf(color_path, sizeof(color_path),
                 "ReplaceableTextures\\TeamColor\\TeamColor%02u.blp",
                 (unsigned)(config->color % 16));
        GameSetup_SetBackdropTexture(row->color_value, color_path, false);
    }
    UI_SetHidden(row->row, false);
}

static void GameSetup_DrawSlotConfigs(void) {
    FOR_LOOP(i, MAX_PLAYERS) {
        GameSetup_DrawSlotConfig(i);
    }
}

static void GameSetup_PopulateSlots(void) {
    DWORD visible = 0;
    BOOL first_human = true;

    GameSetup_ClearSlots();
    memset(setup.configs, 0, sizeof(setup.configs));
    if (!setup.have_map_info) {
        return;
    }

    FOR_LOOP(i, MAX_PLAYERS) {
        mapPlayer_t const *player = &setup.map_info.players[i];
        char name[96];
        DWORD force_index;
        BOOL is_first_human;

        if (!player->used ||
            player->playerType == kPlayerTypeNone ||
            player->playerType == kPlayerTypeNeutral) {
            continue;
        }
        if (visible >= MAX_PLAYERS) {
            break;
        }

        is_first_human = player->playerType == kPlayerTypeHuman && first_human;
        if (is_first_human) {
            first_human = false;
        }

        force_index = GameSetup_ForceForPlayer(&setup.map_info, i);
        GameSetup_SlotName(player, is_first_human, name, sizeof(name));

        setup.configs[visible].visible = true;
        setup.configs[visible].type = player->playerType == kPlayerTypeComputer
            ? GAME_SETUP_SLOT_COMPUTER
            : GAME_SETUP_SLOT_HUMAN;
        setup.configs[visible].race = player->playerRace;
        setup.configs[visible].team = force_index;
        setup.configs[visible].color = i;
        snprintf(setup.configs[visible].name, sizeof(setup.configs[visible].name), "%s", name);
        visible++;
    }
    GameSetup_DrawSlotConfigs();
}

static void GameSetup_BindMapInfoPane(LPFRAMEDEF container) {
    LPFRAMEDEF source;

    if (setup.info_root || !container) {
        return;
    }

    source = UI_FindFrame("MapInfoPane");
    if (!source) {
        uiimport.Printf("GameSetup_BindMapInfoPane: MapInfoPane missing\n");
        return;
    }

    setup.info_root = UI_CloneFrameTree(source, container);
    if (!setup.info_root) {
        return;
    }
    UI_SetPoint(setup.info_root, FRAMEPOINT_TOPLEFT, container, FRAMEPOINT_TOPLEFT, 0.0f, 0.0f);
    UI_SetPoint(setup.info_root, FRAMEPOINT_BOTTOMRIGHT, container, FRAMEPOINT_BOTTOMRIGHT, 0.0f, 0.0f);
    UI_LayoutMapInfoPane(setup.info_root);

    UI_FRAME(setup.info_root, MaxPlayersValue);
    UI_FRAME(setup.info_root, MapNameValue);
    UI_FRAME(setup.info_root, MinimapImage);
    UI_FRAME(setup.info_root, SuggestedPlayersValue);
    UI_FRAME(setup.info_root, MapSizeValue);
    UI_FRAME(setup.info_root, MapTilesetValue);
    UI_FRAME(setup.info_root, MapDescValue);

    setup.info_players = MaxPlayersValue;
    setup.info_name = MapNameValue;
    setup.info_minimap = MinimapImage;
    setup.info_suggested = SuggestedPlayersValue;
    setup.info_size = MapSizeValue;
    setup.info_tileset = MapTilesetValue;
    setup.info_description = MapDescValue;
}

static DWORD GameSetup_CountPlayers(LPCMAPINFO info) {
    DWORD count = 0;

    FOR_LOOP(i, MAX_PLAYERS) {
        if (info && info->players[i].used &&
            info->players[i].playerType != kPlayerTypeNone &&
            info->players[i].playerType != kPlayerTypeNeutral) {
            count++;
        }
    }
    return count;
}

static void GameSetup_UpdateMapInfo(void) {
    char value[512];
    LPCSTR tileset;

    if (!setup.have_map_info) {
        if (setup.info_minimap) {
            UI_SetHidden(setup.info_minimap, true);
        }
        GameSetup_SetTextIfPresent(setup.info_players, " ");
        GameSetup_SetTextIfPresent(setup.info_name, "%s", setup.map_name[0] ? setup.map_name : " ");
        GameSetup_SetTextIfPresent(setup.info_suggested, "%s", UI_GetString("UNKNOWNMAP_SUGGESTEDPLAYERS"));
        GameSetup_SetTextIfPresent(setup.info_size, "%s", UI_GetString("UNKNOWNMAP_MAPSIZE"));
        GameSetup_SetTextIfPresent(setup.info_tileset, "%s", UI_GetString("UNKNOWNMAP_TILESET"));
        GameSetup_SetTextIfPresent(setup.info_description, "%s", UI_GetString("UNKNOWNMAP_DESCRIPTION"));
        return;
    }

    GameSetup_SetTextIfPresent(setup.info_players, "%u", (unsigned)GameSetup_CountPlayers(&setup.map_info));
    GameSetup_SetTextIfPresent(setup.info_name, "%s", setup.map_name[0] ? setup.map_name : setup.map_path);

    uiimport.ResolveMapInfoString(&setup.map_info,
                                  setup.map_info.playersRecommended,
                                  value,
                                  sizeof(value));
    uiimport.SanitizeMapInfoText(value);
    GameSetup_SetTextIfPresent(setup.info_suggested, "%s", value);

    GameSetup_SetTextIfPresent(setup.info_size,
                               "%s",
                               uiimport.MapSizeName(setup.map_info.playableArea.width,
                                                    setup.map_info.playableArea.height));

    tileset = uiimport.MapTilesetName((BYTE)setup.map_info.mainGroundType);
    GameSetup_SetTextIfPresent(setup.info_tileset, "%s", tileset ? tileset : UI_GetString("UNKNOWNMAP_TILESET"));

    uiimport.ResolveMapInfoString(&setup.map_info,
                                  setup.map_info.mapDescription,
                                  value,
                                  sizeof(value));
    uiimport.SanitizeMapInfoText(value);
    GameSetup_SetTextIfPresent(setup.info_description, "%s", value[0] ? value : UI_GetString("UNKNOWNMAP_DESCRIPTION"));

    if (setup.info_minimap && uiimport.FindMapPreviewTexture) {
        PATHSTR preview;

        if (uiimport.FindMapPreviewTexture(setup.map_path, preview, sizeof(preview))) {
            UI_SetTexture(setup.info_minimap, preview, false);
            UI_SetHidden(setup.info_minimap, false);
        } else {
            UI_SetHidden(setup.info_minimap, true);
        }
    }
}

static void GameSetup_LoadSelectedMap(void) {
    LPCSTR selected_path = LAN_SelectedMapPath();
    LPCSTR selected_name = LAN_SelectedMapName();
    LPCSTR debug_path = NULL;

    if (setup.have_map_info) {
        uiimport.FreeMapInfo(&setup.map_info);
        setup.have_map_info = false;
    }
    setup.map_path[0] = '\0';
    setup.map_name[0] = '\0';

    if (selected_path) {
        snprintf(setup.map_path, sizeof(setup.map_path), "%s", selected_path);
    }
    if (selected_name) {
        snprintf(setup.map_name, sizeof(setup.map_name), "%s", selected_name);
    }

    if (!setup.map_path[0] && uiimport.Cvar_String) {
        debug_path = uiimport.Cvar_String("ui_game_setup_map", "");
        if (debug_path && debug_path[0]) {
            snprintf(setup.map_path, sizeof(setup.map_path), "%s", debug_path);
            snprintf(setup.map_name, sizeof(setup.map_name), "%s", GameSetup_BaseName(debug_path));
        }
    }

    if (setup.map_path[0] && uiimport.ReadMapInfo) {
        setup.have_map_info = uiimport.ReadMapInfo(setup.map_path, &setup.map_info);
        GameSetup_UseResolvedMapTitle();
    }
}

static void GameSetup_LoadRouteMap(LPCSTR path) {
    LPCSTR map_path;

    if (!path || strncmp(path, "?map=", 5)) {
        return;
    }
    map_path = path + 5;
    if (!map_path[0]) {
        return;
    }

    if (setup.have_map_info) {
        uiimport.FreeMapInfo(&setup.map_info);
        setup.have_map_info = false;
    }
    snprintf(setup.map_path, sizeof(setup.map_path), "%s", map_path);
    snprintf(setup.map_name, sizeof(setup.map_name), "%s", GameSetup_BaseName(map_path));
    if (uiimport.ReadMapInfo) {
        setup.have_map_info = uiimport.ReadMapInfo(setup.map_path, &setup.map_info);
        GameSetup_UseResolvedMapTitle();
    }
    GameSetup_SetTextIfPresent(setup.game_name, "%s", setup.map_name[0] ? setup.map_name : "Local Game");
    GameSetup_UpdateMapInfo();
    GameSetup_PopulateSlots();
}

static void GameSetup_BuildFrames(void) {
    setup.ready = false;
    setup.root = UI_FindFrame("GameChatroom");
    if (!setup.root) {
        uiimport.Printf("GameSetup_BuildFrames: GameChatroom missing\n");
        return;
    }
    UI_SetAllPoints(setup.root);

    setup.game_name = UI_FindChildFrame(setup.root, "GameNameValue");
    setup.start_button = UI_FindChildFrame(setup.root, "StartGameButton");
    setup.cancel_button = UI_FindChildFrame(setup.root, "CancelButton");
    setup.team_container = UI_FindChildFrame(setup.root, "TeamSetupContainer");
    UI_FRAME(setup.root, MapInfoPaneContainer);

    GameSetup_BuildSlotRows();
    GameSetup_BindMapInfoPane(MapInfoPaneContainer);

    UI_SetOnClick(setup.start_button, "menu /game-setup/start");
    UI_SetOnClick(setup.cancel_button, "menu_startserver");
    setup.ready = true;
}

static void GameSetup_Init(void) {
    uiimport.Printf("GameSetup_Init\n");
    UI_PreloadGlueSceneModels();
    if (!setup.root) {
        GameSetup_BuildFrames();
    }
    if (!setup.ready) {
        return;
    }
    GameSetup_LoadSelectedMap();
    GameSetup_SetTextIfPresent(setup.game_name, "%s", setup.map_name[0] ? setup.map_name : "Local Game");
    GameSetup_UpdateMapInfo();
    GameSetup_PopulateSlots();
}

static void GameSetup_Shutdown(void) {
}

static void GameSetup_Refresh(int msec) {
    (void)msec;
}

static void GameSetup_Draw(void) {
    if (!setup.ready) {
        return;
    }

    UI_DrawGlueScene("MultiplayerPreGameChat Stand");
    UI_DrawFrame(setup.root);
}

static void GameSetup_KeyEvent(int key, BOOL down) {
    (void)key;
    (void)down;
}

static void GameSetup_MouseEvent(int x, int y, int buttons) {
    (void)x;
    (void)y;
    (void)buttons;
}

static void GameSetup_StartGame(void) {
    char command[MAX_PATHLEN + 32];

    if (!setup.map_path[0]) {
        return;
    }
    snprintf(command, sizeof(command), "map \"%s\"\n", setup.map_path);
    uiimport.Cmd_ExecuteText(command);
}

static void GameSetup_SetSlotType(DWORD slot, DWORD value) {
    if (slot >= MAX_PLAYERS || !setup.configs[slot].visible) {
        return;
    }
    if (value > GAME_SETUP_SLOT_CLOSED) {
        return;
    }
    setup.configs[slot].type = (gameSetupSlotType_t)value;
    if (setup.configs[slot].type == GAME_SETUP_SLOT_HUMAN && !setup.configs[slot].name[0]) {
        snprintf(setup.configs[slot].name, sizeof(setup.configs[slot].name), "Player");
    }
    GameSetup_DrawSlotConfig(slot);
}

static void GameSetup_SetSlotRace(DWORD slot, DWORD value) {
    if (slot >= MAX_PLAYERS || !setup.configs[slot].visible || value > kPlayerRaceNightElf) {
        return;
    }
    setup.configs[slot].race = (playerRace_t)value;
    GameSetup_DrawSlotConfig(slot);
}

static void GameSetup_CycleSlotTeam(DWORD slot) {
    DWORD max_teams;

    if (slot >= MAX_PLAYERS || !setup.configs[slot].visible) {
        return;
    }
    max_teams = setup.have_map_info && setup.map_info.num_teams > 0
        ? setup.map_info.num_teams
        : MAX_PLAYERS;
    setup.configs[slot].team = (setup.configs[slot].team + 1) % MAX(1, max_teams);
    GameSetup_DrawSlotConfig(slot);
}

static void GameSetup_CycleSlotColor(DWORD slot) {
    if (slot >= MAX_PLAYERS || !setup.configs[slot].visible) {
        return;
    }
    setup.configs[slot].color = (setup.configs[slot].color + 1) % 16;
    GameSetup_DrawSlotConfig(slot);
}

static BOOL GameSetup_HandleSlotRoute(LPCSTR path) {
    unsigned slot = 0;
    unsigned value = 0;

    if (!path || strncmp(path, "/slot/", 6)) {
        return false;
    }
    if (sscanf(path, "/slot/%u/type/%u", &slot, &value) == 2) {
        GameSetup_SetSlotType(slot, value);
        return true;
    }
    if (sscanf(path, "/slot/%u/race/%u", &slot, &value) == 2) {
        GameSetup_SetSlotRace(slot, value);
        return true;
    }
    if (sscanf(path, "/slot/%u/team/next", &slot) == 1) {
        GameSetup_CycleSlotTeam(slot);
        return true;
    }
    if (sscanf(path, "/slot/%u/color/next", &slot) == 1) {
        GameSetup_CycleSlotColor(slot);
        return true;
    }
    return false;
}

static void GameSetup_Route(LPCSTR path) {
    if (!setup.ready) {
        return;
    }

    if (path && !strcmp(path, "/start")) {
        GameSetup_StartGame();
    } else if (path && !strncmp(path, "?map=", 5)) {
        GameSetup_LoadRouteMap(path);
    } else if (GameSetup_HandleSlotRoute(path)) {
        return;
    }
}

uiScreen_t gameSetupScreen = {
    .name = "game-setup",
    .init = GameSetup_Init,
    .shutdown = GameSetup_Shutdown,
    .refresh = GameSetup_Refresh,
    .draw = GameSetup_Draw,
    .key_event = GameSetup_KeyEvent,
    .mouse_event = GameSetup_MouseEvent,
    .route = GameSetup_Route,
};
