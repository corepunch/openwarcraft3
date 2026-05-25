#include "common.h"
#include "mpq.h"

#ifndef _WIN32
#include <strings.h>
#endif

static struct {
    LPWAR3MAP map;
    MAPINFO info;
    struct Doodad *doodads;
} world = { 0 };

void CM_ReadPathMap(HANDLE archive);

void PF_TextRemoveComments(LPSTR buffer);
BOMStatus PF_TextRemoveBom(LPSTR buffer);

DWORD SFileReadStringLength(HANDLE file) {
    DWORD filePosition = SFileSetFilePointer(file, 0, 0, FILE_CURRENT);
    DWORD stringLength = 1;
    while (true) {
        BYTE ch = 0;
        SFileReadFile(file, &ch, 1, NULL, NULL);
        if (ch == 0) {
            break;
        } else {
            stringLength++;
        }
    }
    SFileSetFilePointer(file, filePosition, 0, FILE_BEGIN);
    return stringLength;
}

void SFileReadString(HANDLE file, LPSTR *lppString) {
    DWORD stringLength = SFileReadStringLength(file);
    *lppString = MemAlloc(stringLength);
    SFileReadFile(file, *lppString, stringLength, NULL, NULL);
}

static void SFileSkipBytes(HANDLE file, DWORD size) {
    SFileSetFilePointer(file, size, 0, FILE_CURRENT);
}

static DWORD SFileBytesRemaining(HANDLE file) {
    DWORD position = SFileSetFilePointer(file, 0, 0, FILE_CURRENT);
    DWORD size = SFileGetFileSize(file, NULL);

    if (position >= size) {
        return 0;
    }
    return size - position;
}

static void SFileSkipString(HANDLE file) {
    LPSTR text = NULL;

    SFileReadString(file, &text);
    SAFE_DELETE(text, MemFree);
}

