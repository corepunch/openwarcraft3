#include "sc2_map.h"

DWORD SC2_MapObjectClassId(sc2MapObject_t const *object);

bool CM_LoadMapFormat(LPCSTR mapFilename) {
    memset(&world, 0, sizeof(world));
    SC2_MapSetHost(&(sc2MapHost_t){
        .read_file = FS_ReadFile,
        .free_file = FS_FreeFile,
        .mem_alloc = MemAlloc,
        .mem_free = MemFree,
    });
    if (!SC2_MapLoad(mapFilename))
        return false;

    sc2Map_t const *map = SC2_MapCurrent();
    world.map = MemAlloc(sizeof(WAR3MAP));
    memset(world.map, 0, sizeof(WAR3MAP));
    world.map->width = map->width + 1;
    world.map->height = map->height + 1;
    world.map->center = map->origin;
    world.info.mapName = MemAlloc(strlen(map->map_name) + 1);
    strcpy(world.info.mapName, map->map_name);
    world.info.playableArea.width = map->width;
    world.info.playableArea.height = map->height;
    world.info.players[0].used = true;
    world.info.players[0].playerType = kPlayerTypeHuman;
    world.info.players[0].playerRace = kPlayerRaceHuman;
    world.info.players[0].startingPosition = (VECTOR2){ 0.0f, 0.0f };
    CM_SetupPathMap(map->width, map->height, NULL);
    return true;
}

FLOAT CM_GetHeightAtPoint(FLOAT sx, FLOAT sy) {
    return SC2_MapHeightAtPoint(sx, sy);
}

FLOAT CM_GetCameraHeightOffset(void) {
    sc2Map_t const *map = SC2_MapCurrent();
    return map ? map->camera_height_offset : 0.0f;
}

VECTOR2 CM_GetNormalizedMapPosition(FLOAT x, FLOAT y) {
    return SC2_MapNormalizedPosition(x, y);
}

VECTOR2 CM_GetDenormalizedMapPosition(FLOAT x, FLOAT y) {
    return SC2_MapDenormalizedPosition(x, y);
}

BOX2 CM_GetWorldBounds(void) {
    return SC2_MapBounds();
}
