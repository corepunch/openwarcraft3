#ifndef war3map_h
#define war3map_h

#include "common.h"

typedef struct {
    float bounds[8];
    int complement[4]; // bounds complements* (see note 1) (ints A, B, C and D)
} mapCameraBounds_t;

enum mapInfoFlags_t {
    hide_minimap_in_preview_screens = 0x0001,
    modify_ally_priorities = 0x0002,
    melee_map = 0x0004,
    playable_map_size_was_large_and_has_never_been_reduced_to_medium = 0x0008,
    masked_area_are_partially_visible = 0x0010,
    fixed_player_setting_for_custom_forces = 0x0020,
    use_custom_forces = 0x0040,
    use_custom_techtree = 0x0080,
    use_custom_abilities = 0x0100,
    use_custom_upgrades = 0x0200,
    map_properties_menu_opened_at_least_once_since_map_creation = 0x0400,
    show_water_waves_on_cliff_shores = 0x0800,
    show_water_waves_on_rolling_shores = 0x1000,
};

enum playerFlags_t {
    fixed_start_position = 0x0001,
};

typedef enum {
    kPlayerTypeNone,
    kPlayerTypeHuman,
    kPlayerTypeComputer,
    kPlayerTypeNeutral,
    kPlayerTypeRescuable
} playerType_t;

typedef enum {
    kPlayerRaceNone,
    kPlayerRaceHuman,
    kPlayerRaceOrc,
    kPlayerRaceUndead,
    kPlayerRaceNightElf
} playerRace_t;

typedef enum {
    allied_force_1 = 0x0001,
    allied_victory = 0x0002,
    share_vision = 0x0004,
    share_unit_control = 0x0010,
    share_advanced_unit_control = 0x0020,
} forceFlags_t;

typedef struct {
    DWORD internalPlayerNumber;
    playerType_t playerType;
    playerRace_t playerRace;
    DWORD flags;
    LPSTR playerName;
    VECTOR2 startingPosition;
    DWORD allyLowPrioritiesFlags; // (bit "x"=1 -> set for player "x")
    DWORD allyHighPrioritiesFlags; // (bit "x"=1 -> set for player "x")
} mapPlayer_t;

typedef struct {
    DWORD focesFlags;
    DWORD playerMasks; // (bit "x"=1 -> player "x" is in this force)
    LPSTR forceName;
} mapForce_t;

typedef enum {
    upgrade_unavailable,
    upgrace_available,
    upgrade_researched
} upgradeAvailability_t;

typedef struct {
    DWORD playerFlags; // (bit "x"=1 if this change applies for player "x")
    DWORD upgradeID; // (as in UpgradeData.slk)
    DWORD levelOfTheUpgrade; // for which the availability is changed (this is actually the level - 1, so 1 => 0)
    upgradeAvailability_t availability; // (0 = unavailable, 1 = available, 2 = researched)
} mapUpgradeAvailability_t;

typedef struct {
    DWORD playerFlags; // (bit "x"=1 if this change applies for player "x")
    DWORD techID; // (this can be an item, unit or ability)
    // there's no need for an availability value, if a tech-id is in this list, it means that it's not available
} mapTechAvailability_t;

typedef enum {
    group_unit_table,
    group_building_table,
    group_item_table
} mapRandomGroupPositionType_t;

typedef struct {
    mapRandomGroupPositionType_t type;
    DWORD chanceItem; // (percentage)
    DWORD itemID;
//    for each position are the unit/item id's for this line specified
//    this can also be random unit/item ids (see bottom of war3mapUnits.doo definition)
//    a unit/item id of 0x00000000 indicates that no unit/item is created
} mapRandomGroupPositionItem_t;

typedef struct {
    mapRandomGroupPositionType_t type;
    DWORD num_items;
    mapRandomGroupPositionItem_t *items;
} mapRandomGroupPosition_t;

typedef struct {
    DWORD groupNumber;
    LPSTR groupName;
    DWORD num_positions;
//    positions are the table columns where you can enter the unit/item ids,
//    all units in the same line have the same chance, but belong to different "sets"
//    of the random group, called positions here
    mapRandomGroupPosition_t *positions;
} MapRandomGroup;

typedef struct {
    DWORD num_randomGroups;
    MapRandomGroup *randomGroups;
} mapRandomUnitTable_t;

typedef struct {
    DWORD fileFormat; // file format version = 18
    DWORD numberOfSaves;
    DWORD editorVersion;
    LPSTR mapName;
    LPSTR mapAuthor;
    LPSTR mapDescription;
    LPSTR playersRecommended;
    mapCameraBounds_t cameraBounds; // as defined in the JASS file
    size2_t playableArea; // width E, height F, *note 1: map width = A + E + B, map height = C + F + D
    DWORD flags;
    char mainGroundType; // Example: 'A'= Ashenvale, 'X' = City Dalaran
    DWORD campaignBackgroundNumber; // (-1 = none)
    LPSTR loadingScreenText;
    LPSTR loadingScreenTitle;
    LPSTR loadingScreenSubtitle;
    DWORD loadingScreenNumber; // (-1 = none)
    LPSTR prologueScreenText;
    LPSTR prologueScreenTitle;
    LPSTR prologueScreenSubtitle;
    DWORD num_players;
    DWORD num_forces;
    DWORD num_upgradeAvailabilities;
    DWORD num_techAvailabilities;
    DWORD num_randomUnits;
    mapPlayer_t *players;
    mapForce_t *forces;
    mapUpgradeAvailability_t *upgradeAvailabilities;
    mapTechAvailability_t *techAvailabilities;
    mapRandomUnitTable_t *randomUnits;
} mapInfo_t;

struct War3MapVertex {
    USHORT accurate_height;
    USHORT waterlevel:14;
    BYTE mapedge:2;
    BYTE ground:4;
    BYTE ramp:1;
    BYTE blight:1;
    BYTE water:1;
    BYTE boundary:1;
    BYTE details:4;
    BYTE unknown:4;
    BYTE level:4;
    BYTE cliff:4;
};

struct war3map {
    DWORD header;
    DWORD version;
    BYTE tileset;
    DWORD custom;
    LPDWORD grounds;
    LPDWORD cliffs;
    VECTOR2 center;
    DWORD width;
    DWORD height;
    HANDLE vertices;
    DWORD num_grounds;
    DWORD num_cliffs;
};

void CM_LoadMap(LPCSTR mapFilename);
float CM_GetHeightAtPoint(float sx, float sy);
LPDOODAD CM_GetDoodads(void);
mapPlayer_t const *CM_GetPlayer(DWORD index);
VECTOR2 CM_GetNormalizedMapPosition(float x, float y);
VECTOR2 CM_GetDenormalizedMapPosition(float x, float y);

#endif
