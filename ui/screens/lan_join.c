/*
 * ui/screens/lan_join.c - LAN create-game map selection screen.
 */

#include <stdlib.h>

#ifndef _WIN32
#include <strings.h>
#endif

#include "../ui_local.h"
#include "../ui_screen.h"
#include "../generated/local_multiplayer_create.h"

#define LAN_VISIBLE_MAPS 14
#define LAN_FILELIST_SIZE 32768

typedef struct lan_join_state_s {
    LocalMultiplayerCreate_t frames;
    uiMapListState_t maps;
    BOOL ready;
    LPFRAMEDEF root;
    LPFRAMEDEF play_button;
    LPFRAMEDEF map_list_box;
    LPFRAMEDEF info_root;
    LPFRAMEDEF info_players;
    LPFRAMEDEF info_name;
    LPFRAMEDEF info_suggested;
    LPFRAMEDEF info_size;
    LPFRAMEDEF info_tileset;
    LPFRAMEDEF info_description;
    LPFRAMEDEF info_minimap;
    LPFRAMEDEF game_speed_slider;
    LPFRAMEDEF game_speed_value;
} lan_join_state_t;

static lan_join_state_t lan;

static BOOL LANJoin_LoadScreen(void) {
    return LocalMultiplayerCreate_Load(&lan.frames);
}

static void LAN_CopyString(LPSTR out, size_t out_size, LPCSTR value) {
    if (!out || out_size == 0) {
        return;
    }
    snprintf(out, out_size, "%s", value ? value : "");
}

static void LAN_CopyTitleSubtitle(LPSTR out, size_t out_size, LPCSTR title, LPCSTR subtitle) {
    if (!out || out_size == 0) {
        return;
    }
    snprintf(out, out_size, "%s: %s", title ? title : "", subtitle ? subtitle : "");
}

static BOOL LAN_HasMapExtension(LPCSTR path) {
    LPCSTR dot = strrchr(path, '.');
    return dot && (!strcasecmp(dot, ".w3m") || !strcasecmp(dot, ".w3x"));
}

static BOOL LAN_IsCampaignMap(LPCSTR path) {
    return path && (!strncasecmp(path, "Maps\\Campaign\\", 14) ||
                    !strncasecmp(path, "Maps/Campaign/", 14) ||
                    !strncasecmp(path, "Maps\\FrozenThrone\\Campaign\\", 26) ||
                    !strncasecmp(path, "Maps/FrozenThrone/Campaign/", 26));
}

static void LAN_SetMapDisplayName(uiMapListItem_t *item,
                                  LPCMAPINFO info)
{
    char name[128];
    char description[128];
    char loading_title[64];
    char loading_subtitle[96];

    uiimport.ResolveMapInfoString(info, info ? info->mapName : NULL, name, sizeof(name));
    uiimport.SanitizeMapListField(name);
    if (name[0]) {
        LAN_CopyString(item->name, sizeof(item->name), name);
    }

    if (strncasecmp(item->path, "Maps\\Campaign\\", 14) ||
        !uiimport.MapNameMatchesFile(item->name, item->path)) {
        return;
    }

    uiimport.ResolveMapInfoString(info, info ? info->mapDescription : NULL, description, sizeof(description));
    uiimport.ResolveMapInfoString(info, info ? info->loadingScreenTitle : NULL, loading_title, sizeof(loading_title));
    uiimport.ResolveMapInfoString(info, info ? info->loadingScreenSubtitle : NULL, loading_subtitle, sizeof(loading_subtitle));
    uiimport.SanitizeMapListField(description);
    uiimport.SanitizeMapListField(loading_title);
    uiimport.SanitizeMapListField(loading_subtitle);

    if (loading_title[0] && loading_subtitle[0]) {
        LAN_CopyTitleSubtitle(item->name, sizeof(item->name), loading_title, loading_subtitle);
    } else if (description[0]) {
        LAN_CopyString(item->name, sizeof(item->name), description);
    } else if (loading_title[0]) {
        LAN_CopyString(item->name, sizeof(item->name), loading_title);
    } else if (loading_subtitle[0]) {
        LAN_CopyString(item->name, sizeof(item->name), loading_subtitle);
    }
}

