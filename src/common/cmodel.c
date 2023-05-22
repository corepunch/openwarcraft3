#include "common.h"
#include <StormLib.h>

static struct {
    LPWAR3MAP map;
    mapInfo_t info;
    struct size2 pathmapSize;
    struct PathMapNode *pathmap;
    struct Doodad *doodads;
} cmodel;

struct PathMapNode const *CM_PathMapNode(LPCWAR3MAP terrain, DWORD x, DWORD y) {
    int const index = x + y * cmodel.pathmapSize.width;
    return &cmodel.pathmap[index];
}

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
    mapInfo_t *info = &cmodel.info;
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
    SFileReadFile(file, &info->playableArea, sizeof(SIZE2), NULL, NULL);
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
        
        ADD_TO_LIST(doodad, cmodel.doodads);
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

static void CM_ReadUnits(HANDLE archive) {
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
        ADD_TO_LIST(doodad, cmodel.doodads);
    }

    SFileCloseFile(file);
}

static void CM_ReadPathMap(HANDLE archive) {
    HANDLE file;
    DWORD header, version;
    SFileOpenFileEx(archive, "war3map.wpm", SFILE_OPEN_FROM_MPQ, &file);
    SFileReadFile(file, &header, 4, NULL, NULL);
    SFileReadFile(file, &version, 4, NULL, NULL);
    SFileReadFile(file, &cmodel.pathmapSize, 8, NULL, NULL);
    int const pathmapblocksize = cmodel.pathmapSize.width * cmodel.pathmapSize.height;
    cmodel.pathmap = MemAlloc(pathmapblocksize);
    SFileReadFile(file, cmodel.pathmap, pathmapblocksize, 0, 0);
    SFileCloseFile(file);
}

static void CM_ReadHeightmap(HANDLE archive) {
    cmodel.map = MemAlloc(sizeof(WAR3MAP));
    HANDLE file;
    SFileOpenFileEx(archive, "war3map.w3e", SFILE_OPEN_FROM_MPQ, &file);
    SFileReadFile(file, &cmodel.map->header, 4, NULL, NULL);
    SFileReadFile(file, &cmodel.map->version, 4, NULL, NULL);
    SFileReadFile(file, &cmodel.map->tileset, 1, NULL, NULL);
    SFileReadFile(file, &cmodel.map->custom, 4, NULL, NULL);
    SFileReadArray(file, cmodel.map, grounds, 4, MemAlloc);
    SFileReadArray(file, cmodel.map, cliffs, 4, MemAlloc);
    SFileReadFile(file, &cmodel.map->width, 4, NULL, NULL);
    SFileReadFile(file, &cmodel.map->height, 4, NULL, NULL);
    SFileReadFile(file, &cmodel.map->center, 8, NULL, NULL);
    int const vertexblocksize = MAP_VERTEX_SIZE * cmodel.map->width * cmodel.map->height;
    cmodel.map->vertices = MemAlloc(vertexblocksize);
    SFileReadFile(file, cmodel.map->vertices, vertexblocksize, 0, 0);
    SFileCloseFile(file);
}

void CM_LoadMap(LPCSTR mapFilename) {
    HANDLE mapArchive;
    memset(&cmodel, 0, sizeof(cmodel));
    FS_ExtractFile(mapFilename, TMP_MAP);
    SFileOpenArchive(TMP_MAP, 0, 0, &mapArchive);
    CM_ReadPathMap(mapArchive);
    CM_ReadDoodads(mapArchive);
    CM_ReadUnits(mapArchive);
    CM_ReadHeightmap(mapArchive);
    CM_ReadInfo(mapArchive);
    SFileCloseArchive(mapArchive);
}

VECTOR3 CM_PointIntoHeightmap(LPCVECTOR3 point) {
    return (VECTOR3) {
        .x = (point->x - cmodel.map->center.x) / TILESIZE,
        .y = (point->y - cmodel.map->center.y) / TILESIZE,
        .z = point->z
    };
}

VECTOR3 CM_PointFromHeightmap(LPCVECTOR3 point) {
    return (VECTOR3) {
        .x = point->x * TILESIZE + cmodel.map->center.x,
        .y = point->y * TILESIZE + cmodel.map->center.y,
        .z = point->z
    };
}

static LPCWAR3MAPVERTEX CM_GetWar3MapVertex(DWORD x, DWORD y) {
    int const index = x + y * cmodel.map->width;
    char const *ptr = ((char const *)cmodel.map->vertices) + index * MAP_VERTEX_SIZE;
    return (LPCWAR3MAPVERTEX)ptr;
}

static float CM_GetWar3MapVertexHeight(LPCWAR3MAPVERTEX vert) {
    return DECODE_HEIGHT(vert->accurate_height) + vert->level * TILESIZE - HEIGHT_COR;
}

static short CM_GetHeightMapValue(int x, int y) {
    return CM_GetWar3MapVertexHeight(CM_GetWar3MapVertex(x, y));
}

float CM_GetHeightAtPoint(float sx, float sy) {
    float x = (sx - cmodel.map->center.x) / TILESIZE;
    float y = (sy - cmodel.map->center.y) / TILESIZE;
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

bool CM_IntersectLineWithHeightmap(LPCLINE3 _line, LPVECTOR3 output) {
    LINE3 line = {
        .a = CM_PointIntoHeightmap(&_line->a),
        .b = CM_PointIntoHeightmap(&_line->b),
    };
    FOR_LOOP(x, cmodel.map->width) {
        FOR_LOOP(y, cmodel.map->height) {
            TRIANGLE3 const tri1 = {
                { x, y, CM_GetHeightMapValue(x, y) },
                { x+1, y, CM_GetHeightMapValue(x+1, y) },
                { x+1, y+1, CM_GetHeightMapValue(x, y+1) },
            };
            TRIANGLE3 const tri2 = {
                { x+1, y+1, CM_GetHeightMapValue(x, y+1) },
                { x, y+1, CM_GetHeightMapValue(x, y+1) },
                { x, y, CM_GetHeightMapValue(x, y) },
            };
            if (Line3_intersect_triangle(&line, &tri1, output)) {
                *output = CM_PointFromHeightmap(output);
                return true;
            }
            if (Line3_intersect_triangle(&line, &tri2, output)) {
                *output = CM_PointFromHeightmap(output);
                return true;
            }
        }
    }
    return false;
}

LPDOODAD CM_GetDoodads(void) {
    return cmodel.doodads;
}

mapPlayer_t const *CM_GetPlayer(DWORD index) {
    if (index < cmodel.info.num_players) {
        return &cmodel.info.players[index];
    } else {
        return NULL;
    }
}
