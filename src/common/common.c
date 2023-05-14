#include "common.h"

#define MPQ_PATH "/Users/igor/Documents/Warcraft3/war3.mpq"

static struct {
    struct TerrainInfo *lpTerrainInfo;
    struct CliffInfo *lpCliffInfo;
    struct DoodadInfo *lpDoodadInfo;
    struct DestructableInfo *lpDestructableInfo;
} stats = { NULL };

static HANDLE hArchive;

static void ExtractStarCraft2(void) {
//    SFileOpenArchive("/Users/igor/Documents/SC2Install/Installer Tome 1.MPQ", 0, 0, &hArchive);
//    SFileOpenArchive("/Users/igor/Documents/StarCraft2/Campaigns/Liberty.SC2Campaign/Base.SC2Maps", 0, 0, &hArchive);
//    SFileOpenArchive("/Users/igor/Documents/StarCraft2/Mods/Liberty.SC2Mod/base.SC2Assets", 0, 0, &hArchive);
    SFileOpenArchive(MPQ_PATH, 0, 0, &hArchive);
    SFileExtractFile(hArchive, "Doodads\\Terrain\\LordaeronTree\\LordaeronTree7.mdx", "/Users/igor/Desktop/LordaeronTree7.mdx", 0);

    SFILE_FIND_DATA findData;
    HANDLE handle = SFileFindFirstFile(hArchive, "*", &findData, 0);
    if (handle) {
        do {
            printf("%s\n", findData.cFileName);
        } while (SFileFindNextFile(handle, &findData));
        SFileFindClose(handle);
    }
//
//    LoadModel(hArchive, "Assets\\Cliffs\\CliffMade0\\CliffMade0_ABAB_01.m3");
//    SFileExtractFile(hArchive, "Repack-MPQ\\fileset.base#Campaigns#Liberty.SC2Campaign#Base.SC2Maps\\Maps\\Campaign\\TRaynor01.SC2Map\\t3Terrain.xml", "/Users/igor/Desktop/t3Terrain.xml", 0);

    exit(0);
}

HANDLE FS_OpenFile(LPCSTR szFileName) {
    HANDLE hFile = NULL;
    SFileOpenFileEx(hArchive, szFileName, SFILE_OPEN_FROM_MPQ, &hFile);
    return hFile;
}

bool FS_ExtractFile(LPCSTR szToExtract, LPCSTR szExtracted) {
    return SFileExtractFile(hArchive, szToExtract, szExtracted, 0);
}

void FS_Init(void) {
//     ExtractStarCraft2();
    SFileOpenArchive(MPQ_PATH, 0, 0, &hArchive);
    // SFileExtractFile(hArchive, "Units\\DestructableData.slk", "/Users/igor/Desktop/DestructableData.slk", 0);
    // SFileExtractFile(hArchive, "Doodads\\Terrain\\WoodBridgeLarge45\\WoodBridgeLarge45.mdx", "/Users/igor/Desktop/WoodBridgeLarge450.mdx", 0);
    
    // SFILE_FIND_DATA findData;
    // HANDLE handle = SFileFindFirstFile(hArchive, "*", &findData, 0);
    // if (handle) {
    //     do {
    //         printf("%s\n", findData.cFileName);
    //     } while (SFileFindNextFile(handle, &findData));
    //     SFileFindClose(handle);
    // }
    
    struct SheetCell *sheet1 = FS_ReadSheet("TerrainArt\\Terrain.slk");
    struct SheetCell *sheet2 = FS_ReadSheet("TerrainArt\\CliffTypes.slk");
    struct SheetCell *sheet3 = FS_ReadSheet("Doodads\\Doodads.slk");
    struct SheetCell *sheet4 = FS_ReadSheet("Units\\DestructableData.slk");

    stats.lpTerrainInfo = MakeTerrainInfo(sheet1);
    stats.lpCliffInfo = MakeCliffInfo(sheet2);
    stats.lpDoodadInfo = MakeDoodadInfo(sheet3);
    stats.lpDestructableInfo = MakeDestructableInfo(sheet4);
}

void FS_Shutdown(void) {
    SFileCloseArchive(hArchive);
}

void *MemAlloc(long size) {
    void *mem = malloc(size);
//    printf("Alloc (%d) %llx\n", size, mem);
    memset(mem, 0, size);
    return mem;
}

void MemFree(void *mem) {
//    printf("Free %llx\n", mem);
    free(mem);
}

struct TerrainInfo *FindTerrainInfo(int tileID) {
    FOR_EACH_LIST(struct TerrainInfo, info, stats.lpTerrainInfo) {
        if (info->dwTileID == tileID)
            return info;
    }
    return NULL;
}

struct CliffInfo *FindCliffInfo(int cliffID) {
    FOR_EACH_LIST(struct CliffInfo, info, stats.lpCliffInfo) {
        if (info->cliffID == cliffID)
            return info;
    }
    return NULL;
}

struct DoodadInfo *FindDoodadInfo(int doodID) {
    FOR_EACH_LIST(struct DoodadInfo, info, stats.lpDoodadInfo) {
        if (info->doodID == doodID)
            return info;
    }
    return NULL;
}

struct DestructableInfo *FindDestructableInfo(int DestructableID) {
    FOR_EACH_LIST(struct DestructableInfo, info, stats.lpDestructableInfo) {
        if (info->DestructableID == DestructableID)
            return info;
    }
    return NULL;
}

void Com_Quit(void) {
    CL_Shutdown();
    SV_Shutdown();
    FS_Shutdown();
    Sys_Quit();
}