static BOOL CM_ReadInfoInto(HANDLE archive, LPMAPINFO info, BOOL setup_only) {
    HANDLE file;

    if (!archive || !info) {
        return false;
    }
    if (!SFileOpenFileEx(archive, "war3map.w3i", SFILE_OPEN_FROM_MPQ, &file)) {
        return false;
    }
    SFileReadFile(file, &info->fileFormat, 4, NULL, NULL);
    SFileReadFile(file, &info->numberOfSaves, sizeof(DWORD), NULL, NULL);
    SFileReadFile(file, &info->editorVersion, sizeof(DWORD), NULL, NULL);
    if (info->fileFormat > 27) {
        SFileSkipBytes(file, sizeof(DWORD) * 4);
    }
    SFileReadString(file, &info->mapName);
    SFileReadString(file, &info->mapAuthor);
    SFileReadString(file, &info->mapDescription);
    SFileReadString(file, &info->playersRecommended);
    SFileReadFile(file, &info->cameraBounds, sizeof(mapCameraBounds_t), NULL, NULL);
    SFileReadFile(file, &info->playableArea, sizeof(size2_t), NULL, NULL);
    SFileReadFile(file, &info->flags, sizeof(DWORD), NULL, NULL);
    SFileReadFile(file, &info->mainGroundType, sizeof(char), NULL, NULL);
    SFileReadFile(file, &info->campaignBackgroundNumber, sizeof(DWORD), NULL, NULL);
    if (info->fileFormat > 24) {
        SFileSkipString(file);
    }
    SFileReadString(file, &info->loadingScreenText);
    SFileReadString(file, &info->loadingScreenTitle);
    SFileReadString(file, &info->loadingScreenSubtitle);
    SFileReadFile(file, &info->loadingScreenNumber, sizeof(DWORD), NULL, NULL);
    if (info->fileFormat > 24) {
        SFileSkipString(file);
    }
    SFileReadString(file, &info->prologueScreenText);
    SFileReadString(file, &info->prologueScreenTitle);
    SFileReadString(file, &info->prologueScreenSubtitle);
    if (info->fileFormat > 24) {
        SFileSkipBytes(file, sizeof(DWORD) + sizeof(FLOAT) * 2 + sizeof(FLOAT) + sizeof(BYTE) * 4 + sizeof(DWORD));
        SFileSkipString(file);
        SFileSkipBytes(file, sizeof(BYTE) + sizeof(BYTE) * 4);
    }
    if (info->fileFormat > 27) {
        SFileSkipBytes(file, sizeof(BYTE) * 4);
    }
    if (info->fileFormat > 30) {
        SFileSkipBytes(file, sizeof(DWORD) * 2);
    }

    DWORD num_players = 0;
    SFileReadFile(file, &num_players, sizeof(DWORD), NULL, NULL);
    FOR_LOOP(i, num_players) {
        DWORD playerNumber = 0;
        mapPlayer_t scratch = { 0 };
        mapPlayer_t *player;

        SFileReadFile(file, &playerNumber, sizeof(DWORD), NULL, NULL);
        player = playerNumber < MAX_PLAYERS ? info->players + playerNumber : &scratch;
        player->used = true;
        SFileReadFile(file, &player->playerType, sizeof(playerType_t), NULL, NULL);
        SFileReadFile(file, &player->playerRace, sizeof(playerRace_t), NULL, NULL);
        SFileReadFile(file, &player->flags, sizeof(DWORD), NULL, NULL);
        SFileReadString(file, &player->playerName);
        SFileReadFile(file, &player->startingPosition, sizeof(VECTOR2), NULL, NULL);
        SFileReadFile(file, &player->allyLowPrioritiesFlags, sizeof(DWORD), NULL, NULL);
        SFileReadFile(file, &player->allyHighPrioritiesFlags, sizeof(DWORD), NULL, NULL);
        SAFE_DELETE(scratch.playerName, MemFree);
    }

    SFileReadFile(file, &info->num_teams, sizeof(DWORD), NULL, NULL);
    info->teams = MemAlloc(sizeof(mapTeam_t) * info->num_teams);
    FOR_LOOP(i, info->num_teams) {
        mapTeam_t *force = &info->teams[i];
        SFileReadFile(file, &force->flags, sizeof(DWORD), NULL, NULL);
        SFileReadFile(file, &force->playerMasks, sizeof(DWORD), NULL, NULL);
        SFileReadString(file, &force->name);
    }

    if (setup_only || SFileBytesRemaining(file) <= 1) {
        SFileCloseFile(file);
        return true;
    }
    SFileReadFile(file, &info->num_upgradeAvailabilities, sizeof(DWORD), NULL, NULL);
    info->upgradeAvailabilities = MemAlloc(sizeof(mapUpgradeAvailability_t) * info->num_upgradeAvailabilities);
    FOR_LOOP(i, info->num_upgradeAvailabilities) {
        mapUpgradeAvailability_t *upgrade = &info->upgradeAvailabilities[i];
        SFileReadFile(file, &upgrade->playerFlags, sizeof(DWORD), NULL, NULL);
        SFileReadFile(file, &upgrade->upgradeID, sizeof(DWORD), NULL, NULL);
        SFileReadFile(file, &upgrade->levelOfTheUpgrade, sizeof(DWORD), NULL, NULL);
        SFileReadFile(file, &upgrade->availability, sizeof(upgradeAvailability_t), NULL, NULL);
    }

    if (SFileBytesRemaining(file) <= 1) {
        SFileCloseFile(file);
        return true;
    }
    SFileReadFile(file, &info->num_techAvailabilities, sizeof(DWORD), NULL, NULL);
    info->techAvailabilities = MemAlloc(sizeof(mapTechAvailability_t) * info->num_techAvailabilities);
    FOR_LOOP(i, info->num_techAvailabilities) {
        mapTechAvailability_t *tech = &info->techAvailabilities[i];
        SFileReadFile(file, &tech->playerFlags, sizeof(DWORD), NULL, NULL);
        SFileReadFile(file, &tech->techID, sizeof(DWORD), NULL, NULL);
    }

    if (SFileBytesRemaining(file) <= 1) {
        SFileCloseFile(file);
        return true;
    }
    SFileReadFile(file, &info->num_randomUnits, sizeof(DWORD), NULL, NULL);
    info->randomUnits = MemAlloc(sizeof(mapRandomUnitTable_t) * info->num_randomUnits);
    FOR_LOOP(i, info->num_randomUnits) {
        mapRandomUnitTable_t *table = &info->randomUnits[i];
        SFileReadFile(file, &table->num_randomGroups, sizeof(DWORD), NULL, NULL);
        table->randomGroups = MemAlloc(sizeof(MapRandomGroup) * table->num_randomGroups);
        FOR_LOOP(j, table->num_randomGroups) {
            MapRandomGroup *group = &table->randomGroups[j];
            SFileReadFile(file, &group->groupNumber, sizeof(DWORD), NULL, NULL);
            SFileReadString(file, &group->groupName);
            SFileReadFile(file, &group->num_positions, sizeof(DWORD), NULL, NULL);
            group->positions = MemAlloc(sizeof(mapRandomGroupPosition_t) * group->num_positions);
            FOR_LOOP(k, group->num_positions) {
                mapRandomGroupPosition_t *position = &group->positions[k];
                SFileReadFile(file, &position->type, sizeof(mapRandomGroupPositionType_t), NULL, NULL);
                SFileReadFile(file, &position->num_items, sizeof(DWORD), NULL, NULL);
                position->items = MemAlloc(sizeof(mapRandomGroupPositionItem_t) * position->num_items);
                SFileReadFile(file, position->items, sizeof(mapRandomGroupPositionItem_t) * position->num_items, NULL, NULL);
            }
        }
    }

    SFileCloseFile(file);
    return true;
}

static void CM_ReadInfo(HANDLE archive) {
    CM_ReadInfoInto(archive, &world.info, false);
}

