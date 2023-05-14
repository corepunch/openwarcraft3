#include "server.h"

static struct size2 sv_pathmapSize;
static struct PathMapNode *sv_pathmap;

struct PathMapNode const *SV_PathMapNode(struct Terrain const *heightmap, int x, int y) {
    int const index = x + y * sv_pathmapSize.width;
    return &sv_pathmap[index];
}

static void SV_ReadDoodads(HANDLE hArchive) {
    HANDLE hFile;
    DWORD dwFileHeader, dwVersion, dwUnknown, dwNumDoodads;

    SFileOpenFileEx(hArchive, "war3map.doo", SFILE_OPEN_FROM_MPQ, &hFile);
    SFileReadFile(hFile, &dwFileHeader, 4, NULL, NULL);
    SFileReadFile(hFile, &dwVersion, 4, NULL, NULL);
    SFileReadFile(hFile, &dwUnknown, 4, NULL, NULL);
    SFileReadFile(hFile, &dwNumDoodads, 4, NULL, NULL);
    
    struct Doodad *lpDoodads = MemAlloc(dwNumDoodads * DOODAD_SIZE);
    
    SFileReadFile(hFile, lpDoodads, dwNumDoodads * DOODAD_SIZE, NULL, NULL);
    SFileCloseFile(hFile);
    
    ge->SpawnEntities(lpDoodads, dwNumDoodads);
    
    MemFree(lpDoodads);
}

static void SV_ReadPathMap(HANDLE hArchive) {
    HANDLE hFile;
    DWORD header, version;
    SFileOpenFileEx(hArchive, "war3map.wpm", SFILE_OPEN_FROM_MPQ, &hFile);
    SFileReadFile(hFile, &header, 4, NULL, NULL);
    SFileReadFile(hFile, &version, 4, NULL, NULL);
    SFileReadFile(hFile, &sv_pathmapSize, 8, NULL, NULL);
    int const pathmapblocksize = sv_pathmapSize.width * sv_pathmapSize.height;
    sv_pathmap = MemAlloc(pathmapblocksize);
    SFileReadFile(hFile, sv_pathmap, pathmapblocksize, 0, 0);
    SFileCloseFile(hFile);
}

void SV_Map(LPCSTR szMapFilename) {
    HANDLE hMapArchive;
    memset(&sv, 0, sizeof(struct server));
    strcpy(sv.configstrings[CS_MODELS+1], szMapFilename);
    FS_ExtractFile(szMapFilename, TMP_MAP);
    SFileOpenArchive(TMP_MAP, 0, 0, &hMapArchive);
    SV_ReadPathMap(hMapArchive);
    SV_ReadDoodads(hMapArchive);
    SFileCloseArchive(hMapArchive);
}
