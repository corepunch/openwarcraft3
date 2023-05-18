#include "r_war3map.h"

static VERTEX aVertexBuffer[(SEGMENT_SIZE+1)*(SEGMENT_SIZE+1)*6];
static LPVERTEX lpCurrentVertex = NULL;

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

static void R_MakeWaterTile(LPCWAR3MAP lpMap, DWORD x, DWORD y) {
    struct War3MapVertex tile[4];
    GetTileVertices(x, y, tr.world, tile);
        
    if (!IsTileWater(tile))
        return;
    
    VECTOR2 const pos[] = {
        { tr.world->center.x + x * TILESIZE, tr.world->center.y + y * TILESIZE },
        { tr.world->center.x + (x + 1) * TILESIZE, tr.world->center.y + y * TILESIZE },
        { tr.world->center.x + (x + 1) * TILESIZE, tr.world->center.y + (y + 1) * TILESIZE },
        { tr.world->center.x + x * TILESIZE, tr.world->center.y + (y + 1) * TILESIZE },
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

    struct vertex geom[] = {
        {
            .position = { pos[0].x, pos[0].y, waterlevel[0] },
            .texcoord = { 0, 0 },
            .texcoord2 = {0, 0},
            .normal = { 0, 0, 1 },
            .color = color[0],
        },
        {
            .position = { pos[1].x, pos[1].y, waterlevel[1] },
            .texcoord = { 1, 0 },
            .texcoord2 = {0, 0},
            .normal = { 0, 0, 1 },
            .color = color[1],
        },
        {
            .position = { pos[2].x, pos[2].y, waterlevel[2] },
            .texcoord = { 1, 1 },
            .texcoord2 = {0, 0},
            .normal = { 0, 0, 1 },
            .color = color[2],
        },
        {
            .position = { pos[0].x, pos[0].y, waterlevel[0] },
            .texcoord = { 0, 0 },
            .texcoord2 = {0, 0},
            .normal = { 0, 0, 1 },
            .color = color[0],
        },
        {
            .position = { pos[2].x, pos[2].y, waterlevel[2] },
            .texcoord = { 1, 1 },
            .texcoord2 = {0, 0},
            .normal = { 0, 0, 1 },
            .color = color[2],
        },
        {
            .position = { pos[3].x, pos[3].y, waterlevel[3] },
            .texcoord = { 0, 1 },
            .texcoord2 = {0, 0},
            .normal = { 0, 0, 1 },
            .color = color[3],
        },
    };

    memcpy(lpCurrentVertex, geom, sizeof(geom));
    lpCurrentVertex += sizeof(geom) / sizeof(VERTEX);
}

LPMAPLAYER R_BuildMapSegmentWater(LPCWAR3MAP lpMap, DWORD sx, DWORD sy) {
    LPMAPLAYER lpMapLayer = ri.MemAlloc(sizeof(MAPLAYER));
    lpMapLayer->dwType = MAPLAYERTYPE_WATER;
    lpMapLayer->lpTexture = tr.waterTexture;
    lpCurrentVertex = aVertexBuffer;
    for (DWORD x = sx * SEGMENT_SIZE; x < (sx + 1) * SEGMENT_SIZE; x++) {
        for (DWORD y = sy * SEGMENT_SIZE; y < (sy + 1) * SEGMENT_SIZE; y++) {
            R_MakeWaterTile(lpMap, x, y);
        }
    }
    lpMapLayer->numVertices = (DWORD)(lpCurrentVertex - aVertexBuffer);
    lpMapLayer->lpBuffer = R_MakeVertexArrayObject(aVertexBuffer, lpMapLayer->numVertices);
    return lpMapLayer;
}