static void MapInfo_Release(LPMAPINFO mapInfo) {
    mapTrigStr_t *string = mapInfo ? mapInfo->strings : NULL;

    if (!mapInfo) {
        return;
    }
    FOR_LOOP(i, MAX_PLAYERS) {
        SAFE_DELETE(mapInfo->players[i].playerName, MemFree);
    }
    FOR_LOOP(i, mapInfo->num_teams) {
        SAFE_DELETE(mapInfo->teams[i].name, MemFree);
    }
    FOR_LOOP(i, mapInfo->num_randomUnits) {
        FOR_LOOP(j, mapInfo->randomUnits[i].num_randomGroups) {
            FOR_LOOP(k, mapInfo->randomUnits[i].randomGroups[j].num_positions) {
                SAFE_DELETE(mapInfo->randomUnits[i].randomGroups[j].positions[k].items, MemFree);
            }
            SAFE_DELETE(mapInfo->randomUnits[i].randomGroups[j].positions, MemFree);
            SAFE_DELETE(mapInfo->randomUnits[i].randomGroups[j].groupName, MemFree);
        }
        SAFE_DELETE(mapInfo->randomUnits[i].randomGroups, MemFree);
    }
    SAFE_DELETE(mapInfo->mapName, MemFree);
    SAFE_DELETE(mapInfo->mapAuthor, MemFree);
    SAFE_DELETE(mapInfo->mapDescription, MemFree);
    SAFE_DELETE(mapInfo->playersRecommended, MemFree);
    SAFE_DELETE(mapInfo->loadingScreenText, MemFree);
    SAFE_DELETE(mapInfo->loadingScreenTitle, MemFree);
    SAFE_DELETE(mapInfo->loadingScreenSubtitle, MemFree);
    SAFE_DELETE(mapInfo->prologueScreenText, MemFree);
    SAFE_DELETE(mapInfo->prologueScreenTitle, MemFree);
    SAFE_DELETE(mapInfo->prologueScreenSubtitle, MemFree);
    SAFE_DELETE(mapInfo->teams, MemFree);
    SAFE_DELETE(mapInfo->upgradeAvailabilities, MemFree);
    SAFE_DELETE(mapInfo->techAvailabilities, MemFree);
    SAFE_DELETE(mapInfo->randomUnits, MemFree);
    while (string) {
        mapTrigStr_t *next = string->next;
        MemFree(string);
        string = next;
    }
    SAFE_DELETE(mapInfo->mapscript, MemFree);
}

void CM_FreeMapInfo(LPMAPINFO mapInfo) {
    if (!mapInfo) {
        return;
    }
    MapInfo_Release(mapInfo);
    memset(mapInfo, 0, sizeof(*mapInfo));
}

static void CM_ReadDoodads(HANDLE archive) {
    HANDLE file;
    DWORD fileHeader, version, unknown, numDoodads;

    SFileOpenFileEx(archive, "war3map.doo", SFILE_OPEN_FROM_MPQ, &file);
    SFileReadFile(file, &fileHeader, 4, NULL, NULL);
    SFileReadFile(file, &version, 4, NULL, NULL);
    SFileReadFile(file, &unknown, 4, NULL, NULL);
    SFileReadFile(file, &numDoodads, 4, NULL, NULL);

    FOR_LOOP(index, numDoodads) {
        LPDOODAD doodad = MemAlloc(sizeof(DOODAD));
        SFileReadFile(file, &doodad->doodID, sizeof(DWORD), NULL, NULL);
        SFileReadFile(file, &doodad->variation, sizeof(DWORD), NULL, NULL);
        SFileReadFile(file, &doodad->position, sizeof(VECTOR3), NULL, NULL);
        SFileReadFile(file, &doodad->angle, sizeof(FLOAT), NULL, NULL);
        SFileReadFile(file, &doodad->scale, sizeof(VECTOR3), NULL, NULL);
        SFileReadFile(file, &doodad->flags, sizeof(BYTE), NULL, NULL);
        SFileReadFile(file, &doodad->treeLife, sizeof(BYTE), NULL, NULL);
        SFileReadFile(file, &doodad->unitID, sizeof(DWORD), NULL, NULL);
        
        ADD_TO_LIST(doodad, world.doodads);
    }

    SFileCloseFile(file);
}

