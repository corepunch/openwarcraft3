#include <stdio.h>
#include <StormLib.h>

#include "../common/common.h"

LPCWAR3MAPVERTEX GetWar3MapVertex(LPCWAR3MAP lpWar3Map, DWORD x, DWORD y) {
    int const index = x + y * lpWar3Map->width;
    char const *ptr = ((char const *)lpWar3Map->vertices) + index * MAP_VERTEX_SIZE;
    return (LPCWAR3MAPVERTEX)ptr;
}

LPWAR3MAP FileReadWar3Map(HANDLE hArchive) {
    LPWAR3MAP lpWar3Map = MemAlloc(sizeof(WAR3MAP));
    HANDLE hFile;
    SFileOpenFileEx(hArchive, "war3map.w3e", SFILE_OPEN_FROM_MPQ, &hFile);
    SFileReadFile(hFile, &lpWar3Map->header, 4, NULL, NULL);
    SFileReadFile(hFile, &lpWar3Map->version, 4, NULL, NULL);
    SFileReadFile(hFile, &lpWar3Map->tileset, 1, NULL, NULL);
    SFileReadFile(hFile, &lpWar3Map->custom, 4, NULL, NULL);
    SFileReadArray(hFile, lpWar3Map, Grounds, 4);
    SFileReadArray(hFile, lpWar3Map, Cliffs, 4);
    SFileReadFile(hFile, &lpWar3Map->width, 4, NULL, NULL);
    SFileReadFile(hFile, &lpWar3Map->height, 4, NULL, NULL);
    SFileReadFile(hFile, &lpWar3Map->center, 8, NULL, NULL);
    int const vertexblocksize = MAP_VERTEX_SIZE * lpWar3Map->width * lpWar3Map->height;
    lpWar3Map->vertices = MemAlloc(vertexblocksize);
    SFileReadFile(hFile, lpWar3Map->vertices, vertexblocksize, 0, 0);
    SFileCloseFile(hFile);

//    for (DWORD x = 0; x < lpWar3Map->width; x++) {
//        for (DWORD y = 0; y < lpWar3Map->height; y++) {
//            printf("%x", GetWar3MapVertex(lpWar3Map, x, y)->accurate_height);
//        }
//        printf("\n");
//    }
    return lpWar3Map;
}


DWORD GetTile(LPCWAR3MAPVERTEX mv, DWORD ground) {
    if (ground == 0)
        return 15;
    return
        (mv[0].ground == ground ? 4 : 0) +
        (mv[1].ground == ground ? 8 : 0) +
        (mv[2].ground == ground ? 1 : 0) +
        (mv[3].ground == ground ? 2 : 0);
}

float GetWar3MapVertexHeight(LPCWAR3MAPVERTEX vert) {
    return DECODE_HEIGHT(vert->accurate_height) + vert->level * TILESIZE - HEIGHT_COR;
}

float GetWar3MapVertexWaterLevel(LPCWAR3MAPVERTEX vert) {
    return DECODE_HEIGHT(vert->waterlevel);
}

void GetTileVertices(DWORD x, DWORD y, LPCWAR3MAP lpWar3Map, LPWAR3MAPVERTEX vertices) {
    vertices[0] = *GetWar3MapVertex(lpWar3Map, x+1, y+1);
    vertices[1] = *GetWar3MapVertex(lpWar3Map, x, y+1);
    vertices[2] = *GetWar3MapVertex(lpWar3Map, x+1, y);
    vertices[3] = *GetWar3MapVertex(lpWar3Map, x, y);
}

DWORD GetTileRamps(LPCWAR3MAPVERTEX vertices) {
    return vertices[0].ramp + vertices[1].ramp + vertices[2].ramp + vertices[3].ramp;
}

DWORD IsTileCliff(LPCWAR3MAPVERTEX vertices) {
    int bIsCliff = 0;
    FOR_LOOP(index, 4) {
        bIsCliff |= vertices[index].level != vertices[0].level;
    }
    return bIsCliff;
}

DWORD IsTileWater(LPCWAR3MAPVERTEX vertices) {
    int bIsWater = 0;
    FOR_LOOP(index, 4) {
        bIsWater |= vertices[index].water;
    }
    FOR_LOOP(index, 4) {
        bIsWater &= (vertices[index].waterlevel & 0x4000) == 0;
    }
    return bIsWater;
}