static DWORD LAN_CountMapPlayers(LPCMAPINFO info) {
    DWORD count = 0;

    FOR_LOOP(i, MAX_PLAYERS) {
        if (info && info->players[i].used) {
            count++;
        }
    }
    return count;
}

static BOOL LAN_ParseMapInfo(uiMapListItem_t *item) {
    MAPINFO info;
    LPCSTR tileset;

    if (!item || !uiimport.ReadMapInfo || !uiimport.FreeMapInfo ||
        !uiimport.ResolveMapInfoString || !uiimport.MapTilesetName ||
        !uiimport.MapSizeName || !uiimport.MapNameMatchesFile ||
        !uiimport.SanitizeMapListField || !uiimport.SanitizeMapInfoText) {
        return false;
    }
    if (!uiimport.ReadMapInfo(item->path, &info)) {
        return false;
    }

    LAN_SetMapDisplayName(item, &info);
    uiimport.ResolveMapInfoString(&info, info.mapDescription, item->description, sizeof(item->description));
    uiimport.ResolveMapInfoString(&info, info.playersRecommended, item->suggestedPlayers, sizeof(item->suggestedPlayers));
    uiimport.SanitizeMapInfoText(item->description);
    uiimport.SanitizeMapInfoText(item->suggestedPlayers);
    snprintf(item->mapSize, sizeof(item->mapSize), "%s", uiimport.MapSizeName(info.playableArea.width, info.playableArea.height));
    tileset = uiimport.MapTilesetName((BYTE)info.mainGroundType);
    snprintf(item->tileset, sizeof(item->tileset), "%s", tileset ? tileset : UI_GetString("UNKNOWNMAP_TILESET"));
    item->flags = info.flags;
    item->players = MIN(LAN_CountMapPlayers(&info), 16);
    uiimport.FreeMapInfo(&info);
    return true;
}

static BOOL LAN_MapExists(LPCSTR path) {
    for (DWORD i = 0; i < lan.maps.count; i++) {
        if (!strcasecmp(lan.maps.items[i].path, path)) {
            return true;
        }
    }
    return false;
}

static void LAN_AddMap(LPCSTR path) {
    uiMapListItem_t *item;

    if (!path || !*path || lan.maps.count >= UI_MAX_MAP_LIST_ITEMS ||
        !LAN_HasMapExtension(path) || LAN_IsCampaignMap(path) || LAN_MapExists(path)) {
        return;
    }

    item = &lan.maps.items[lan.maps.count++];
    memset(item, 0, sizeof(*item));
    snprintf(item->path, sizeof(item->path), "%s", path);
    for (char *p = item->path; *p; p++) {
        if (*p == '/') {
            *p = '\\';
        }
    }
    if (uiimport.DefaultMapName) {
        uiimport.DefaultMapName(item->path, item->name, sizeof(item->name));
    } else {
        LAN_CopyString(item->name, sizeof(item->name), item->path);
    }
    snprintf(item->description, sizeof(item->description), "%s", UI_GetString("UNKNOWNMAP_DESCRIPTION"));
    snprintf(item->suggestedPlayers, sizeof(item->suggestedPlayers), "%s", UI_GetString("UNKNOWNMAP_SUGGESTEDPLAYERS"));
    snprintf(item->mapSize, sizeof(item->mapSize), "%s", UI_GetString("UNKNOWNMAP_MAPSIZE"));
    snprintf(item->tileset, sizeof(item->tileset), "%s", UI_GetString("UNKNOWNMAP_TILESET"));
    LAN_ParseMapInfo(item);
}

