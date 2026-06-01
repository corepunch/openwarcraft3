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

#ifdef WOW
static VECTOR3 cm_wow_spawn_position = { 0.0f, 0.0f, 0.0f };
static char cm_wow_map_dir[PATH_MAX] = { 0 };
static char cm_wow_map_name[128] = { 0 };

#define CM_WOW_ADT_SIZE 533.333313f
#define CM_WOW_ADT_UNIT_SIZE (CM_WOW_ADT_SIZE / 16.0f / 8.0f)
#define CM_WOW_MCVT_COUNT (9 * 9 + 8 * 8)

typedef struct {
    BOOL has_heights;
    VECTOR3 position;
    float heights[CM_WOW_MCVT_COUNT];
} cmWowChunkHeight_t;

typedef struct {
    BOOL loaded;
    BOOL valid;
    int tile_x;
    int tile_y;
    cmWowChunkHeight_t chunks[16][16];
} cmWowAdtHeightCache_t;

static cmWowAdtHeightCache_t cm_wow_height_cache = { 0 };

static DWORD CM_WowRead32(BYTE const *p) {
    return ((DWORD)p[0]) | ((DWORD)p[1] << 8) | ((DWORD)p[2] << 16) | ((DWORD)p[3] << 24);
}

static FLOAT CM_WowReadFloat(BYTE const *p) {
    FLOAT value;
    memcpy(&value, p, sizeof(value));
    return value;
}

static BOOL CM_WowTagEquals(BYTE const *tag, LPCSTR reversed) {
    return memcmp(tag, reversed, 4) == 0;
}

static LPCSTR CM_WowDbcString(BYTE const *string_block, DWORD string_size, DWORD offset) {
    if (offset >= string_size) {
        return NULL;
    }
    return (LPCSTR)(string_block + offset);
}

static BOOL CM_WowExtractMapName(LPCSTR mapFilename, LPSTR out, size_t out_size) {
    LPCSTR start;
    LPCSTR slash;
    LPCSTR backslash;
    LPCSTR dot;
    size_t len;

    if (!mapFilename || !out || out_size == 0) {
        return false;
    }

    slash = strrchr(mapFilename, '/');
    backslash = strrchr(mapFilename, '\\');
    start = slash && backslash ? MAX(slash, backslash) + 1 : slash ? slash + 1 : backslash ? backslash + 1 : mapFilename;
    dot = strrchr(start, '.');
    len = dot && dot > start ? (size_t)(dot - start) : strlen(start);
    if (len >= out_size) {
        len = out_size - 1;
    }
    memcpy(out, start, len);
    out[len] = '\0';
    return len > 0;
}

static void CM_WowSetMapPath(LPCSTR mapFilename) {
    LPCSTR path = mapFilename && *mapFilename ? mapFilename : "World/Maps/Azeroth/Azeroth.wdt";
    LPCSTR slash = strrchr(path, '/');
    LPCSTR backslash = strrchr(path, '\\');
    LPCSTR base = slash && backslash ? MAX(slash, backslash) : slash ? slash : backslash;
    size_t dir_len;
    size_t name_len;

    memset(&cm_wow_height_cache, 0, sizeof(cm_wow_height_cache));
    cm_wow_map_dir[0] = '\0';
    cm_wow_map_name[0] = '\0';

    base = base ? base + 1 : path;
    dir_len = (size_t)(base - path);
    if (dir_len > 0) {
        dir_len = MIN(dir_len, sizeof(cm_wow_map_dir) - 1);
        memcpy(cm_wow_map_dir, path, dir_len);
        cm_wow_map_dir[dir_len] = '\0';
        if (dir_len > 0 && (cm_wow_map_dir[dir_len - 1] == '/' || cm_wow_map_dir[dir_len - 1] == '\\')) {
            cm_wow_map_dir[dir_len - 1] = '\0';
        }
    }

    name_len = strlen(base);
    if (name_len > 4 && !strcasecmp(base + name_len - 4, ".wdt")) {
        name_len -= 4;
    }
    name_len = MIN(name_len, sizeof(cm_wow_map_name) - 1);
    memcpy(cm_wow_map_name, base, name_len);
    cm_wow_map_name[name_len] = '\0';

    if (!cm_wow_map_dir[0] && cm_wow_map_name[0]) {
        snprintf(cm_wow_map_dir, sizeof(cm_wow_map_dir), "World/Maps/%s", cm_wow_map_name);
    }
}

static int CM_WowAdtIndexForWorldCoord(float coord) {
    return (int)floorf(32.0f - coord / CM_WOW_ADT_SIZE);
}

