#include <stdio.h>
#include <StormLib.h>

#include "../common/common.h"

LPWAR3MAP FileReadWar3Map(HANDLE hArchive) {
    LPWAR3MAP lpWar3Map = MemAlloc(sizeof(WAR3MAP));
    HANDLE hFile;
    SFileOpenFileEx(hArchive, "war3map.w3e", SFILE_OPEN_FROM_MPQ, &hFile);
    SFileReadFile(hFile, &lpWar3Map->header, 4, NULL, NULL);
    SFileReadFile(hFile, &lpWar3Map->version, 4, NULL, NULL);
    SFileReadFile(hFile, &lpWar3Map->tileset, 1, NULL, NULL);
    SFileReadFile(hFile, &lpWar3Map->custom, 4, NULL, NULL);
    SFileReadArray(hFile, lpWar3Map, Grounds, 4, MemAlloc);
    SFileReadArray(hFile, lpWar3Map, Cliffs, 4, MemAlloc);
    SFileReadFile(hFile, &lpWar3Map->width, 4, NULL, NULL);
    SFileReadFile(hFile, &lpWar3Map->height, 4, NULL, NULL);
    SFileReadFile(hFile, &lpWar3Map->center, 8, NULL, NULL);
    int const vertexblocksize = MAP_VERTEX_SIZE * lpWar3Map->width * lpWar3Map->height;
    lpWar3Map->vertices = MemAlloc(vertexblocksize);
    SFileReadFile(hFile, lpWar3Map->vertices, vertexblocksize, 0, 0);
    SFileCloseFile(hFile);
    return lpWar3Map;
}