static void CM_ReadUnit(HANDLE file, struct Doodad *unit) {
    SFileReadFile(file, &unit->doodID, sizeof(DWORD), NULL, NULL);
    SFileReadFile(file, &unit->variation, sizeof(DWORD), NULL, NULL);
    SFileReadFile(file, &unit->position, sizeof(VECTOR3), NULL, NULL);
    SFileReadFile(file, &unit->angle, sizeof(FLOAT), NULL, NULL);
    SFileReadFile(file, &unit->scale, sizeof(VECTOR3), NULL, NULL);
    SFileReadFile(file, &unit->flags, sizeof(BYTE), NULL, NULL);
    SFileReadFile(file, &unit->player, sizeof(DWORD), NULL, NULL);
    SFileReadFile(file, &unit->unknown1, sizeof(BYTE), NULL, NULL);
    SFileReadFile(file, &unit->unknown2, sizeof(BYTE), NULL, NULL);
    SFileReadFile(file, &unit->hitPoints, sizeof(DWORD), NULL, NULL); // (-1 = use default)
    SFileReadFile(file, &unit->manaPoints, sizeof(DWORD), NULL, NULL); // (-1 = use default, 0 = unit doesn't have mana)
    // in Frozen Throne:
//    SFileReadFile(file, &unit->droppedItemSetPtr, sizeof(DWORD), NULL, NULL);
    SFileReadFile(file, &unit->num_droppedItemSets, sizeof(DWORD), NULL, NULL);
    unit->droppableItemSets = MemAlloc(unit->num_droppedItemSets * sizeof(droppableItemSet_t));
    FOR_LOOP(droppedItemSetIdx, unit->num_droppedItemSets) {
        droppableItemSet_t *set = &unit->droppableItemSets[droppedItemSetIdx];
        SFileReadFile(file, &set->num_droppableItems, sizeof(DWORD), NULL, NULL);
        set->droppableItems = MemAlloc(set->num_droppableItems * sizeof(droppableItem_t));
        SFileReadFile(file, set->droppableItems, set->num_droppableItems * sizeof(droppableItem_t), NULL, NULL);
    }

    SFileReadFile(file, &unit->goldAmount, sizeof(DWORD), NULL, NULL); // (default = 12500)
    SFileReadFile(file, &unit->targetAcquisition, sizeof(FLOAT), NULL, NULL); // (-1 = normal, -2 = camp)

    SFileReadFile(file, &unit->hero, sizeof(DWORD), NULL, NULL); // (set to 1 for non hero units and items)
    // in Frozen Throne:
//    SFileReadFile(file, &unit->hero, sizeof(struct DoodadHero), NULL, NULL); // (set to 1 for non hero units and items)

    SFileReadArray(file, unit, inventoryItems, sizeof(inventoryItem_t), MemAlloc);
    SFileReadArray(file, unit, modifiedAbilities, sizeof(modifiedAbility_t), MemAlloc);

    SFileReadFile(file, &unit->randomUnitFlag, sizeof(DWORD), NULL, NULL); // "r" (for uDNR units and iDNR items)

    switch (unit->randomUnitFlag) {
        case 0:
            SFileReadFile(file, &unit->levelOfRandomItem, sizeof(DWORD), NULL, NULL);
            break;
        case 1:
            SFileReadFile(file, &unit->randomUnitGroupNumber, sizeof(DWORD), NULL, NULL);
            SFileReadFile(file, &unit->randomUnitPositionNumber, sizeof(DWORD), NULL, NULL);
            break;
        case 2:
            SFileReadArray(file, unit, diffAvailUnits, sizeof(droppableItem_t), MemAlloc);
            break;
    }
    SFileReadFile(file, &unit->color, sizeof(COLOR32), NULL, NULL);
    SFileReadFile(file, &unit->waygate, sizeof(DWORD), NULL, NULL);
    SFileReadFile(file, &unit->unitID, sizeof(DWORD), NULL, NULL);
}

static void CM_ReadUnitDoodads(HANDLE archive) {
    HANDLE file;
    DWORD fileHeader, version, subversion, numUnits;

    SFileOpenFileEx(archive, "war3mapUnits.doo", SFILE_OPEN_FROM_MPQ, &file);
    SFileReadFile(file, &fileHeader, 4, NULL, NULL);
    SFileReadFile(file, &version, 4, NULL, NULL);
    SFileReadFile(file, &subversion, 4, NULL, NULL);
    SFileReadFile(file, &numUnits, 4, NULL, NULL);

    FOR_LOOP(index, numUnits) {
        LPDOODAD doodad = MemAlloc(sizeof(DOODAD));
        CM_ReadUnit(file, doodad);        
        ADD_TO_LIST(doodad, world.doodads);
    }

    SFileCloseFile(file);
}

static void CM_ReadHeightmap(HANDLE archive) {
    world.map = MemAlloc(sizeof(WAR3MAP));
    HANDLE file;
    SFileOpenFileEx(archive, "war3map.w3e", SFILE_OPEN_FROM_MPQ, &file);
    SFileReadFile(file, &world.map->header, 4, NULL, NULL);
    SFileReadFile(file, &world.map->version, 4, NULL, NULL);
    SFileReadFile(file, &world.map->tileset, 1, NULL, NULL);
    SFileReadFile(file, &world.map->custom, 4, NULL, NULL);
    SFileReadArray(file, world.map, grounds, 4, MemAlloc);
    SFileReadArray(file, world.map, cliffs, 4, MemAlloc);
    SFileReadFile(file, &world.map->width, 4, NULL, NULL);
    SFileReadFile(file, &world.map->height, 4, NULL, NULL);
    SFileReadFile(file, &world.map->center, 8, NULL, NULL);
    int const vertexblocksize = MAP_VERTEX_SIZE * world.map->width * world.map->height;
    world.map->vertices = MemAlloc(vertexblocksize);
    SFileReadFile(file, world.map->vertices, vertexblocksize, 0, 0);
    SFileCloseFile(file);
}

