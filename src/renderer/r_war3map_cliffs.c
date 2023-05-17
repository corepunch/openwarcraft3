#include "r_war3map.h"
#include "r_mdx.h"

static VERTEX aVertexBuffer[(SEGMENT_SIZE+1)*(SEGMENT_SIZE+1)*64];
static LPVERTEX lpCurrentVertex = NULL;

struct tCliff {
    DWORD cliffid;
    LPCMODEL model;
    struct tCliff *lpNext;
};

static struct tCliff *g_cliffs = NULL;

// HELPERS

static int TileBaseLevel(LPCWAR3MAPVERTEX lpTile) {
    DWORD dwMinLevel = lpTile->level;
    FOR_LOOP(dwTileIndex, 4) {
        dwMinLevel = MIN(dwMinLevel, lpTile[dwTileIndex].level);
    }
    return dwMinLevel;
}

static float GetAccurateHeightAtPoint(float sx, float sy) {
    float x = sx / TILESIZE;
    float y = sy / TILESIZE;
    float fx = floorf(x);
    float fy = floorf(y);
    float a = GetWar3MapVertex(tr.world, fx, fy)->accurate_height;
    float b = GetWar3MapVertex(tr.world, fx + 1, fy)->accurate_height;
    float c = GetWar3MapVertex(tr.world, fx, fy + 1)->accurate_height;
    float d = GetWar3MapVertex(tr.world, fx + 1, fy + 1)->accurate_height;
    float ab = LerpNumber(a, b, x - fx);
    float cd = LerpNumber(c, d, x - fx);
    return DECODE_HEIGHT(LerpNumber(ab, cd, y - fy));
}

static float GetAccurateWaterLevelAtPoint(float sx, float sy) {
    float x = sx / TILESIZE;
    float y = sy / TILESIZE;
    float fx = floorf(x);
    float fy = floorf(y);
    float a = GetWar3MapVertex(tr.world, fx, fy)->waterlevel;
    float b = GetWar3MapVertex(tr.world, fx + 1, fy)->waterlevel;
    float c = GetWar3MapVertex(tr.world, fx, fy + 1)->waterlevel;
    float d = GetWar3MapVertex(tr.world, fx + 1, fy + 1)->waterlevel;
    float ab = LerpNumber(a, b, x - fx);
    float cd = LerpNumber(c, d, x - fx);
    return DECODE_HEIGHT(LerpNumber(ab, cd, y - fy));
}

// FUNCTIONS

static LPCMODEL R_LoadCliffModel(LPCCLIFFINFO lpCliffInfo, char const *ccfg, bool ramp) {
    PATHSTR zBuffer;
    const int cliffid = *(int *)ccfg;
    LPCSTR dir = ramp ? lpCliffInfo->rampModelDir : lpCliffInfo->cliffModelDir;
    for (struct tCliff *it = g_cliffs; it; it = it->lpNext) {
        if (it->cliffid == cliffid)
            return it->model;
    }
    struct tCliff *cliff = MemAlloc(sizeof(struct tCliff));
    cliff->cliffid = cliffid;
    sprintf(zBuffer, "Doodads\\Terrain\\%s\\%s%s0.mdx", dir, dir, ccfg);
    cliff->model = R_LoadModel(zBuffer);
    cliff->lpNext = g_cliffs;
    g_cliffs = cliff;
    return cliff->model;
}