static int LAN_CompareMaps(const void *a, const void *b) {
    const uiMapListItem_t *ma = a;
    const uiMapListItem_t *mb = b;

    if (ma->players != mb->players) {
        return ma->players < mb->players ? -1 : 1;
    }
    return strcasecmp(ma->name, mb->name);
}

static void LAN_LoadMapsWithExtension(LPCSTR extension) {
    char listbuf[LAN_FILELIST_SIZE];
    int count;
    char *cursor;

    if (!uiimport.FS_GetFileList) {
        return;
    }

    memset(listbuf, 0, sizeof(listbuf));
    count = uiimport.FS_GetFileList("Maps", extension, listbuf, sizeof(listbuf));
    cursor = listbuf;
    for (int i = 0; i < count && *cursor; i++) {
        LAN_AddMap(cursor);
        cursor += strlen(cursor) + 1;
    }
}

static void LAN_LoadMaps(void) {
    memset(&lan.maps, 0, sizeof(lan.maps));
    LAN_LoadMapsWithExtension(".w3m");
    LAN_LoadMapsWithExtension(".w3x");
    qsort(lan.maps.items, lan.maps.count, sizeof(lan.maps.items[0]), LAN_CompareMaps);
    lan.maps.visualScroll = (FLOAT)lan.maps.scroll;
    uiimport.Printf("LAN_LoadMaps: %d maps\n", (int)lan.maps.count);
}