void CM_ReadModification(HANDLE file, unitModification_t *mod) {
    SFileReadFile(file, &mod->modID, 4, NULL, NULL);
    SFileReadFile(file, &mod->type, 4, NULL, NULL);
    DWORD strlength = 0;
    switch (mod->type) {
        case mod_int:
        case mod_real:
        case mod_unreal:
            mod->data = MemAlloc(4);
            SFileReadFile(file, mod->data, 4, NULL, NULL);
            break;
        case mod_bool:
        case mod_char:
            mod->data = MemAlloc(1);
            SFileReadFile(file, mod->data, 1, NULL, NULL);
            break;
        case mod_string:
        case mod_unitList:
        case mod_itemList:
        case mod_regenType:
        case mod_attackType:
        case mod_weaponType:
        case mod_targetType:
        case mod_moveType:
        case mod_defenseType:
        case mod_pathingTexture:
        case mod_upgradeList:
        case mod_stringList:
        case mod_abilityList:
        case mod_heroAbilityList:
        case mod_missileArt:
        case mod_attributeType:
        case mod_attackBits:
            strlength = SFileReadStringLength(file);
            mod->data = MemAlloc(strlength);
            SFileReadFile(file, mod->data, strlength, NULL, NULL);
            break;
        default:
            assert(false);
            break;
    }
}

unitData_t *CM_ReadUnitsOverrides(HANDLE file, DWORD *numUnits) {
    DWORD unknown;
    SFileReadFile(file,numUnits, 4, NULL, NULL);
    unitData_t *units = MemAlloc(*numUnits * sizeof(unitData_t));
    for (unitData_t *unit = units; unit - units < *numUnits; unit++) {
        SFileReadFile(file, &unit->originalUnitID, 4, NULL, NULL);
        SFileReadFile(file, &unit->newUnitID, 4, NULL, NULL);
        SFileReadFile(file, &unit->numbeOfModifications, 4, NULL, NULL);
        unit->modifications = MemAlloc(unit->numbeOfModifications * sizeof(unitModification_t));
        FOR_LOOP(j, unit->numbeOfModifications) {
            CM_ReadModification(file, &unit->modifications[j]);
            SFileReadFile(file, &unknown, 4, NULL, NULL);
        }
    }
    return units;
}

void CM_ReadUnits(HANDLE archive) {
    DWORD version;
    HANDLE file;
    if (!SFileOpenFileEx(archive, "war3map.w3u", SFILE_OPEN_FROM_MPQ, &file)) {
        world.info.num_originalUnits = 0;
        world.info.num_userCreatedUnits = 0;
        world.info.originalUnits = NULL;
        world.info.userCreatedUnits = NULL;
        return;
    }
    SFileReadFile(file, &version, 4, NULL, NULL);
    world.info.originalUnits = CM_ReadUnitsOverrides(file, &world.info.num_originalUnits);
    world.info.userCreatedUnits = CM_ReadUnitsOverrides(file, &world.info.num_userCreatedUnits);
    SFileCloseFile(file);
}

LPSTR FS_ReadArchiveFileIntoString(HANDLE archive, LPCSTR filename) {
    HANDLE file;
    if (!SFileOpenFileEx(archive, filename, SFILE_OPEN_FROM_MPQ, &file)) {
        return NULL;
    }
    DWORD fileSize = SFileGetFileSize(file, NULL);
    LPSTR buffer = MemAlloc(fileSize + 1);
    SFileReadFile(file, buffer, fileSize, NULL, NULL);
    FS_CloseFile(file);
    buffer[fileSize] = '\0';
    return buffer;
}

void removeTrailingWhitespace(char *s) {
    size_t len;

    if (!s) {
        return;
    }
    len = strlen(s);
    while (len > 0 && isspace((unsigned char)s[len - 1])) {
        s[--len] = '\0';
    }
}

static LPCSTR CM_SkipSpace(LPCSTR text) {
    while (text && (*text == ' ' || *text == '\t' || *text == '\r')) {
        text++;
    }
    return text;
}

static void CM_ReadLine(LPCSTR *cursor, LPSTR out, DWORD out_size) {
    DWORD len = 0;
    LPCSTR p;

    if (!cursor || !*cursor || !out || out_size == 0) {
        return;
    }
    p = *cursor;
    while (*p && *p != '\n' && *p != '\r') {
        if (len + 1 < out_size) {
            out[len++] = *p;
        }
        p++;
    }
    out[len] = '\0';
    while (*p == '\n' || *p == '\r') {
        p++;
    }
    *cursor = p;
}

