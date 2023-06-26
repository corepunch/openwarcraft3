#include "r_war3map.h"
#include "../mdx/r_mdx.h"

static VERTEX aVertexBuffer[(SEGMENT_SIZE+1)*(SEGMENT_SIZE+1)*64];
static LPVERTEX currentVertex = NULL;

#define SAME_TILE 852063
#define NO_CLIFF MAKEFOURCC('C','L','n','o')

typedef struct {
    DWORD cliff;
    LPCSTR texDir;
    LPCSTR texFile;
    LPCSTR groundTile;
    LPCSTR upperTile;
    LPCSTR rampModelDir;
    LPCSTR cliffModelDir;
} cliffData_t;

struct tCliff {
    DWORD cliffid;
    LPCMODEL model;
    struct tCliff *next;
};

static struct tCliff *g_cliffs = NULL;

// HELPERS

static int TileBaseLevel(LPCWAR3MAPVERTEX tile) {
    DWORD minLevel = tile->level;
    FOR_LOOP(tileIndex, 4) {
        minLevel = MIN(minLevel, tile[tileIndex].level);
    }
    return minLevel;
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

static LPCMODEL R_LoadCliffModel(cliffData_t const *data, char const *ccfg, bool ramp) {
    PATHSTR zBuffer;
    const int cliffid = *(int *)ccfg;
    LPCSTR dir = ramp ? data->rampModelDir : data->cliffModelDir;
    for (struct tCliff *it = g_cliffs; it; it = it->next) {
        if (it->cliffid == cliffid)
            return it->model;
    }
    struct tCliff *cliff = ri.MemAlloc(sizeof(struct tCliff));
    cliff->cliffid = cliffid;
    sprintf(zBuffer, "Doodads\\Terrain\\%s\\%s%s0.mdx", dir, dir, ccfg);
    cliff->model = R_LoadModel(zBuffer);
    ADD_TO_LIST(cliff, g_cliffs);
    return cliff->model;
}

static void R_MakeCliff(LPCWAR3MAP map, DWORD x, DWORD y, cliffData_t const *data) {
    struct War3MapVertex tile[4];
    GetTileVertices(x, y, map, tile);
    int remap[4] = { 3, 1, 0, 2 };

    if (GetTileRamps(tile) || !IsTileCliff(tile) || tile[remap[0]].cliff != data->cliff)
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
    
    FOR_LOOP(gindx, map->num_grounds) {
//        DWORD tile = *(DWORD *)(IS_FOURCC(data->groundTile) ? data->groundTile : data->upperTile);
        if (map->grounds[gindx] == *(DWORD *)data->groundTile) {
            ((LPWAR3MAPVERTEX)GetWar3MapVertex(map, x+1, y+1))->ground = gindx;
            ((LPWAR3MAPVERTEX)GetWar3MapVertex(map, x, y+1))->ground = gindx;
            ((LPWAR3MAPVERTEX)GetWar3MapVertex(map, x+1, y))->ground = gindx;
            ((LPWAR3MAPVERTEX)GetWar3MapVertex(map, x, y))->ground = gindx;
            break;
        }
    }

    LPCMODEL pModel = R_LoadCliffModel(data, cliffcfg, tileramps > 1);
    mdxGeoset_t *pGeoset = pModel->mdx->geosets;

    FOR_LOOP(t, pGeoset->num_triangles) {
        const int i = pGeoset->triangles[t];
        const float fx = pGeoset->vertices[i][0] + (x+1) * TILESIZE;
        const float fy = pGeoset->vertices[i][1] + y * TILESIZE;
        const float fh = GetAccurateHeightAtPoint(fx, fy);
        const float fw = GetAccurateWaterLevelAtPoint(fx, fy);
        const float fz = pGeoset->vertices[i][2] + baselevel * TILESIZE + fh - HEIGHT_COR;
        const float dp = GetTileDepth(fw, fz);
        struct vertex *v = currentVertex + t;
        v->color = MakeColor(dp, LerpNumber(dp, 1, 0.25), LerpNumber(dp, 1, 0.5), 1);
        v->position.x = map->center.x + fx;
        v->position.y = map->center.y + fy;
        v->position.z = fz;
        v->texcoord.x = pGeoset->texcoord[i][0];
        v->texcoord.y = pGeoset->texcoord[i][1];
        v->normal.x = pGeoset->normals[i][0];
        v->normal.y = pGeoset->normals[i][1];
        v->normal.z = pGeoset->normals[i][2];
    }

    currentVertex += pGeoset->num_triangles;
}

LPMAPLAYER R_BuildMapSegmentCliffs(LPCWAR3MAP map, DWORD sx, DWORD sy, DWORD cliff) {
    LPMAPLAYER mapLayer = ri.MemAlloc(sizeof(MAPLAYER));
    PATHSTR zBuffer;
    DWORD cliffID = map->cliffs[cliff];
    if (cliffID == NO_CLIFF)
        return NULL;
    char cliffID_str[5] = { 0 };
    memcpy(cliffID_str, &cliffID, 4);
    cliffData_t data = {
        .cliff = cliff,
        .texDir = ri.FindSheetCell(tr.sheet[SHEET_CLIFF], cliffID_str, "texDir"),
        .texFile = ri.FindSheetCell(tr.sheet[SHEET_CLIFF], cliffID_str, "texFile"),
        .groundTile = ri.FindSheetCell(tr.sheet[SHEET_CLIFF], cliffID_str, "groundTile"),
        .upperTile = ri.FindSheetCell(tr.sheet[SHEET_CLIFF], cliffID_str, "upperTile"),
        .rampModelDir = ri.FindSheetCell(tr.sheet[SHEET_CLIFF], cliffID_str, "rampModelDir"),
        .cliffModelDir = ri.FindSheetCell(tr.sheet[SHEET_CLIFF], cliffID_str, "cliffModelDir"),

    };
    sprintf(zBuffer, "%s\\%c_%s.blp", data.texDir, map->tileset, data.texFile);
    mapLayer->texture = R_LoadTexture(zBuffer);
    if (!mapLayer->texture) {
        sprintf(zBuffer, "%s\\%s.blp", data.texDir, data.texFile);
        mapLayer->texture = R_LoadTexture(zBuffer);
    }
    mapLayer->type = MAPLAYERTYPE_CLIFF;
    currentVertex = aVertexBuffer;
    for (DWORD x = sx * SEGMENT_SIZE; x < (sx + 1) * SEGMENT_SIZE; x++) {
        for (DWORD y = sy * SEGMENT_SIZE; y < (sy + 1) * SEGMENT_SIZE; y++) {
            R_MakeCliff(map, x, y, &data);
        }
    }
    mapLayer->num_vertices = (DWORD)(currentVertex - aVertexBuffer);
    mapLayer->buffer = R_MakeVertexArrayObject(aVertexBuffer, mapLayer->num_vertices);
    return mapLayer;
}
