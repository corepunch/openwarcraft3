#include "r_war3map.h"
#include "../TerrainArt/Terrain.h"

#define MAX_MAP_LAYERS 16
#define WATER(INDEX) \
MakeColor(color[INDEX], LerpNumber(color[INDEX], 1, 0.25f), LerpNumber(color[INDEX], 1, 0.5f), 1)

LPCTEXTURE g_groundTextures[MAX_MAP_LAYERS] = { NULL };

static VERTEX aVertexBuffer[(SEGMENT_SIZE+1)*(SEGMENT_SIZE+1)*6];
static LPVERTEX currentVertex = NULL;

static void R_MakeTile(LPCWAR3MAP map, DWORD x, DWORD y, DWORD ground, LPCTEXTURE texture) {
    struct War3MapVertex tile[4];
    GetTileVertices(x, y, map, tile);
    int _tile = GetTile(tile, ground);

    if (_tile == 0)
        return;

    if (!GetTileRamps(tile) && IsTileCliff(tile)) {
        return;
    }

    VECTOR2 p[] = {
        { map->center.x + x * TILESIZE, map->center.y + y * TILESIZE },
        { map->center.x + (x + 1) * TILESIZE, map->center.y + y * TILESIZE },
        { map->center.x + (x + 1) * TILESIZE, map->center.y + (y + 1) * TILESIZE },
        { map->center.x + x * TILESIZE, map->center.y + (y + 1) * TILESIZE },
    };

    float const waterlevel[] = {
        GetWar3MapVertexWaterLevel(&tile[3]),
        GetWar3MapVertexWaterLevel(&tile[2]),
        GetWar3MapVertexWaterLevel(&tile[0]),
        GetWar3MapVertexWaterLevel(&tile[1]),
    };

    float const h[] = {
        GetWar3MapVertexHeight(&tile[3]),
        GetWar3MapVertexHeight(&tile[2]),
        GetWar3MapVertexHeight(&tile[0]),
        GetWar3MapVertexHeight(&tile[1]),
    };

    float const color[] = {
        GetTileDepth(waterlevel[0], h[0]),
        GetTileDepth(waterlevel[1], h[1]),
        GetTileDepth(waterlevel[2], h[2]),
        GetTileDepth(waterlevel[3], h[3]),
    };
//
//    VECTOR2 const tilecenter = {
//        map->center.x + (x + 0.5) * TILESIZE,
//        map->center.y + (y + 0.5) * TILESIZE,
//    };
//
//    p[0] = Vector2_lerp(&p[0], &tilecenter, 0.025);
//    p[1] = Vector2_lerp(&p[1], &tilecenter, 0.025);
//    p[2] = Vector2_lerp(&p[2], &tilecenter, 0.025);
//    p[3] = Vector2_lerp(&p[3], &tilecenter, 0.025);

    struct vertex geom[] = {
        {
            .position = { p[0].x, p[0].y, h[0] },
            .texcoord = {0, 0},
            .texcoord2 = GetWar3MapPosition(map, p[0].x, p[0].y),
            .normal = {0,0,1},
            .color = WATER(0),
        },
        {
            .position = { p[1].x, p[1].y, h[1] },
            .texcoord = {1, 0},
            .texcoord2 = GetWar3MapPosition(map, p[1].x, p[1].y),
            .normal = {0,0,1},
            .color = WATER(1),
        },
        {
            .position = { p[2].x, p[2].y, h[2] },
            .texcoord = {1, 1},
            .texcoord2 = GetWar3MapPosition(map, p[2].x, p[2].y),
            .normal = {0,0,1},
            .color = WATER(2),
        },
        {
            .position = { p[0].x, p[0].y, h[0] },
            .texcoord = {0, 0},
            .texcoord2 = GetWar3MapPosition(map, p[0].x, p[0].y),
            .normal = {0,0,1},
            .color = WATER(0),
        },
        {
            .position = { p[2].x, p[2].y, h[2] },
            .texcoord = {1, 1},
            .texcoord2 = GetWar3MapPosition(map, p[2].x, p[2].y),
            .normal = {0,0,1},
            .color = WATER(2),
        },
        {
            .position = { p[3].x, p[3].y, h[3] },
            .texcoord = {0, 1},
            .texcoord2 = GetWar3MapPosition(map, p[3].x, p[3].y),
            .normal = {0,0,1},
            .color = WATER(3),
        },
    };
    
    SetTileUV(_tile, geom, texture);

    memcpy(currentVertex, geom, sizeof(geom));
    currentVertex += sizeof(geom) / sizeof(VERTEX);
}

LPMAPLAYER R_BuildMapSegmentLayer(LPCWAR3MAP map, DWORD sx, DWORD sy, DWORD layer) {
    LPMAPLAYER mapLayer = ri.MemAlloc(sizeof(MAPLAYER));
    PATHSTR zBuffer;
    if (g_groundTextures[layer] == NULL) {
        LPCTERRAIN terrain = FindTerrain(map->grounds[layer]);
        if (!terrain)
            return NULL;
        sprintf(zBuffer, "%s\\%s.blp", terrain->dir, terrain->file);
        g_groundTextures[layer] = R_LoadTexture(zBuffer);
    }
    mapLayer->texture = g_groundTextures[layer];
    mapLayer->type = MAPLAYERTYPE_GROUND;
    currentVertex = aVertexBuffer;
    for (DWORD x = sx * SEGMENT_SIZE; x < (sx + 1) * SEGMENT_SIZE; x++) {
        for (DWORD y = sy * SEGMENT_SIZE; y < (sy + 1) * SEGMENT_SIZE; y++) {
            R_MakeTile(map, x, y, layer, mapLayer->texture);
        }
    }
    mapLayer->num_vertices = (DWORD)(currentVertex - aVertexBuffer);
    mapLayer->buffer = R_MakeVertexArrayObject(aVertexBuffer, mapLayer->num_vertices);
    return mapLayer;
}
