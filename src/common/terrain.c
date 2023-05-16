#include <stdio.h>
#include <StormLib.h>

#include "../common/common.h"

struct TerrainVertex const *GetTerrainVertex(struct terrain const *heightmap, int x, int y) {
    int const index = x + y * heightmap->size.width;
    char const *ptr = ((char const *)heightmap->vertices) + index * MAP_VERTEX_SIZE;
    return (struct TerrainVertex const *)ptr;
}

struct terrain *FileReadTerrain(HANDLE hArchive) {
    struct terrain *lpTerrain = MemAlloc(sizeof(struct terrain));
    HANDLE hFile;
    SFileOpenFileEx(hArchive, "war3map.w3e", SFILE_OPEN_FROM_MPQ, &hFile);
    SFileReadFile(hFile, &lpTerrain->header, 4, NULL, NULL);
    SFileReadFile(hFile, &lpTerrain->version, 4, NULL, NULL);
    SFileReadFile(hFile, &lpTerrain->tileset, 1, NULL, NULL);
    SFileReadFile(hFile, &lpTerrain->custom, 4, NULL, NULL);
    SFileReadArray(hFile, lpTerrain, Grounds, 4);
    SFileReadArray(hFile, lpTerrain, Cliffs, 4);
    SFileReadFile(hFile, &lpTerrain->size, 8, NULL, NULL);
    SFileReadFile(hFile, &lpTerrain->center, 8, NULL, NULL);
    int const vertexblocksize = MAP_VERTEX_SIZE * lpTerrain->size.width * lpTerrain->size.height;
    lpTerrain->vertices = MemAlloc(vertexblocksize);
    SFileReadFile(hFile, lpTerrain->vertices, vertexblocksize, 0, 0);
    SFileCloseFile(hFile);

//    for (int x = 0; x < lpTerrain->size.width; x++) {
//        for (int y = 0; y < lpTerrain->size.height; y++) {
//            printf("%x", GetTerrainVertex(lpTerrain, x, y)->accurate_height);
//        }
//        printf("\n");
//    }
    return lpTerrain;
}


int GetTile(struct TerrainVertex const *mv, int ground) {
    if (ground == 0)
        return 15;
    return
        (mv[0].ground == ground ? 4 : 0) +
        (mv[1].ground == ground ? 8 : 0) +
        (mv[2].ground == ground ? 1 : 0) +
        (mv[3].ground == ground ? 2 : 0);
}

float GetTerrainVertexHeight(struct TerrainVertex const *vert) {
    return DECODE_HEIGHT(vert->accurate_height) + vert->level * TILESIZE - HEIGHT_COR;
}

float GetTerrainVertexWaterLevel(struct TerrainVertex const *vert) {
    return DECODE_HEIGHT(vert->waterlevel);
}

void GetTileVertices(int x, int y, struct terrain const *heightmap, struct TerrainVertex *vertices) {
    vertices[0] = *GetTerrainVertex(heightmap, x+1, y+1);
    vertices[1] = *GetTerrainVertex(heightmap, x, y+1);
    vertices[2] = *GetTerrainVertex(heightmap, x+1, y);
    vertices[3] = *GetTerrainVertex(heightmap, x, y);
}

int GetTileRamps(struct TerrainVertex const *vertices) {
    return vertices[0].ramp + vertices[1].ramp + vertices[2].ramp + vertices[3].ramp;
}

int IsTileCliff(struct TerrainVertex const *vertices) {
    int bIsCliff = 0;
    FOR_LOOP(index, 4) {
        bIsCliff |= vertices[index].level != vertices[0].level;
    }
    return bIsCliff;
}

int IsTileWater(struct TerrainVertex const *vertices) {
    int bIsWater = 0;
    FOR_LOOP(index, 4) {
        bIsWater |= vertices[index].water;
    }
    FOR_LOOP(index, 4) {
        bIsWater &= (vertices[index].waterlevel & 0x4000) == 0;
    }
    return bIsWater;
}

