#include "r_war3map.h"

#define MAX_MAP_LAYERS 16
#define WATER(INDEX) \
MakeColor(color[INDEX], LerpNumber(color[INDEX], 1, 0.25f), LerpNumber(color[INDEX], 1, 0.5f), 1)

LPCTEXTURE g_groundTextures[MAX_MAP_LAYERS] = { NULL };

static VERTEX aVertexBuffer[(SEGMENT_SIZE+1)*(SEGMENT_SIZE+1)*6];
static LPVERTEX lpCurrentVertex = NULL;

static void R_MakeTile(LPCWAR3MAP lpMap, DWORD x, DWORD y, DWORD ground, LPCTEXTURE lpTexture) {
    struct War3MapVertex tile[4];
    GetTileVertices(x, y, lpMap, tile);
    int _tile = GetTile(tile, ground);
    
    if (_tile == 0)
        return;
    
    if (!GetTileRamps(tile) && IsTileCliff(tile)) {
        return;
    }
    
    VECTOR2 p[] = {
        { lpMap->center.x + x * TILESIZE, lpMap->center.y + y * TILESIZE },
        { lpMap->center.x + (x + 1) * TILESIZE, lpMap->center.y + y * TILESIZE },
        { lpMap->center.x + (x + 1) * TILESIZE, lpMap->center.y + (y + 1) * TILESIZE },
        { lpMap->center.x + x * TILESIZE, lpMap->center.y + (y + 1) * TILESIZE },
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
    
    VECTOR2 const tilecenter = {
        lpMap->center.x + (x + 0.5) * TILESIZE,
        lpMap->center.y + (y + 0.5) * TILESIZE,
    };
    
    p[0] = Vector2_lerp(&p[0], &tilecenter, 0.05);
    p[1] = Vector2_lerp(&p[1], &tilecenter, 0.05);
    p[2] = Vector2_lerp(&p[2], &tilecenter, 0.05);
    p[3] = Vector2_lerp(&p[3], &tilecenter, 0.05);

    struct vertex geom[] = {
        {
            .position = { p[0].x, p[0].y, h[0] },
            .texcoord = {0, 0},
            .texcoord2 = GetWar3MapPosition(lpMap, p[0].x, p[0].y),
            .normal = {0,0,1},
            .color = WATER(0),
        },
        {
            .position = { p[1].x, p[1].y, h[1] },
            .texcoord = {1, 0},
            .texcoord2 = GetWar3MapPosition(lpMap, p[1].x, p[1].y),
            .normal = {0,0,1},
            .color = WATER(1),
        },
        {
            .position = { p[2].x, p[2].y, h[2] },
            .texcoord = {1, 1},
            .texcoord2 = GetWar3MapPosition(lpMap, p[2].x, p[2].y),
            .normal = {0,0,1},
            .color = WATER(2),
        },
        {
            .position = { p[0].x, p[0].y, h[0] },
            .texcoord = {0, 0},
            .texcoord2 = GetWar3MapPosition(lpMap, p[0].x, p[0].y),
            .normal = {0,0,1},
            .color = WATER(0),
        },
        {
            .position = { p[2].x, p[2].y, h[2] },
            .texcoord = {1, 1},
            .texcoord2 = GetWar3MapPosition(lpMap, p[2].x, p[2].y),
            .normal = {0,0,1},
            .color = WATER(2),
        },
        {
            .position = { p[3].x, p[3].y, h[3] },
            .texcoord = {0, 1},
            .texcoord2 = GetWar3MapPosition(lpMap, p[3].x, p[3].y),
            .normal = {0,0,1},
            .color = WATER(3),
        },
    };

    SetTileUV(_tile, geom, lpTexture);
    
    memcpy(lpCurrentVertex, geom, sizeof(geom));
    lpCurrentVertex += sizeof(geom) / sizeof(VERTEX);
}

LPMAPLAYER R_BuildMapSegmentLayer(LPCWAR3MAP lpMap, DWORD sx, DWORD sy, DWORD dwLayer) {
    LPMAPLAYER lpMapLayer = ri.MemAlloc(sizeof(MAPLAYER));
    PATHSTR zBuffer;
    if (g_groundTextures[dwLayer] == NULL) {
        LPTERRAININFO lpTerrainInfo = ri.FindTerrainInfo(lpMap->lpGrounds[dwLayer]);
        if (!lpTerrainInfo)
            return NULL;
        sprintf(zBuffer, "%s\\%s.blp", lpTerrainInfo->dir, lpTerrainInfo->file);
        g_groundTextures[dwLayer] = R_LoadTexture(zBuffer);
    }
    lpMapLayer->lpTexture = g_groundTextures[dwLayer];
    lpMapLayer->dwType = MAPLAYERTYPE_GROUND;
    lpCurrentVertex = aVertexBuffer;
    for (DWORD x = sx * SEGMENT_SIZE; x < (sx + 1) * SEGMENT_SIZE; x++) {
        for (DWORD y = sy * SEGMENT_SIZE; y < (sy + 1) * SEGMENT_SIZE; y++) {
            R_MakeTile(lpMap, x, y, dwLayer, lpMapLayer->lpTexture);
        }
    }
    lpMapLayer->numVertices = (DWORD)(lpCurrentVertex - aVertexBuffer);
    lpMapLayer->lpBuffer = R_MakeVertexArrayObject(aVertexBuffer, lpMapLayer->numVertices);
    return lpMapLayer;
}