static LPCSTR CM_WowAdtPath(int tile_x, int tile_y, LPSTR out, DWORD out_size) {
    if (!cm_wow_map_dir[0] || !cm_wow_map_name[0] || !out || !out_size) {
        return NULL;
    }
    snprintf(out, out_size, "%s/%s_%d_%d.adt", cm_wow_map_dir, cm_wow_map_name, tile_x, tile_y);
    return out;
}

static void CM_WowLoadAdtHeights(int tile_x, int tile_y) {
    PATHSTR path;
    LPBYTE data;
    DWORD size = 0;
    DWORD offset = 0;

    memset(&cm_wow_height_cache, 0, sizeof(cm_wow_height_cache));
    cm_wow_height_cache.loaded = true;
    cm_wow_height_cache.tile_x = tile_x;
    cm_wow_height_cache.tile_y = tile_y;

    if (!CM_WowAdtPath(tile_x, tile_y, path, sizeof(path))) {
        return;
    }

    data = FS_ReadFile(path, &size);
    if (!data || !size) {
        SAFE_DELETE(data, FS_FreeFile);
        return;
    }

    while (offset + 8 <= size) {
        BYTE const *tag = data + offset;
        DWORD chunk_size = CM_WowRead32(data + offset + 4);
        BYTE const *chunk = data + offset + 8;

        offset += 8;
        if (offset + chunk_size > size) {
            break;
        }

        if (CM_WowTagEquals(tag, "KNCM") && chunk_size >= 0x80) {
            DWORD sub = 0x80;
            DWORD index_x = CM_WowRead32(chunk + 0x04);
            DWORD index_y = CM_WowRead32(chunk + 0x08);
            cmWowChunkHeight_t *height_chunk = NULL;

            if (index_x < 16 && index_y < 16) {
                height_chunk = &cm_wow_height_cache.chunks[index_y][index_x];
                memcpy(&height_chunk->position, chunk + 0x68, sizeof(height_chunk->position));
            }

            while (height_chunk && sub + 8 <= chunk_size) {
                BYTE const *subtag = chunk + sub;
                DWORD sub_size = CM_WowRead32(chunk + sub + 4);
                BYTE const *subchunk = chunk + sub + 8;
                BOOL is_mcnr = CM_WowTagEquals(subtag, "RNCM");

                sub += 8;
                if (sub + sub_size > chunk_size) {
                    break;
                }
                if (CM_WowTagEquals(subtag, "TVCM") && sub_size >= sizeof(height_chunk->heights)) {
                    memcpy(height_chunk->heights, subchunk, sizeof(height_chunk->heights));
                    height_chunk->has_heights = true;
                    cm_wow_height_cache.valid = true;
                }
                sub += sub_size;
                if (is_mcnr && sub_size == 145 * 3 && sub + 13 <= chunk_size) {
                    sub += 13;
                }
            }
        }

        offset += chunk_size;
    }

    FS_FreeFile(data);
}

static BOOL CM_WowBarycentricHeight(float px,
                                    float py,
                                    float ax,
                                    float ay,
                                    float ah,
                                    float bx,
                                    float by,
                                    float bh,
                                    float cx,
                                    float cy,
                                    float ch,
                                    float *height) {
    float den = (by - cy) * (ax - cx) + (cx - bx) * (ay - cy);
    float wa;
    float wb;
    float wc;

    if (fabsf(den) < 0.000001f || !height) {
        return false;
    }
    wa = ((by - cy) * (px - cx) + (cx - bx) * (py - cy)) / den;
    wb = ((cy - ay) * (px - cx) + (ax - cx) * (py - cy)) / den;
    wc = 1.0f - wa - wb;
    if (wa < -0.0001f || wb < -0.0001f || wc < -0.0001f) {
        return false;
    }
    *height = wa * ah + wb * bh + wc * ch;
    return true;
}

static BOOL CM_WowHeightInCell(float const *heights, int row, int col, float fx, float fy, float *height) {
    int base = row * 17 + col;
    float h_tl = heights[base];
    float h_tr = heights[base + 1];
    float h_bl = heights[base + 17];
    float h_br = heights[base + 18];
    float h_c = heights[base + 9];

    return CM_WowBarycentricHeight(fx, fy, 0.5f, 0.5f, h_c, 0.0f, 0.0f, h_tl, 1.0f, 0.0f, h_bl, height) ||
           CM_WowBarycentricHeight(fx, fy, 0.5f, 0.5f, h_c, 0.0f, 1.0f, h_tr, 0.0f, 0.0f, h_tl, height) ||
           CM_WowBarycentricHeight(fx, fy, 0.5f, 0.5f, h_c, 1.0f, 1.0f, h_br, 0.0f, 1.0f, h_tr, height) ||
           CM_WowBarycentricHeight(fx, fy, 0.5f, 0.5f, h_c, 1.0f, 0.0f, h_bl, 1.0f, 1.0f, h_br, height);
}

