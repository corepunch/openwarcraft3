#include "common.h"
#include <StormLib.h>

#define DOODAD_FILE_SIZE 42

static struct {
    LPWAR3MAP map;
    struct size2 pathmapSize;
    struct PathMapNode *pathmap;
    LPDOODAD doodads;
    DWORD numDoodads;
} cmodel;

struct PathMapNode const *CM_PathMapNode(LPCWAR3MAP lpTerrain, DWORD x, DWORD y) {
    int const index = x + y * cmodel.pathmapSize.width;
    return &cmodel.pathmap[index];
}

static void CM_ReadDoodads(HANDLE hArchive) {
    HANDLE hFile;
    DWORD dwFileHeader, dwVersion, dwUnknown;

    SFileOpenFileEx(hArchive, "war3map.doo", SFILE_OPEN_FROM_MPQ, &hFile);
    SFileReadFile(hFile, &dwFileHeader, 4, NULL, NULL);
    SFileReadFile(hFile, &dwVersion, 4, NULL, NULL);
    SFileReadFile(hFile, &dwUnknown, 4, NULL, NULL);
    SFileReadFile(hFile, &cmodel.numDoodads, 4, NULL, NULL);
    
    cmodel.doodads = MemAlloc(cmodel.numDoodads * sizeof(struct Doodad));

    FOR_LOOP(index, cmodel.numDoodads) {
        SFileReadFile(hFile, &cmodel.doodads[index], DOODAD_FILE_SIZE, NULL, NULL);
    }

    SFileCloseFile(hFile);
}

static void CM_ReadPathMap(HANDLE hArchive) {
    HANDLE hFile;
    DWORD header, version;
    SFileOpenFileEx(hArchive, "war3map.wpm", SFILE_OPEN_FROM_MPQ, &hFile);
    SFileReadFile(hFile, &header, 4, NULL, NULL);
    SFileReadFile(hFile, &version, 4, NULL, NULL);
    SFileReadFile(hFile, &cmodel.pathmapSize, 8, NULL, NULL);
    int const pathmapblocksize = cmodel.pathmapSize.width * cmodel.pathmapSize.height;
    cmodel.pathmap = MemAlloc(pathmapblocksize);
    SFileReadFile(hFile, cmodel.pathmap, pathmapblocksize, 0, 0);
    SFileCloseFile(hFile);
}

static void CM_ReadHeightmap(HANDLE hArchive) {
    cmodel.map = MemAlloc(sizeof(WAR3MAP));
    HANDLE hFile;
    SFileOpenFileEx(hArchive, "war3map.w3e", SFILE_OPEN_FROM_MPQ, &hFile);
    SFileReadFile(hFile, &cmodel.map->header, 4, NULL, NULL);
    SFileReadFile(hFile, &cmodel.map->version, 4, NULL, NULL);
    SFileReadFile(hFile, &cmodel.map->tileset, 1, NULL, NULL);
    SFileReadFile(hFile, &cmodel.map->custom, 4, NULL, NULL);
    SFileReadArray(hFile, cmodel.map, Grounds, 4, MemAlloc);
    SFileReadArray(hFile, cmodel.map, Cliffs, 4, MemAlloc);
    SFileReadFile(hFile, &cmodel.map->width, 4, NULL, NULL);
    SFileReadFile(hFile, &cmodel.map->height, 4, NULL, NULL);
    SFileReadFile(hFile, &cmodel.map->center, 8, NULL, NULL);
    int const vertexblocksize = MAP_VERTEX_SIZE * cmodel.map->width * cmodel.map->height;
    cmodel.map->vertices = MemAlloc(vertexblocksize);
    SFileReadFile(hFile, cmodel.map->vertices, vertexblocksize, 0, 0);
    SFileCloseFile(hFile);
}

void CM_LoadMap(LPCSTR szMapFilename) {
    HANDLE hMapArchive;
    FS_ExtractFile(szMapFilename, TMP_MAP);
    SFileOpenArchive(TMP_MAP, 0, 0, &hMapArchive);
    CM_ReadPathMap(hMapArchive);
    CM_ReadDoodads(hMapArchive);
    CM_ReadHeightmap(hMapArchive);
    SFileCloseArchive(hMapArchive);
}

VECTOR3 CM_PointIntoHeightmap(LPCVECTOR3 lpPoint) {
    return (VECTOR3) {
        .x = (lpPoint->x - cmodel.map->center.x) / TILESIZE,
        .y = (lpPoint->y - cmodel.map->center.y) / TILESIZE,
        .z = lpPoint->z
    };
}

VECTOR3 CM_PointFromHeightmap(LPCVECTOR3 lpPoint) {
    return (VECTOR3) {
        .x = lpPoint->x * TILESIZE + cmodel.map->center.x,
        .y = lpPoint->y * TILESIZE + cmodel.map->center.y,
        .z = lpPoint->z
    };
}

static float LerpNumber(float a, float b, float t) {
    return a * (1 - t) + b * t;
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

bool CM_IntersectLineWithHeightmap(LPCLINE3 lpLine, LPVECTOR3 lpOutput) {
    LINE3 line = {
        .a = CM_PointIntoHeightmap(&lpLine->a),
        .b = CM_PointIntoHeightmap(&lpLine->b),
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
            if (Line3_intersect_triangle(&line, &tri1, lpOutput)) {
                *lpOutput = CM_PointFromHeightmap(lpOutput);
                return true;
            }
            if (Line3_intersect_triangle(&line, &tri2, lpOutput)) {
                *lpOutput = CM_PointFromHeightmap(lpOutput);
                return true;
            }
        }
    }
    return false;
}

DWORD CM_GetDoodadsArray(LPCDOODAD *lppDoodads) {
    *lppDoodads = cmodel.doodads;
    return cmodel.numDoodads;
}