static void R_MakeCliff(LPCWAR3MAP lpMap, DWORD x, DWORD y, DWORD dwCliff, LPCCLIFFINFO lpCliffInfo) {
    struct War3MapVertex tile[4];
    GetTileVertices(x, y, lpMap, tile);
    int remap[4] = { 3, 1, 0, 2 };
    
    if (GetTileRamps(tile) || !IsTileCliff(tile) || tile[remap[0]].cliff != dwCliff)
        return;
    
    char cliffcfg[5] = { 0 };
    int const tileramps = GetTileRamps(tile);
    int const baselevel = TileBaseLevel(tile);
    
    FOR_LOOP(index, 4) {
        LPCWAR3MAPVERTEX vert = &tile[remap[index]];
        int const diff = vert->level - baselevel;
        if (diff == 0) {
            cliffcfg[index] = (tileramps > 1 && vert->ramp) ? 'L' : 'A';
        } else if (diff == 1) {
            cliffcfg[index] = (tileramps > 1 && vert->ramp) ? 'H' : 'B';
        } else {
            cliffcfg[index] = (tileramps > 1 && vert->ramp) ? 'X' : 'C';
        }
    }
    
    FOR_LOOP(gindx, lpMap->numGrounds) {
        if (lpMap->lpGrounds[gindx] == lpCliffInfo->groundTile) {
            ((LPWAR3MAPVERTEX)GetWar3MapVertex(lpMap, x+1, y+1))->ground = gindx;
            ((LPWAR3MAPVERTEX)GetWar3MapVertex(lpMap, x, y+1))->ground = gindx;
            ((LPWAR3MAPVERTEX)GetWar3MapVertex(lpMap, x+1, y))->ground = gindx;
            ((LPWAR3MAPVERTEX)GetWar3MapVertex(lpMap, x, y))->ground = gindx;
            break;
        }
    }
    
    LPCMODEL pModel = R_LoadCliffModel(lpCliffInfo, cliffcfg, tileramps > 1);
    LPMODELGEOSET pGeoset = pModel->lpGeosets;
    
    FOR_LOOP(t, pGeoset->numTriangles) {
        const int i = pGeoset->lpTriangles[t];
        const float fx = pGeoset->lpVertices[i].x + (x+1) * TILESIZE;
        const float fy = pGeoset->lpVertices[i].y + y * TILESIZE;
        const float fh = GetAccurateHeightAtPoint(fx, fy);
        const float fw = GetAccurateWaterLevelAtPoint(fx, fy);
        const float fz = pGeoset->lpVertices[i].z + baselevel * TILESIZE + fh - HEIGHT_COR;
        const float dp = GetTileDepth(fw, fz);
        struct vertex *v = lpCurrentVertex + t;
        v->color = MakeColor(dp, LerpNumber(dp, 1, 0.25), LerpNumber(dp, 1, 0.5), 1);
        v->position.x = lpMap->center.x + fx;
        v->position.y = lpMap->center.y + fy;
        v->position.z = fz;
        v->texcoord = pGeoset->lpTexcoord[i];
        v->texcoord2 = GetWar3MapPosition(lpMap, lpMap->center.x + fx, lpMap->center.y + fy);
        v->normal = pGeoset->lpNormals[i];
    }
    
    lpCurrentVertex += pGeoset->numTriangles;
}

LPMAPLAYER R_BuildMapSegmentCliffs(LPCWAR3MAP lpMap, DWORD sx, DWORD sy, DWORD dwCliff) {
    LPMAPLAYER lpMapLayer = ri.MemAlloc(sizeof(MAPLAYER));
    PATHSTR zBuffer;
    LPCLIFFINFO lpCliffInfo = FindCliffInfo(lpMap->lpCliffs[dwCliff]);
    if (!lpCliffInfo)
        return NULL;
    sprintf(zBuffer, "%s\\%s.blp", lpCliffInfo->texDir, lpCliffInfo->texFile);
    lpMapLayer->lpTexture = R_LoadTexture(zBuffer);
    lpMapLayer->dwType = MAPLAYERTYPE_CLIFF;
    lpCurrentVertex = aVertexBuffer;
    for (DWORD x = sx * SEGMENT_SIZE; x < (sx + 1) * SEGMENT_SIZE; x++) {
        for (DWORD y = sy * SEGMENT_SIZE; y < (sy + 1) * SEGMENT_SIZE; y++) {
            R_MakeCliff(lpMap, x, y, dwCliff, lpCliffInfo);
        }
    }
    lpMapLayer->numVertices = (DWORD)(lpCurrentVertex - aVertexBuffer);
    lpMapLayer->lpBuffer = R_MakeVertexArrayObject(aVertexBuffer, lpMapLayer->numVertices);
    return lpMapLayer;
}