static void CM_AppendTrigStringText(mapTrigStr_t *entry, LPCSTR line) {
    DWORD len;
    DWORD add;

    if (!entry || !line) {
        return;
    }
    len = (DWORD)strlen(entry->text);
    add = (DWORD)strlen(line);
    if (len + add >= sizeof(entry->text)) {
        add = sizeof(entry->text) - len - 1;
    }
    if (add) {
        memcpy(entry->text + len, line, add);
        entry->text[len + add] = '\0';
    }
}

static void CM_ReadStringsInto(HANDLE archive, LPMAPINFO info) {
    LPSTR buffer = FS_ReadArchiveFileIntoString(archive, "war3map.wts");
    LPCSTR cursor;
    mapTrigStr_t *entry = NULL;
    BOOL reading_data = false;
    char line[MAX_TRIGSTR_LENGTH];

    if (!info) {
        SAFE_DELETE(buffer, MemFree);
        return;
    }
    if (!buffer) {
        return;
    }
    PF_TextRemoveBom(buffer);
    cursor = buffer;
    while (*cursor) {
        LPCSTR trimmed;

        CM_ReadLine(&cursor, line, sizeof(line));
        trimmed = CM_SkipSpace(line);
        if (!reading_data) {
            if (!strncmp(trimmed, "STRING ", strlen("STRING "))) {
                SAFE_DELETE(entry, MemFree);
                entry = MemAlloc(sizeof(*entry));
                memset(entry, 0, sizeof(*entry));
                sscanf(trimmed, "STRING %d", &entry->id);
            } else if (entry && *trimmed == '{') {
                reading_data = true;
            }
            continue;
        }
        if (*trimmed == '}') {
            ADD_TO_LIST(entry, info->strings);
            entry = NULL;
            reading_data = false;
            continue;
        }
        removeTrailingWhitespace(line);
        CM_AppendTrigStringText(entry, line);
    }
    if (entry) {
        if (reading_data) {
            ADD_TO_LIST(entry, info->strings);
        } else {
            MemFree(entry);
        }
    }
    MemFree(buffer);
}

void CM_ReadStrings(HANDLE archive) {
    CM_ReadStringsInto(archive, &world.info);
}

BOOL CM_ReadMapInfo(LPCSTR mapFilename, LPMAPINFO info) {
    HANDLE mapArchive;
    HANDLE mapData;
    DWORD mapSize = 0;

    if (!mapFilename || !info) {
        return false;
    }

    memset(info, 0, sizeof(*info));
    mapData = FS_ReadFile(mapFilename, &mapSize);
    if (!mapData || mapSize == 0) {
        return false;
    }
    if (!SFileOpenArchiveFromMemory(mapData, mapSize, 0, &mapArchive)) {
        MemFree(mapData);
        return false;
    }

    if (!CM_ReadInfoInto(mapArchive, info, true)) {
        SFileCloseArchive(mapArchive);
        MemFree(mapData);
        return false;
    }
    CM_ReadStringsInto(mapArchive, info);

    SFileCloseArchive(mapArchive);
    MemFree(mapData);
    return true;
}

BOOL CM_FindMapPreviewTexture(LPCSTR mapFilename, LPSTR out, DWORD out_size) {
    static LPCSTR candidates[] = {
        "war3mapPreview.tga",
        "war3mapMap.blp",
        "war3mapMap.tga",
        NULL,
    };
    HANDLE mapArchive;
    HANDLE mapData;
    DWORD mapSize = 0;
    BOOL found = false;

    if (!mapFilename || !out || out_size == 0) {
        return false;
    }
    out[0] = '\0';

    mapData = FS_ReadFile(mapFilename, &mapSize);
    if (!mapData || mapSize == 0) {
        return false;
    }
    if (!SFileOpenArchiveFromMemory(mapData, mapSize, 0, &mapArchive)) {
        MemFree(mapData);
        return false;
    }

    for (DWORD i = 0; candidates[i]; i++) {
        HANDLE file;

        if (!SFileOpenFileEx(mapArchive, candidates[i], SFILE_OPEN_FROM_MPQ, &file)) {
            continue;
        }
        SFileCloseFile(file);
        snprintf(out, out_size, "%s\\%s", mapFilename, candidates[i]);
        found = true;
        break;
    }

    SFileCloseArchive(mapArchive);
    MemFree(mapData);
    return found;
}

static LPCSTR CM_PathBaseFileName(LPCSTR path) {
    LPCSTR base = path ? path : "";

    for (LPCSTR p = base; *p; p++) {
        if (*p == '\\' || *p == '/') {
            base = p + 1;
        }
    }
    return base;
}

void CM_DefaultMapName(LPCSTR path, LPSTR out, DWORD out_size) {
    DWORD len;

    if (!out || out_size == 0) {
        return;
    }
    snprintf(out, out_size, "%s", CM_PathBaseFileName(path));
    len = (DWORD)strlen(out);
    if (len > 4 && out[len - 4] == '.') {
        out[len - 4] = '\0';
    }
}

