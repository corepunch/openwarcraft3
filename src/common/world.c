#include "common.h"
#include <StormLib.h>

static struct {
    LPWAR3MAP map;
    mapInfo_t info;
    struct Doodad *doodads;
} world;

void CM_ReadPathMap(HANDLE archive);

void SFileReadString(HANDLE file, LPSTR *lppString) {
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
    *lppString = MemAlloc(stringLength);
    SFileSetFilePointer(file, filePosition, 0, FILE_BEGIN);
    SFileReadFile(file, *lppString, stringLength, NULL, NULL);
}

static void CM_ReadInfo(HANDLE archive) {
    mapInfo_t *info = &world.info;
    HANDLE file;
    SFileOpenFileEx(archive, "war3map.w3i", SFILE_OPEN_FROM_MPQ, &file);
    SFileReadFile(file, &info->fileFormat, 4, NULL, NULL);
    SFileReadFile(file, &info->numberOfSaves, sizeof(DWORD), NULL, NULL);
    SFileReadFile(file, &info->editorVersion, sizeof(DWORD), NULL, NULL);
    SFileReadString(file, &info->mapName);
    SFileReadString(file, &info->mapAuthor);
    SFileReadString(file, &info->mapDescription);
    SFileReadString(file, &info->playersRecommended);
    SFileReadFile(file, &info->cameraBounds, sizeof(mapCameraBounds_t), NULL, NULL);
    SFileReadFile(file, &info->playableArea, sizeof(size2_t), NULL, NULL);
    SFileReadFile(file, &info->flags, sizeof(DWORD), NULL, NULL);
    SFileReadFile(file, &info->mainGroundType, sizeof(char), NULL, NULL);
    SFileReadFile(file, &info->campaignBackgroundNumber, sizeof(DWORD), NULL, NULL);
    SFileReadString(file, &info->loadingScreenText);
    SFileReadString(file, &info->loadingScreenTitle);
    SFileReadString(file, &info->loadingScreenSubtitle);
    SFileReadFile(file, &info->loadingScreenNumber, sizeof(DWORD), NULL, NULL);
    SFileReadString(file, &info->prologueScreenText);
    SFileReadString(file, &info->prologueScreenTitle);
    SFileReadString(file, &info->prologueScreenSubtitle);

    SFileReadFile(file, &info->num_players, sizeof(DWORD), NULL, NULL);
    info->players = MemAlloc(sizeof(mapPlayer_t) * info->num_players);
    FOR_LOOP(i, info->num_players) {
        mapPlayer_t *player = &info->players[i];
        SFileReadFile(file, &player->internalPlayerNumber, sizeof(DWORD), NULL, NULL);
        SFileReadFile(file, &player->playerType, sizeof(playerType_t), NULL, NULL);
        SFileReadFile(file, &player->playerRace, sizeof(playerRace_t), NULL, NULL);
        SFileReadFile(file, &player->flags, sizeof(DWORD), NULL, NULL);
        SFileReadString(file, &player->playerName);
        SFileReadFile(file, &player->startingPosition, sizeof(VECTOR2), NULL, NULL);
        SFileReadFile(file, &player->allyLowPrioritiesFlags, sizeof(DWORD), NULL, NULL);
        SFileReadFile(file, &player->allyHighPrioritiesFlags, sizeof(DWORD), NULL, NULL);
    }

    SFileReadFile(file, &info->num_forces, sizeof(DWORD), NULL, NULL);
    info->forces = MemAlloc(sizeof(mapForce_t) * info->num_forces);
    FOR_LOOP(i, info->num_forces) {
        mapForce_t *force = &info->forces[i];
        SFileReadFile(file, &force->focesFlags, sizeof(DWORD), NULL, NULL);
        SFileReadFile(file, &force->playerMasks, sizeof(DWORD), NULL, NULL);
        SFileReadString(file, &force->forceName);
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

    SFileReadFile(file, &info->num_techAvailabilities, sizeof(DWORD), NULL, NULL);
    info->techAvailabilities = MemAlloc(sizeof(mapTechAvailability_t) * info->num_techAvailabilities);
    FOR_LOOP(i, info->num_techAvailabilities) {
        mapTechAvailability_t *tech = &info->techAvailabilities[i];
        SFileReadFile(file, &tech->playerFlags, sizeof(DWORD), NULL, NULL);
        SFileReadFile(file, &tech->techID, sizeof(DWORD), NULL, NULL);
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
}

static void MapInfo_Release(mapInfo_t *mapInfo) {
    FOR_LOOP(i, mapInfo->num_players) {
        SAFE_DELETE(mapInfo->players[i].playerName, MemFree);
    }
    FOR_LOOP(i, mapInfo->num_forces) {
        SAFE_DELETE(mapInfo->forces[i].forceName, MemFree);
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
    SAFE_DELETE(mapInfo->players, MemFree);
    SAFE_DELETE(mapInfo->forces, MemFree);
    SAFE_DELETE(mapInfo->upgradeAvailabilities, MemFree);
    SAFE_DELETE(mapInfo->techAvailabilities, MemFree);
    SAFE_DELETE(mapInfo->randomUnits, MemFree);
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
        SFileReadFile(file, &doodad->angle, sizeof(float), NULL, NULL);
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
    SFileReadFile(file, &unit->angle, sizeof(float), NULL, NULL);
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
    SFileReadFile(file, &unit->targetAcquisition, sizeof(float), NULL, NULL); // (-1 = normal, -2 = camp)

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

void CM_ReadUnits(HANDLE archive) {
    DWORD version;
    DWORD num_units;
    HANDLE file;
//    SFileExtractFile(archive, "war3map.w3u", "/Users/igor/Desktop/war3map.w3u", 0);
    SFileOpenFileEx(archive, "war3map.w3u", SFILE_OPEN_FROM_MPQ, &file);
    SFileReadFile(file, &version, 4, NULL, NULL);
    SFileReadFile(file, &num_units, 4, NULL, NULL);
    SFileCloseFile(file);
}

void CM_LoadMap(LPCSTR mapFilename) {
    HANDLE mapArchive;
    memset(&world, 0, sizeof(world));
    FS_ExtractFile(mapFilename, TMP_MAP);
    SFileOpenArchive(TMP_MAP, 0, 0, &mapArchive);
    CM_ReadPathMap(mapArchive);
    CM_ReadDoodads(mapArchive);
    CM_ReadUnitDoodads(mapArchive);
    CM_ReadHeightmap(mapArchive);
    CM_ReadInfo(mapArchive);
    CM_ReadUnits(mapArchive);
        
//    HANDLE file;
//    SFileOpenFileEx(mapArchive, "war3map.j", SFILE_OPEN_FROM_MPQ, &file);
//    char ch;
//    while (SFileReadFile(file, &ch, 1, NULL, NULL)) {
//        printf("%c", ch);
//    }
//    SFileCloseFile(file);
//    printf("\n");

    SFileCloseArchive(mapArchive);
}

static LPCWAR3MAPVERTEX CM_GetWar3MapVertex(DWORD x, DWORD y) {
    int const index = x + y * world.map->width;
    char const *ptr = ((char const *)world.map->vertices) + index * MAP_VERTEX_SIZE;
    return (LPCWAR3MAPVERTEX)ptr;
}

static float CM_GetWar3MapVertexHeight(LPCWAR3MAPVERTEX vert) {
    return DECODE_HEIGHT(vert->accurate_height) + vert->level * TILESIZE - HEIGHT_COR;
}

float CM_GetHeightAtPoint(float sx, float sy) {
    float x = (sx - world.map->center.x) / TILESIZE;
    float y = (sy - world.map->center.y) / TILESIZE;
    float fx = floorf(x);
    float fy = floorf(y);
    float a = CM_GetWar3MapVertexHeight(CM_GetWar3MapVertex(fx, fy));
    float b = CM_GetWar3MapVertexHeight(CM_GetWar3MapVertex(fx + 1, fy));
    float c = CM_GetWar3MapVertexHeight(CM_GetWar3MapVertex(fx, fy + 1));
    float d = CM_GetWar3MapVertexHeight(CM_GetWar3MapVertex(fx + 1, fy + 1));
    float ab = LerpNumber(a, b, x - fx);
    float cd = LerpNumber(c, d, x - fx);
    return LerpNumber(ab, cd, y - fy);
}

LPDOODAD CM_GetDoodads(void) {
    return world.doodads;
}

mapPlayer_t const *CM_GetPlayer(DWORD index) {
    if (index < world.info.num_players) {
        return &world.info.players[index];
    } else {
        return NULL;
    }
}

VECTOR2 CM_GetNormalizedMapPosition(float x, float y) {
    float _x = (x - world.map->center.x) / ((world.map->width-1) * TILESIZE);
    float _y = (y - world.map->center.y) / ((world.map->height-1) * TILESIZE);
    return (VECTOR2) { _x, _y };
}

VECTOR2 CM_GetDenormalizedMapPosition(float x, float y) {
    float _x = x * (world.map->width-1) * TILESIZE + world.map->center.x;
    float _y = y * (world.map->height-1) * TILESIZE + world.map->center.y;
    return (VECTOR2) { _x, _y };
}

void CM_ReleaseModel(void) {
    MapInfo_Release(&world.info);
}