static void LAN_SetTextIfPresent(LPFRAMEDEF frame, LPCSTR format, ...) {
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

static LPCSTR LAN_GameSpeedText(void) {
    int value;

    if (!lan.game_speed_slider) {
        return UI_GetString("FAST");
    }

    value = (int)floorf(lan.game_speed_slider->Slider.InitialValue + 0.5f);
    switch (value) {
        case 0:
            return UI_GetString("SLOW");
        case 1:
            return UI_GetString("NORMAL");
        case 2:
        default:
            return UI_GetString("FAST");
    }
}

static void LAN_UpdateGameSpeed(void) {
    LAN_SetTextIfPresent(lan.game_speed_value, "%s", LAN_GameSpeedText());
}

static void LAN_BindMapInfoPane(LPFRAMEDEF container) {
    LPFRAMEDEF source;

    if (lan.info_root || !container) {
        return;
    }

    source = UI_FindFrame("MapInfoPane");
    if (!source) {
        uiimport.Printf("LAN_BindMapInfoPane: MapInfoPane missing\n");
        return;
    }

    lan.info_root = UI_CloneFrameTree(source, container);
    if (!lan.info_root) {
        return;
    }
    UI_SetPoint(lan.info_root, FRAMEPOINT_TOPLEFT, container, FRAMEPOINT_TOPLEFT, 0.0f, 0.0f);
    UI_SetPoint(lan.info_root, FRAMEPOINT_BOTTOMRIGHT, container, FRAMEPOINT_BOTTOMRIGHT, 0.0f, 0.0f);
    UI_LayoutMapInfoPane(lan.info_root);

    UI_FRAME(lan.info_root, MaxPlayersValue);
    UI_FRAME(lan.info_root, MapNameValue);
    UI_FRAME(lan.info_root, MinimapImage);
    UI_FRAME(lan.info_root, SuggestedPlayersValue);
    UI_FRAME(lan.info_root, MapSizeValue);
    UI_FRAME(lan.info_root, MapTilesetValue);
    UI_FRAME(lan.info_root, MapDescValue);

    lan.info_players = MaxPlayersValue;
    lan.info_name = MapNameValue;
    lan.info_minimap = MinimapImage;
    lan.info_suggested = SuggestedPlayersValue;
    lan.info_size = MapSizeValue;
    lan.info_tileset = MapTilesetValue;
    lan.info_description = MapDescValue;
}

static void LAN_CreateMapListFrame(LPFRAMEDEF container, LPFRAMEDEF label) {
    LPFRAMEDEF source;

    if (lan.map_list_box || !container) {
        return;
    }
    source = UI_FindFrame("MapListBox");
    lan.map_list_box = source ? UI_CloneFrameTree(source, container) : NULL;
    if (!lan.map_list_box) {
        return;
    }
    UI_SetPoint(lan.map_list_box, FRAMEPOINT_TOPLEFT, container, FRAMEPOINT_TOPLEFT, 0.0f, 0.0f);
    UI_SetPoint(lan.map_list_box, FRAMEPOINT_BOTTOMRIGHT, container, FRAMEPOINT_BOTTOMRIGHT, 0.0f, 0.0f);
    UI_BindMapList(lan.map_list_box, &lan.maps, label, LAN_VISIBLE_MAPS, "menu_lan_select %u");
}

static void LAN_UpdateMapInfo(void) {
    uiMapListItem_t *item;

    if (lan.maps.count == 0 || lan.maps.selected >= lan.maps.count) {
        UI_SetHidden(lan.info_minimap, true);
        LAN_SetTextIfPresent(lan.info_players, " ");
        LAN_SetTextIfPresent(lan.info_name, " ");
        LAN_SetTextIfPresent(lan.info_suggested, "%s", UI_GetString("UNKNOWNMAP_SUGGESTEDPLAYERS"));
        LAN_SetTextIfPresent(lan.info_size, "%s", UI_GetString("UNKNOWNMAP_MAPSIZE"));
        LAN_SetTextIfPresent(lan.info_tileset, "%s", UI_GetString("UNKNOWNMAP_TILESET"));
        LAN_SetTextIfPresent(lan.info_description, "%s", UI_GetString("UNKNOWNMAP_DESCRIPTION"));
        return;
    }

    item = &lan.maps.items[lan.maps.selected];
    if (lan.info_minimap && uiimport.FindMapPreviewTexture) {
        PATHSTR preview;

        if (uiimport.FindMapPreviewTexture(item->path, preview, sizeof(preview))) {
            UI_SetTexture(lan.info_minimap, preview, false);
            UI_SetHidden(lan.info_minimap, false);
        } else {
            UI_SetHidden(lan.info_minimap, true);
        }
    }
    LAN_SetTextIfPresent(lan.info_players, "%u", (unsigned)item->players);
    LAN_SetTextIfPresent(lan.info_name, "%s", item->name[0] ? item->name : item->path);
    LAN_SetTextIfPresent(lan.info_suggested, "%s", item->suggestedPlayers);
    LAN_SetTextIfPresent(lan.info_size, "%s", item->mapSize);
    LAN_SetTextIfPresent(lan.info_tileset, "%s", item->tileset);
    LAN_SetTextIfPresent(lan.info_description, "%s", item->description);
}

static void LAN_UpdateControls(void) {
    if (!lan.ready) {
        return;
    }

    if (lan.play_button) {
        if (lan.maps.count > 0) {
            UI_SetOnClick(lan.play_button, "menu_lan_start");
        } else {
            lan.play_button->OnClick[0] = '\0';
        }
    }
    LAN_UpdateGameSpeed();
    LAN_UpdateMapInfo();
}

static void LAN_BuildFrames(void) {
    lan.ready = false;
    lan.root = lan.frames.LocalMultiplayerCreate;
    if (!lan.root) {
        uiimport.Printf("LAN_BuildFrames: LocalMultiplayerCreate missing\n");
        return;
    }
    UI_SetAllPoints(lan.root);

    lan.play_button = lan.frames.PlayButton;
    if (!lan.play_button) {
        uiimport.Printf("LAN_BuildFrames: PlayButton missing\n");
        return;
    }
    lan.game_speed_slider = lan.frames.GameSpeedSlider;
    lan.game_speed_value = lan.frames.GameSpeedValue;

    LAN_CreateMapListFrame(lan.frames.MapListContainer, lan.frames.MapListLabel);
    LAN_BindMapInfoPane(lan.frames.MapInfoPaneContainer);

    UI_SetHidden(lan.frames.MapInfoPanel, false);
    UI_SetHidden(lan.frames.AdvancedOptionsPanel, true);
    UI_SetOnClick(lan.frames.MapInfoButton, "menu_startserver");
    UI_SetOnClick(lan.frames.AdvancedOptionsButton, "menu_startserver");
    UI_SetOnClick(lan.play_button, "menu_lan_start");
    UI_SetOnClick(lan.frames.CancelButton, "menu_main");
    lan.ready = true;
}

static void LANJoin_Init(void) {
    uiimport.Printf("LANCreate_Init\n");
    UI_PreloadGlueSceneModels();
    LAN_BuildFrames();
    if (!lan.ready) {
        return;
    }
    LAN_LoadMaps();
    LAN_UpdateControls();
}

static void LANJoin_Shutdown(void) {
}

static void LANJoin_Refresh(int msec) {
    if (!lan.ready) {
        return;
    }

    FLOAT const target = (FLOAT)lan.maps.scroll;
    FLOAT const diff = target - lan.maps.visualScroll;
    FLOAT alpha = (FLOAT)msec / 90.0f;

    if (alpha > 1.0f) {
        alpha = 1.0f;
    }
    if (diff > -0.001f && diff < 0.001f) {
        lan.maps.visualScroll = target;
    } else {
        lan.maps.visualScroll += diff * alpha;
    }
}

static void LANJoin_Draw(void) {
    if (!lan.ready) {
        return;
    }

    UI_DrawGlueScene("BattlenetCustomCreate Stand");
    LAN_UpdateGameSpeed();
    UI_DrawFrame(lan.root);
}

static void LANJoin_KeyEvent(int key, BOOL down) {
    (void)key;
    (void)down;
}

static void LANJoin_MouseEvent(int x, int y, int buttons) {
    (void)x;
    (void)y;
    (void)buttons;
}

LPCSTR LAN_SelectedMapPath(void) {
    if (!lan.ready) {
        return NULL;
    }
    if (lan.maps.count == 0 || lan.maps.selected >= lan.maps.count) {
        return NULL;
    }
    return lan.maps.items[lan.maps.selected].path;
}

LPCSTR LAN_SelectedMapName(void) {
    if (!lan.ready) {
        return NULL;
    }
    if (lan.maps.count == 0 || lan.maps.selected >= lan.maps.count) {
        return NULL;
    }
    return lan.maps.items[lan.maps.selected].name[0]
        ? lan.maps.items[lan.maps.selected].name
        : lan.maps.items[lan.maps.selected].path;
}

void LAN_StartSelectedMap(void) {
    if (!LAN_SelectedMapPath()) {
        return;
    }
    UI_ShowGameSetupMenu();
}

void LAN_SelectMapIndex(DWORD index) {
    if (!lan.ready) {
        return;
    }
    if (index >= lan.maps.count) {
        return;
    }
    lan.maps.selected = index;
    if (lan.maps.selected < lan.maps.scroll) {
        lan.maps.scroll = lan.maps.selected;
    }
    if (lan.maps.selected >= lan.maps.scroll + LAN_VISIBLE_MAPS) {
        lan.maps.scroll = lan.maps.selected - LAN_VISIBLE_MAPS + 1;
    }
}

void LAN_ShowCreate(void) {
    if (!lan.ready) {
        return;
    }

    LAN_UpdateControls();
}

void LAN_RefreshMaps(void) {
    if (!lan.ready) {
        return;
    }
    LAN_LoadMaps();
    LAN_UpdateControls();
}

uiScreen_t lanJoinScreen = {
    .name = "lan",
    .load = LANJoin_LoadScreen,
    .init = LANJoin_Init,
    .shutdown = LANJoin_Shutdown,
    .refresh = LANJoin_Refresh,
    .draw = LANJoin_Draw,
    .key_event = LANJoin_KeyEvent,
    .mouse_event = LANJoin_MouseEvent,
};