void CM_ResolveMapInfoString(LPCMAPINFO info, LPCSTR text, LPSTR out, DWORD out_size) {
    DWORD id;

    if (!out || out_size == 0) {
        return;
    }
    if (!text) {
        out[0] = '\0';
        return;
    }
    if (strncmp(text, "TRIGSTR_", 8)) {
        snprintf(out, out_size, "%s", text);
        return;
    }
    id = (DWORD)strtoul(text + 8, NULL, 10);
    for (mapTrigStr_t const *string = info ? info->strings : NULL; string; string = string->next) {
        if (string->id == id) {
            snprintf(out, out_size, "%s", string->text);
            return;
        }
    }
    snprintf(out, out_size, "%s", text);
}

BOOL CM_MapNameMatchesFile(LPCSTR name, LPCSTR path) {
    PATHSTR base;
    size_t len;

    if (!name || !path || !*name) {
        return false;
    }
    snprintf(base, sizeof(base), "%s", CM_PathBaseFileName(path));
    len = strlen(base);
    if (len > 4 && (!strcasecmp(base + len - 4, ".w3m") ||
                    !strcasecmp(base + len - 4, ".w3x"))) {
        base[len - 4] = '\0';
    }
    return !strcasecmp(name, base);
}

LPCSTR CM_TilesetName(BYTE tileset) {
    switch (tileset) {
        case 'A': return "Ashenvale";
        case 'B': return "Barrens";
        case 'C': return "Felwood";
        case 'D': return "Dungeon";
        case 'F': return "Lordaeron Fall";
        case 'G': return "Underground";
        case 'I': return "Icecrown Glacier";
        case 'J': return "Dalaran";
        case 'K': return "Black Citadel";
        case 'L': return "Lordaeron Summer";
        case 'N': return "Northrend";
        case 'O': return "Outland";
        case 'Q': return "Village Fall";
        case 'V': return "Village";
        case 'W': return "Lordaeron Winter";
        case 'X': return "Dalaran Ruins";
        case 'Y': return "Cityscape";
        case 'Z': return "Sunken Ruins";
        default: return NULL;
    }
}

LPCSTR CM_MapSizeName(DWORD width, DWORD height) {
    DWORD largest = MAX(width, height);

    if (largest <= 96) {
        return "Small";
    }
    if (largest <= 128) {
        return "Medium";
    }
    if (largest <= 160) {
        return "Large";
    }
    return "Huge";
}

void CM_SanitizeMapListField(LPSTR text) {
    if (!text) {
        return;
    }
    for (LPSTR p = text; *p; p++) {
        if (*p == '\n' || *p == '\r' || *p == '\t') {
            *p = ' ';
        }
    }
}

void CM_SanitizeMapInfoText(LPSTR text) {
    if (!text) {
        return;
    }
    for (LPSTR p = text; *p; p++) {
        if (*p == '\t') {
            *p = ' ';
        }
    }
}

void CM_ReadMapScript(HANDLE archive) {
    world.info.mapscript = FS_ReadArchiveFileIntoString(archive, "war3map.j");
//    SFileExtractFile(archive, "war3map.j", "/Users/igor/Desktop/Human02.j", 0);
}

bool CM_LoadMap(LPCSTR mapFilename) {
#ifdef WOW
    memset(&world, 0, sizeof(world));
    if (mapFilename) {
        size_t len = strlen(mapFilename);
        world.info.mapName = MemAlloc(len + 1);
        memcpy(world.info.mapName, mapFilename, len + 1);
    }
    world.info.players[0].used = true;
    world.info.players[0].playerType = kPlayerTypeHuman;
    world.info.players[0].startingPosition = (VECTOR2){ WOW_START_POSITION_X, WOW_START_POSITION_Y };
    (void)mapFilename;
    return true;
#else
    HANDLE mapArchive;
    HANDLE mapData;
    DWORD mapSize = 0;

    memset(&world, 0, sizeof(world));
    mapData = FS_ReadFile(mapFilename, &mapSize);
    if (!mapData || mapSize == 0) {
        Com_Error(ERR_DROP, "CM_LoadMap: failed to read map %s\n", mapFilename);
        return false;
    }
    if (!SFileOpenArchiveFromMemory(mapData, mapSize, 0, &mapArchive)) {
        MemFree(mapData);
        Com_Error(ERR_DROP, "CM_LoadMap: failed to open map archive %s\n", mapFilename);
        return false;
    }
    CM_ReadPathMap(mapArchive);
    CM_ReadDoodads(mapArchive);
    CM_ReadUnitDoodads(mapArchive);
    CM_ReadHeightmap(mapArchive);
    CM_ReadInfo(mapArchive);
    CM_ReadUnits(mapArchive);
    CM_ReadStrings(mapArchive);
    CM_ReadMapScript(mapArchive);
        
//    SFileExtractFile(mapArchive, "war3map.wts", "/Users/igor/Desktop/war3map.wts", 0);
//    HANDLE file;
//    SFileOpenFileEx(mapArchive, "war3map.wts", SFILE_OPEN_FROM_MPQ, &file);
//    char ch;
//    while (SFileReadFile(file, &ch, 1, NULL, NULL)) {
//        printf("%c", ch);
//    }
//    SFileCloseFile(file);
//    printf("\n");
    
    
//    SFILE_FIND_DATA findData;
//    HANDLE handle = SFileFindFirstFile(mapArchive, "*", &findData, 0);
//    if (handle) {
//         do {
//             printf("%s\n", findData.cFileName);
//             HANDLE file;
//             SFileOpenFileEx(mapArchive, findData.cFileName, SFILE_OPEN_FROM_MPQ, &file);
//             char ch;
//             while (SFileReadFile(file, &ch, 1, NULL, NULL)) {
//                 printf("%c", ch);
//             }
//             SFileCloseFile(file);
//             printf("\n");
//         } while (SFileFindNextFile(handle, &findData));
//         SFileFindClose(handle);
//     }

    SFileCloseArchive(mapArchive);
    MemFree(mapData);
    return true;
#endif
}