static BOOL CM_WowTerrainHeightAtPoint(FLOAT sx, FLOAT sy, FLOAT *height) {
    int tile_x = CM_WowAdtIndexForWorldCoord(sy);
    int tile_y = CM_WowAdtIndexForWorldCoord(sx);

    if (!height || tile_x < 0 || tile_x >= 64 || tile_y < 0 || tile_y >= 64) {
        return false;
    }
    if (!cm_wow_height_cache.loaded ||
        cm_wow_height_cache.tile_x != tile_x ||
        cm_wow_height_cache.tile_y != tile_y) {
        CM_WowLoadAdtHeights(tile_x, tile_y);
    }
    if (!cm_wow_height_cache.valid) {
        return false;
    }

    FOR_LOOP(row, 16) {
        FOR_LOOP(col, 16) {
            cmWowChunkHeight_t const *chunk = &cm_wow_height_cache.chunks[row][col];
            float local_row;
            float local_col;
            int cell_row;
            int cell_col;
            float cell_height;

            if (!chunk->has_heights ||
                sx > chunk->position.x + 0.001f ||
                sx < chunk->position.x - 8.0f * CM_WOW_ADT_UNIT_SIZE - 0.001f ||
                sy > chunk->position.y + 0.001f ||
                sy < chunk->position.y - 8.0f * CM_WOW_ADT_UNIT_SIZE - 0.001f) {
                continue;
            }

            local_row = (chunk->position.x - sx) / CM_WOW_ADT_UNIT_SIZE;
            local_col = (chunk->position.y - sy) / CM_WOW_ADT_UNIT_SIZE;
            cell_row = (int)floorf(MIN(local_row, 7.9999f));
            cell_col = (int)floorf(MIN(local_col, 7.9999f));
            if (cell_row < 0 || cell_row >= 8 || cell_col < 0 || cell_col >= 8) {
                continue;
            }
            if (CM_WowHeightInCell(chunk->heights,
                                   cell_row,
                                   cell_col,
                                   local_row - cell_row,
                                   local_col - cell_col,
                                   &cell_height)) {
                *height = chunk->position.z + cell_height;
                return true;
            }
        }
    }

    return false;
}

static BOOL CM_WowValidDbc(BYTE const *data, DWORD size, DWORD *records, DWORD *fields, DWORD *record_size, DWORD *string_size) {
    if (!data || size <= 20 || memcmp(data, "WDBC", 4) != 0) {
        return false;
    }

    *records = CM_WowRead32(data + 4);
    *fields = CM_WowRead32(data + 8);
    *record_size = CM_WowRead32(data + 12);
    *string_size = CM_WowRead32(data + 16);

    if (*fields == 0 || *record_size < *fields * sizeof(DWORD) ||
        20 + *records * *record_size + *string_size > size) {
        return false;
    }
    return true;
}

static BOOL CM_WowFindMapId(LPCSTR map_name, DWORD *map_id) {
    LPBYTE data;
    DWORD size = 0;
    DWORD records;
    DWORD fields;
    DWORD record_size;
    DWORD string_size;
    BYTE const *records_base;
    BYTE const *strings_base;

    if (!map_name || !*map_name || !map_id) {
        return false;
    }

    data = FS_ReadFile("DBFilesClient\\Map.dbc", &size);
    if (!CM_WowValidDbc(data, size, &records, &fields, &record_size, &string_size)) {
        SAFE_DELETE(data, FS_FreeFile);
        return false;
    }

    records_base = data + 20;
    strings_base = records_base + records * record_size;
    FOR_LOOP(record_index, records) {
        BYTE const *record = records_base + record_index * record_size;
        FOR_LOOP(field_index, fields) {
            DWORD string_offset = CM_WowRead32(record + field_index * sizeof(DWORD));
            LPCSTR value = CM_WowDbcString(strings_base, string_size, string_offset);
            if (value && *value && !strcasecmp(value, map_name)) {
                *map_id = CM_WowRead32(record);
                FS_FreeFile(data);
                return true;
            }
        }
    }

    FS_FreeFile(data);
    return false;
}

