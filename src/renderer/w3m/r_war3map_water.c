#include "r_war3map.h"

static VERTEX aVertexBuffer[(SEGMENT_SIZE+1)*(SEGMENT_SIZE+1)*6];
static LPVERTEX currentVertex = NULL;

// HELPERS

static struct color32 GetWaterOpacity(float waterlevel, float height) {
    float const opacity = MIN(0.5, (waterlevel - height) / 50.0f);
    return (struct color32) {
        .r = 255,
        .g = 255,
        .b = 255,
        .a = MAX(0, opacity) * 255
    };
}

// FUNCTIONS

static void R_MakeWaterTile(LPCWAR3MAP map, DWORD x, DWORD y) {
    struct War3MapVertex tile[4];
    GetTileVertices(x, y, tr.world, tile);

    if (!IsTileWater(tile))
        return;

    VECTOR2 const pos[] = {
        { tr.world->center.x + x * TILE_SIZE, tr.world->center.y + y * TILE_SIZE },
        { tr.world->center.x + (x + 1) * TILE_SIZE, tr.world->center.y + y * TILE_SIZE },
        { tr.world->center.x + (x + 1) * TILE_SIZE, tr.world->center.y + (y + 1) * TILE_SIZE },
        { tr.world->center.x + x * TILE_SIZE, tr.world->center.y + (y + 1) * TILE_SIZE },
    };

    float const waterlevel[] = {
        GetWar3MapVertexWaterLevel(&tile[3]),
        GetWar3MapVertexWaterLevel(&tile[2]),
        GetWar3MapVertexWaterLevel(&tile[0]),
        GetWar3MapVertexWaterLevel(&tile[1]),
    };

    float const height[] = {
        GetWar3MapVertexHeight(&tile[3]),
        GetWar3MapVertexHeight(&tile[2]),
        GetWar3MapVertexHeight(&tile[0]),
        GetWar3MapVertexHeight(&tile[1]),
    };

    struct color32 const color[] = {
        GetWaterOpacity(waterlevel[0], height[0]),
        GetWaterOpacity(waterlevel[1], height[1]),
        GetWaterOpacity(waterlevel[2], height[2]),
        GetWaterOpacity(waterlevel[3], height[3]),
    };
    
#define WATER_SCALE(x,y) (((x%3)+y)/3.0)
    
    VECTOR2 const tc[] = {
        { WATER_SCALE(x, 0),  WATER_SCALE(y, 0) },
        { WATER_SCALE(x, 1),  WATER_SCALE(y, 0) },
        { WATER_SCALE(x, 1),  WATER_SCALE(y, 1) },
        { WATER_SCALE(x, 0),  WATER_SCALE(y, 1) },
    };

    struct vertex geom[] = {
        {
            .position = { pos[0].x, pos[0].y, waterlevel[0] },
            .texcoord = tc[0],
            .normal = { 0, 0, 1 },
            .color = color[0],
        },
        {
            .position = { pos[1].x, pos[1].y, waterlevel[1] },
            .texcoord = tc[1],
            .normal = { 0, 0, 1 },
            .color = color[1],
        },
        {
            .position = { pos[2].x, pos[2].y, waterlevel[2] },
            .texcoord = tc[2],
            .normal = { 0, 0, 1 },
            .color = color[2],
        },
        {
            .position = { pos[0].x, pos[0].y, waterlevel[0] },
            .texcoord = tc[0],
            .normal = { 0, 0, 1 },
            .color = color[0],
        },
        {
            .position = { pos[2].x, pos[2].y, waterlevel[2] },
            .texcoord = tc[2],
            .normal = { 0, 0, 1 },
            .color = color[2],
        },
        {
            .position = { pos[3].x, pos[3].y, waterlevel[3] },
            .texcoord = tc[3],
            .normal = { 0, 0, 1 },
            .color = color[3],
        },
    };

    memcpy(currentVertex, geom, sizeof(geom));
    currentVertex += sizeof(geom) / sizeof(VERTEX);
}

LPMAPLAYER R_BuildMapSegmentWater(LPCWAR3MAP map, DWORD sx, DWORD sy) {
    LPMAPLAYER mapLayer = ri.MemAlloc(sizeof(MAPLAYER));
    mapLayer->type = MAPLAYERTYPE_WATER;
    mapLayer->texture = tr.texture[TEX_WATER];
    currentVertex = aVertexBuffer;
    for (DWORD x = sx * SEGMENT_SIZE; x < (sx + 1) * SEGMENT_SIZE; x++) {
        for (DWORD y = sy * SEGMENT_SIZE; y < (sy + 1) * SEGMENT_SIZE; y++) {
            R_MakeWaterTile(map, x, y);
        }
    }
    mapLayer->num_vertices = (DWORD)(currentVertex - aVertexBuffer);
    mapLayer->buffer = R_MakeVertexArrayObject(aVertexBuffer, mapLayer->num_vertices);
    return mapLayer;
}