static LPCWAR3MAPVERTEX CM_GetWar3MapVertex(DWORD x, DWORD y) {
    int const index = x + y * world.map->width;
    char const *ptr = ((char const *)world.map->vertices) + index * MAP_VERTEX_SIZE;
    return (LPCWAR3MAPVERTEX)ptr;
}

static FLOAT CM_GetWar3MapVertexHeight(LPCWAR3MAPVERTEX vert) {
    return DECODE_HEIGHT(vert->accurate_height) + vert->level * TILE_SIZE - HEIGHT_COR;
}

FLOAT CM_GetHeightAtPoint(FLOAT sx, FLOAT sy) {
#ifdef WOW
    (void)sx;
    (void)sy;
    return WOW_START_POSITION_Z;
#else
    FLOAT x = (sx - world.map->center.x) / TILE_SIZE;
    FLOAT y = (sy - world.map->center.y) / TILE_SIZE;
    FLOAT fx = floorf(x);
    FLOAT fy = floorf(y);
    FLOAT a = CM_GetWar3MapVertexHeight(CM_GetWar3MapVertex(fx, fy));
    FLOAT b = CM_GetWar3MapVertexHeight(CM_GetWar3MapVertex(fx + 1, fy));
    FLOAT c = CM_GetWar3MapVertexHeight(CM_GetWar3MapVertex(fx, fy + 1));
    FLOAT d = CM_GetWar3MapVertexHeight(CM_GetWar3MapVertex(fx + 1, fy + 1));
    FLOAT ab = LerpNumber(a, b, x - fx);
    FLOAT cd = LerpNumber(c, d, x - fx);
    return LerpNumber(ab, cd, y - fy);
#endif
}

LPDOODAD CM_GetDoodads(void) {
    return world.doodads;
}

//LPCMAPPLAYER CM_GetPlayer(DWORD index) {
//    return &world.info.players[index&(MAX_PLAYERS-1)];
//}

DWORD CM_GetLocalPlayerNumber(void) {
    FOR_LOOP(i, MAX_PLAYERS) {
        LPCMAPPLAYER player = world.info.players+i;
        if (player->playerType == kPlayerTypeHuman) {
            return i;
        }
    }
    return 0;
}

LPCMAPINFO CM_GetMapInfo(void) {
    return &world.info;
}

VECTOR2 CM_GetNormalizedMapPosition(FLOAT x, FLOAT y) {
#ifdef WOW
    return (VECTOR2){ x, y };
#else
    FLOAT _x = (x - world.map->center.x) / ((world.map->width-1) * TILE_SIZE);
    FLOAT _y = (y - world.map->center.y) / ((world.map->height-1) * TILE_SIZE);
    return (VECTOR2) { _x, _y };
#endif
}

VECTOR2 CM_GetDenormalizedMapPosition(FLOAT x, FLOAT y) {
#ifdef WOW
    return (VECTOR2){ x, y };
#else
    FLOAT _x = x * (world.map->width-1) * TILE_SIZE + world.map->center.x;
    FLOAT _y = y * (world.map->height-1) * TILE_SIZE + world.map->center.y;
    return (VECTOR2) { _x, _y };
#endif
}

void CM_ReleaseModel(void) {
    MapInfo_Release(&world.info);
}

BOX2 CM_GetWorldBounds(void) {
#ifdef WOW
    float const half = 32.0f * WOW_TILE_SIZE;
    return (BOX2){
        .min = { -half, -half },
        .max = { half, half },
    };
#else
    return MAKE(BOX2,
                .min = world.map->center,
                .max = {
                    .x = (world.map->width-1) *  TILE_SIZE + world.map->center.x,
                    .y = (world.map->height-1) * TILE_SIZE + world.map->center.y,
                });
#endif
}