static BOOL CM_WowFindWorldSafeLoc(DWORD map_id, LPVECTOR3 spawn, LPSTR name, size_t name_size) {
    LPBYTE data;
    DWORD size = 0;
    DWORD records;
    DWORD fields;
    DWORD record_size;
    DWORD string_size;
    BYTE const *records_base;
    BYTE const *strings_base;

    if (!spawn) {
        return false;
    }

    data = FS_ReadFile("DBFilesClient\\WorldSafeLocs.dbc", &size);
    if (!CM_WowValidDbc(data, size, &records, &fields, &record_size, &string_size) ||
        fields < 5 || record_size < 5 * sizeof(DWORD)) {
        SAFE_DELETE(data, FS_FreeFile);
        return false;
    }

    records_base = data + 20;
    strings_base = records_base + records * record_size;
    FOR_LOOP(record_index, records) {
        BYTE const *record = records_base + record_index * record_size;
        if (CM_WowRead32(record + sizeof(DWORD)) != map_id) {
            continue;
        }

        spawn->x = CM_WowReadFloat(record + 2 * sizeof(DWORD));
        spawn->y = CM_WowReadFloat(record + 3 * sizeof(DWORD));
        spawn->z = CM_WowReadFloat(record + 4 * sizeof(DWORD));
        if (name && name_size) {
            name[0] = '\0';
            for (DWORD field_index = 5; field_index < fields; field_index++) {
                DWORD string_offset = CM_WowRead32(record + field_index * sizeof(DWORD));
                LPCSTR value = CM_WowDbcString(strings_base, string_size, string_offset);
                if (value && *value) {
                    strncpy(name, value, name_size - 1);
                    name[name_size - 1] = '\0';
                    break;
                }
            }
        }
        FS_FreeFile(data);
        return true;
    }

    FS_FreeFile(data);
    return false;
}

static void CM_WowChooseSpawn(LPCSTR mapFilename) {
    char map_name[128] = { 0 };
    char safe_loc_name[128] = { 0 };
    DWORD map_id = 0;
    BOOL has_map_id;
    BOOL has_safe_loc;

    cm_wow_spawn_position = (VECTOR3){ 0.0f, 0.0f, 0.0f };
    world.info.players[0].used = true;
    world.info.players[0].playerType = kPlayerTypeHuman;
    CM_WowSetMapPath(mapFilename);

    if (!CM_WowExtractMapName(mapFilename, map_name, sizeof(map_name))) {
        world.info.players[0].startingPosition = (VECTOR2){ cm_wow_spawn_position.x, cm_wow_spawn_position.y };
        fprintf(stderr, "CM_LoadMap: WoW spawn fallback at %.3f %.3f %.3f (no map name)\n",
                cm_wow_spawn_position.x, cm_wow_spawn_position.y, cm_wow_spawn_position.z);
        return;
    }

    has_map_id = CM_WowFindMapId(map_name, &map_id);
    has_safe_loc = has_map_id && CM_WowFindWorldSafeLoc(map_id, &cm_wow_spawn_position, safe_loc_name, sizeof(safe_loc_name));
    world.info.players[0].startingPosition = (VECTOR2){ cm_wow_spawn_position.x, cm_wow_spawn_position.y };

    if (has_safe_loc) {
        fprintf(stderr,
                "CM_LoadMap: WoW map %s id=%u spawn from WorldSafeLocs%s%s at %.3f %.3f %.3f\n",
                map_name,
                (unsigned)map_id,
                safe_loc_name[0] ? " " : "",
                safe_loc_name,
                cm_wow_spawn_position.x,
                cm_wow_spawn_position.y,
                cm_wow_spawn_position.z);
    } else if (has_map_id) {
        fprintf(stderr,
                "CM_LoadMap: WoW map %s id=%u has no WorldSafeLocs entry, spawn fallback at %.3f %.3f %.3f\n",
                map_name,
                (unsigned)map_id,
                cm_wow_spawn_position.x,
                cm_wow_spawn_position.y,
                cm_wow_spawn_position.z);
    } else {
        fprintf(stderr,
                "CM_LoadMap: WoW map %s has no Map.dbc entry, spawn fallback at %.3f %.3f %.3f\n",
                map_name,
                cm_wow_spawn_position.x,
                cm_wow_spawn_position.y,
                cm_wow_spawn_position.z);
    }
}
#endif

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

static DWORD SFileBytesRemaining(HANDLE file) {
    DWORD position = SFileSetFilePointer(file, 0, 0, FILE_CURRENT);
    DWORD size = SFileGetFileSize(file, NULL);

    if (position >= size) {
        return 0;
    }
    return size - position;
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
    if (info->fileFormat >= 28) {
        SFileReadFile(file, &info->gameVersionMajor, sizeof(DWORD), NULL, NULL);
        SFileReadFile(file, &info->gameVersionMinor, sizeof(DWORD), NULL, NULL);
        SFileReadFile(file, &info->gameVersionPatch, sizeof(DWORD), NULL, NULL);
        SFileReadFile(file, &info->gameVersionBuild, sizeof(DWORD), NULL, NULL);
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
    if (info->fileFormat >= 25) {
        SFileReadString(file, &info->loadingScreenModel);
    }
    SFileReadString(file, &info->loadingScreenText);
    SFileReadString(file, &info->loadingScreenTitle);
    SFileReadString(file, &info->loadingScreenSubtitle);
    if (info->fileFormat >= 25) {
        SFileReadFile(file, &info->gameDataSet, sizeof(DWORD), NULL, NULL);
    } else {
        SFileReadFile(file, &info->loadingScreenNumber, sizeof(DWORD), NULL, NULL);
    }
    if (info->fileFormat >= 25) {
        SFileReadString(file, &info->prologueScreenModel);
    }
    SFileReadString(file, &info->prologueScreenText);
    SFileReadString(file, &info->prologueScreenTitle);
    SFileReadString(file, &info->prologueScreenSubtitle);
    if (info->fileFormat >= 25) {
        SFileReadFile(file, &info->fogStyle, sizeof(DWORD), NULL, NULL);
        SFileReadFile(file, &info->fogStartZ, sizeof(FLOAT), NULL, NULL);
        SFileReadFile(file, &info->fogEndZ, sizeof(FLOAT), NULL, NULL);
        SFileReadFile(file, &info->fogDensity, sizeof(FLOAT), NULL, NULL);
        SFileReadFile(file, &info->fogColor, sizeof(COLOR32), NULL, NULL);
        SFileReadFile(file, &info->weatherID, sizeof(DWORD), NULL, NULL);
        SFileReadString(file, &info->soundEnvironment);
        SFileReadFile(file, &info->lightEnvironmentTileset, sizeof(BYTE), NULL, NULL);
        SFileReadFile(file, &info->waterColor, sizeof(COLOR32), NULL, NULL);
    }
    if (info->fileFormat >= 28) {
        SFileReadFile(file, &info->scriptType, sizeof(DWORD), NULL, NULL);
    }
    if (info->fileFormat >= 31) {
        SFileReadFile(file, &info->supportedModes, sizeof(DWORD), NULL, NULL);
        SFileReadFile(file, &info->gameDataVersion, sizeof(DWORD), NULL, NULL);
    }
    if (info->fileFormat >= 32) {
        SFileReadFile(file, &info->defaultZoomOverride, sizeof(DWORD), NULL, NULL);
        SFileReadFile(file, &info->maximumZoomOverride, sizeof(DWORD), NULL, NULL);
    }
    if (info->fileFormat >= 33) {
        SFileReadFile(file, &info->minimumZoomOverride, sizeof(DWORD), NULL, NULL);
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
        if (info->fileFormat >= 31) {
            SFileReadFile(file, &player->enemyLowPrioritiesFlags, sizeof(DWORD), NULL, NULL);
            SFileReadFile(file, &player->enemyHighPrioritiesFlags, sizeof(DWORD), NULL, NULL);
        }
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
        SFileReadFile(file, &table->tableNumber, sizeof(DWORD), NULL, NULL);
        SFileReadString(file, &table->tableName);
        SFileReadFile(file, &table->num_positions, sizeof(DWORD), NULL, NULL);
        if (table->num_positions > 0) {
            table->positionTypes = MemAlloc(sizeof(mapRandomGroupPositionType_t) * table->num_positions);
            SFileReadFile(file,
                          table->positionTypes,
                          sizeof(mapRandomGroupPositionType_t) * table->num_positions,
                          NULL,
                          NULL);
        }
        SFileReadFile(file, &table->num_units, sizeof(DWORD), NULL, NULL);
        if (table->num_units > 0) {
            table->units = MemAlloc(sizeof(mapRandomUnit_t) * table->num_units);
        }
        FOR_LOOP(j, table->num_units) {
            mapRandomUnit_t *unit = &table->units[j];
            SFileReadFile(file, &unit->chance, sizeof(DWORD), NULL, NULL);
            if (table->num_positions > 0) {
                unit->itemIDs = MemAlloc(sizeof(DWORD) * table->num_positions);
                SFileReadFile(file, unit->itemIDs, sizeof(DWORD) * table->num_positions, NULL, NULL);
            }
        }
    }

    if (info->fileFormat >= 25 && SFileBytesRemaining(file) > 1) {
        SFileReadFile(file, &info->num_randomItems, sizeof(DWORD), NULL, NULL);
        info->randomItems = MemAlloc(sizeof(mapRandomItemTable_t) * info->num_randomItems);
        FOR_LOOP(i, info->num_randomItems) {
            mapRandomItemTable_t *table = &info->randomItems[i];
            SFileReadFile(file, &table->tableNumber, sizeof(DWORD), NULL, NULL);
            SFileReadString(file, &table->tableName);
            SFileReadFile(file, &table->num_sets, sizeof(DWORD), NULL, NULL);
            if (table->num_sets > 0) {
                table->sets = MemAlloc(sizeof(mapRandomItemSet_t) * table->num_sets);
            }
            FOR_LOOP(j, table->num_sets) {
                mapRandomItemSet_t *set = &table->sets[j];
                SFileReadFile(file, &set->num_items, sizeof(DWORD), NULL, NULL);
                if (set->num_items > 0) {
                    set->items = MemAlloc(sizeof(mapRandomItem_t) * set->num_items);
                    SFileReadFile(file, set->items, sizeof(mapRandomItem_t) * set->num_items, NULL, NULL);
                }
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
        FOR_LOOP(j, mapInfo->randomUnits[i].num_units) {
            SAFE_DELETE(mapInfo->randomUnits[i].units[j].itemIDs, MemFree);
        }
        SAFE_DELETE(mapInfo->randomUnits[i].units, MemFree);
        SAFE_DELETE(mapInfo->randomUnits[i].positionTypes, MemFree);
        SAFE_DELETE(mapInfo->randomUnits[i].tableName, MemFree);
    }
    FOR_LOOP(i, mapInfo->num_randomItems) {
        FOR_LOOP(j, mapInfo->randomItems[i].num_sets) {
            SAFE_DELETE(mapInfo->randomItems[i].sets[j].items, MemFree);
        }
        SAFE_DELETE(mapInfo->randomItems[i].sets, MemFree);
        SAFE_DELETE(mapInfo->randomItems[i].tableName, MemFree);
    }
    SAFE_DELETE(mapInfo->mapName, MemFree);
    SAFE_DELETE(mapInfo->mapAuthor, MemFree);
    SAFE_DELETE(mapInfo->mapDescription, MemFree);
    SAFE_DELETE(mapInfo->playersRecommended, MemFree);
    SAFE_DELETE(mapInfo->loadingScreenModel, MemFree);
    SAFE_DELETE(mapInfo->loadingScreenText, MemFree);
    SAFE_DELETE(mapInfo->loadingScreenTitle, MemFree);
    SAFE_DELETE(mapInfo->loadingScreenSubtitle, MemFree);
    SAFE_DELETE(mapInfo->prologueScreenModel, MemFree);
    SAFE_DELETE(mapInfo->prologueScreenText, MemFree);
    SAFE_DELETE(mapInfo->prologueScreenTitle, MemFree);
    SAFE_DELETE(mapInfo->prologueScreenSubtitle, MemFree);
    SAFE_DELETE(mapInfo->soundEnvironment, MemFree);
    SAFE_DELETE(mapInfo->teams, MemFree);
    SAFE_DELETE(mapInfo->upgradeAvailabilities, MemFree);
    SAFE_DELETE(mapInfo->techAvailabilities, MemFree);
    SAFE_DELETE(mapInfo->randomUnits, MemFree);
    SAFE_DELETE(mapInfo->randomItems, MemFree);
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

static BOOL CM_ReadUnitArray(HANDLE file, DWORD *num, void **data, DWORD elem_size);

static BOOL CM_ReadDroppedItemSets(HANDLE file, DWORD *num_sets, droppableItemSet_t **sets) {
    DWORD count;

    if (!num_sets || !sets) {
        return false;
    }
    *num_sets = 0;
    *sets = NULL;
    if (!SFileReadFile(file, &count, sizeof(count), NULL, NULL)) {
        return false;
    }
    if (count > SFileBytesRemaining(file) / sizeof(DWORD)) {
        fprintf(stderr, "CM_ReadUnit: invalid dropped item set count %u\n", (unsigned)count);
        return false;
    }
    *num_sets = count;
    if (count > 0) {
        *sets = MemAlloc(count * sizeof(**sets));
    }
    FOR_LOOP(i, count) {
        DWORD num_items;
        droppableItemSet_t *set = &(*sets)[i];

        if (!CM_ReadUnitArray(file, &num_items, (void **)&set->droppableItems, sizeof(droppableItem_t))) {
            return false;
        }
        set->num_droppableItems = (int)num_items;
    }
    return true;
}

static void CM_ReadDoodads(HANDLE archive) {
    HANDLE file;
    DWORD fileHeader, version, subversion, numDoodads;

    SFileOpenFileEx(archive, "war3map.doo", SFILE_OPEN_FROM_MPQ, &file);
    SFileReadFile(file, &fileHeader, 4, NULL, NULL);
    SFileReadFile(file, &version, 4, NULL, NULL);
    SFileReadFile(file, &subversion, 4, NULL, NULL);
    SFileReadFile(file, &numDoodads, 4, NULL, NULL);

    FOR_LOOP(index, numDoodads) {
        DWORD count;
        LPDOODAD doodad = MemAlloc(sizeof(DOODAD));

        doodad->player = PLAYER_NEUTRAL_PASSIVE;
        doodad->hitPoints = (DWORD)-1;
        doodad->manaPoints = (DWORD)-1;
        doodad->goldAmount = 12500;
        doodad->targetAcquisition = -1.0f;
        SFileReadFile(file, &doodad->doodID, sizeof(DWORD), NULL, NULL);
        SFileReadFile(file, &doodad->variation, sizeof(DWORD), NULL, NULL);
        SFileReadFile(file, &doodad->position, sizeof(VECTOR3), NULL, NULL);
        SFileReadFile(file, &doodad->angle, sizeof(FLOAT), NULL, NULL);
        SFileReadFile(file, &doodad->scale, sizeof(VECTOR3), NULL, NULL);
        SFileReadFile(file, &doodad->flags, sizeof(BYTE), NULL, NULL);
        SFileReadFile(file, &doodad->treeLife, sizeof(BYTE), NULL, NULL);
        if (subversion != 0) {
            SFileReadFile(file, &doodad->droppedItemSetPtr, sizeof(DWORD), NULL, NULL);
            if (!CM_ReadDroppedItemSets(file, &count, &doodad->droppableItemSets)) {
                MemFree(doodad);
                break;
            }
            doodad->num_droppedItemSets = count;
        }
        SFileReadFile(file, &doodad->unitID, sizeof(DWORD), NULL, NULL);
        
        ADD_TO_LIST(doodad, world.doodads);
    }

    SFileCloseFile(file);
}

static BOOL CM_ReadUnitArray(HANDLE file, DWORD *num, void **data, DWORD elem_size) {
    DWORD count;

    if (!num || !data || elem_size == 0) {
        return false;
    }
    *num = 0;
    *data = NULL;
    if (!SFileReadFile(file, &count, sizeof(count), NULL, NULL)) {
        return false;
    }
    if (count > SFileBytesRemaining(file) / elem_size) {
        fprintf(stderr, "CM_ReadUnit: invalid array count %u\n", (unsigned)count);
        return false;
    }
    *num = count;
    if (count > 0) {
        *data = MemAlloc(count * elem_size);
        SFileReadFile(file, *data, count * elem_size, NULL, NULL);
    }
    return true;
}

static BOOL CM_ReadUnit(HANDLE file, struct Doodad *unit, BOOL frozenThrone) {
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
    if (frozenThrone) {
        SFileReadFile(file, &unit->droppedItemSetPtr, sizeof(DWORD), NULL, NULL);
    }
    if (!CM_ReadDroppedItemSets(file, &unit->num_droppedItemSets, &unit->droppableItemSets)) {
        return false;
    }

    SFileReadFile(file, &unit->goldAmount, sizeof(DWORD), NULL, NULL); // (default = 12500)
    SFileReadFile(file, &unit->targetAcquisition, sizeof(FLOAT), NULL, NULL); // (-1 = normal, -2 = camp)

    // war3mapUnits.doo stores only the file-format hero fields here.  The
    // runtime doodadHero_t has extra state, so do not read sizeof(hero).
    SFileReadFile(file, &unit->hero.level, sizeof(DWORD), NULL, NULL);
    SFileReadFile(file, &unit->hero.str, sizeof(DWORD), NULL, NULL);
    SFileReadFile(file, &unit->hero.agi, sizeof(DWORD), NULL, NULL);
    SFileReadFile(file, &unit->hero.intel, sizeof(DWORD), NULL, NULL);
    unit->hero.xp = 0;
    unit->hero.suspend_xp = false;

    if (!CM_ReadUnitArray(file, &unit->num_inventoryItems, (void **)&unit->inventoryItems, sizeof(inventoryItem_t))) {
        return false;
    }
    if (!CM_ReadUnitArray(file, &unit->num_modifiedAbilities, (void **)&unit->modifiedAbilities, sizeof(modifiedAbility_t))) {
        return false;
    }

    SFileReadFile(file, &unit->randomUnitFlag, sizeof(DWORD), NULL, NULL); // "r" (for uDNR units and iDNR items)

    switch ((LONG)unit->randomUnitFlag) {
        case -1:
            break;
        case 0:
            SFileReadFile(file, &unit->levelOfRandomItem, sizeof(DWORD), NULL, NULL);
            break;
        case 1:
            SFileReadFile(file, &unit->randomUnitGroupNumber, sizeof(DWORD), NULL, NULL);
            SFileReadFile(file, &unit->randomUnitPositionNumber, sizeof(DWORD), NULL, NULL);
            break;
        case 2:
            if (!CM_ReadUnitArray(file, &unit->num_diffAvailUnits, (void **)&unit->diffAvailUnits, sizeof(droppableItem_t))) {
                return false;
            }
            break;
        default:
            fprintf(stderr, "CM_ReadUnit: invalid random unit flag %d\n", (int)unit->randomUnitFlag);
            return false;
    }
    SFileReadFile(file, &unit->color, sizeof(COLOR32), NULL, NULL);
    SFileReadFile(file, &unit->waygate, sizeof(DWORD), NULL, NULL);
    SFileReadFile(file, &unit->unitID, sizeof(DWORD), NULL, NULL);
    return true;
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
        if (!CM_ReadUnit(file, doodad, subversion != 0)) {
            MemFree(doodad);
            break;
        }
        ADD_TO_LIST(doodad, world.doodads);
    }

    SFileCloseFile(file);
}

static BOOL CM_ReadWar3MapVertex(HANDLE file, LPWAR3MAPVERTEX vert) {
    WORD water_and_edge;
    BYTE flags;
    BYTE variation;
    BYTE cliff_and_layer;

    if (!file || !vert) {
        return false;
    }
    memset(vert, 0, sizeof(*vert));
    if (!SFileReadFile(file, &vert->accurate_height, sizeof(vert->accurate_height), NULL, NULL)) {
        return false;
    }
    if (!SFileReadFile(file, &water_and_edge, sizeof(water_and_edge), NULL, NULL)) {
        return false;
    }
    if (!SFileReadFile(file, &flags, sizeof(flags), NULL, NULL)) {
        return false;
    }
    if (!SFileReadFile(file, &variation, sizeof(variation), NULL, NULL)) {
        return false;
    }
    if (!SFileReadFile(file, &cliff_and_layer, sizeof(cliff_and_layer), NULL, NULL)) {
        return false;
    }

    vert->waterlevel = water_and_edge & 0x3FFF;
    vert->mapedge = (water_and_edge & 0x4000) != 0;
    vert->ground = flags & 0x0F;
    vert->ramp = (flags & 0x10) != 0;
    vert->blight = (flags & 0x20) != 0;
    vert->water = (flags & 0x40) != 0;
    vert->boundary = (flags & 0x80) != 0;
    vert->cliffVariation = (variation >> 5) & 0x07;
    vert->groundVariation = variation & 0x1F;
    vert->cliff = (cliff_and_layer >> 4) & 0x0F;
    vert->level = cliff_and_layer & 0x0F;
    return true;
}

static void CM_ReadHeightmap(HANDLE archive) {
    world.map = MemAlloc(sizeof(WAR3MAP));
    HANDLE file;
    if (!SFileOpenFileEx(archive, "war3map.w3e", SFILE_OPEN_FROM_MPQ, &file)) {
        return;
    }
    SFileReadFile(file, &world.map->header, 4, NULL, NULL);
    SFileReadFile(file, &world.map->version, 4, NULL, NULL);
    SFileReadFile(file, &world.map->tileset, 1, NULL, NULL);
    SFileReadFile(file, &world.map->custom, 4, NULL, NULL);
    SFileReadArray(file, world.map, grounds, 4, MemAlloc);
    SFileReadArray(file, world.map, cliffs, 4, MemAlloc);
    SFileReadFile(file, &world.map->width, 4, NULL, NULL);
    SFileReadFile(file, &world.map->height, 4, NULL, NULL);
    SFileReadFile(file, &world.map->center, 8, NULL, NULL);
    DWORD const num_vertices = world.map->width * world.map->height;
    int const vertexblocksize = sizeof(WAR3MAPVERTEX) * num_vertices;
    world.map->vertices = MemAlloc(vertexblocksize);
    FOR_LOOP(i, num_vertices) {
        if (!CM_ReadWar3MapVertex(file, (LPWAR3MAPVERTEX)world.map->vertices + i)) {
            break;
        }
    }
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
    CM_WowChooseSpawn(mapFilename);
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
    char const *ptr = ((char const *)world.map->vertices) + index * sizeof(WAR3MAPVERTEX);
    return (LPCWAR3MAPVERTEX)ptr;
}

static FLOAT CM_GetWar3MapVertexHeight(LPCWAR3MAPVERTEX vert) {
    return DECODE_HEIGHT(vert->accurate_height) + vert->level * TILE_SIZE - HEIGHT_COR;
}

FLOAT CM_GetHeightAtPoint(FLOAT sx, FLOAT sy) {
#ifdef WOW
    FLOAT terrain_height;
    if (CM_WowTerrainHeightAtPoint(sx, sy, &terrain_height)) {
        return terrain_height;
    }
    return cm_wow_spawn_position.z;
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
    return (BOX2){
        .min = { -32768.0f, -32768.0f },
        .max = { 32768.0f, 32768.0f },
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
