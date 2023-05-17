#include "common.h"

#define MPQ_PATH "/Users/igor/Documents/Warcraft3/war3.mpq"

static struct {
    struct UnitData *lpUnitData;
    LPTERRAININFO lpTerrainInfo;
    LPCLIFFINFO lpCliffInfo;
    struct DoodadInfo *lpDoodadInfo;
    struct DestructableData *lpDestructableData;
} stats = { NULL };

static HANDLE hArchive;

static void ExtractStarCraft2(void) {
//    SFileOpenArchive("/Users/igor/Documents/SC2Install/Installer Tome 1.MPQ", 0, 0, &hArchive);
//    SFileOpenArchive("/Users/igor/Documents/StarCraft2/Campaigns/Liberty.SC2Campaign/Base.SC2Maps", 0, 0, &hArchive);
//    SFileOpenArchive("/Users/igor/Documents/StarCraft2/Mods/Liberty.SC2Mod/base.SC2Assets", 0, 0, &hArchive);
    SFileOpenArchive(MPQ_PATH, 0, 0, &hArchive);
//    SFileExtractFile(hArchive, "Units\\Orc\\Peon\\Peon.mdx", "/Users/igor/Desktop/Peon.mdx", 0);

//    SFILE_FIND_DATA findData;
//    HANDLE handle = SFileFindFirstFile(hArchive, "*", &findData, 0);
//    if (handle) {
//        do {
//            printf("%s\n", findData.cFileName);
//        } while (SFileFindNextFile(handle, &findData));
//        SFileFindClose(handle);
//    }
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

struct SheetLayout cliff_info[] = {
    { "cliffID", ST_ID, FOFS(CliffInfo, cliffID) },
    { "cliffModelDir", ST_STRING, FOFS(CliffInfo, cliffModelDir) },
    { "rampModelDir", ST_STRING, FOFS(CliffInfo, rampModelDir) },
    { "texDir", ST_STRING, FOFS(CliffInfo, texDir) },
    { "texFile", ST_STRING, FOFS(CliffInfo, texFile) },
    { "name", ST_STRING, FOFS(CliffInfo, name) },
    { "groundTile", ST_INT, FOFS(CliffInfo, groundTile) },
    { "upperTile", ST_INT, FOFS(CliffInfo, upperTile) },
    { NULL }
};

struct SheetLayout terrain_info[] = {
    { "tileID", ST_ID, FOFS(TerrainInfo, tileID) },
    { "dir", ST_STRING, FOFS(TerrainInfo, dir) },
    { "file", ST_STRING, FOFS(TerrainInfo, file) },
    { NULL }
};

void FixCliffInfo(LPCLIFFINFO cliffInfo) {
    int const SAME_TILE = 852063;
    if (cliffInfo->upperTile == SAME_TILE) {
        cliffInfo->upperTile = cliffInfo->groundTile;
    }
    if (cliffInfo->groundTile == SAME_TILE) {
        cliffInfo->groundTile = cliffInfo->upperTile;
    }
    if (cliffInfo->lpNext) {
        FixCliffInfo(cliffInfo->lpNext);
    }
}

void FS_Init(void) {
//     ExtractStarCraft2();
    SFileOpenArchive(MPQ_PATH, 0, 0, &hArchive);
    // SFileExtractFile(hArchive, "Units\\DestructableData.slk", "/Users/igor/Desktop/DestructableData.slk", 0);
    // SFileExtractFile(hArchive, "Doodads\\Terrain\\WoodBridgeLarge45\\WoodBridgeLarge45.mdx", "/Users/igor/Desktop/WoodBridgeLarge450.mdx", 0);
    
//     SFILE_FIND_DATA findData;
//     HANDLE handle = SFileFindFirstFile(hArchive, "*", &findData, 0);
//     if (handle) {
//         do {
//             printf("%s\n", findData.cFileName);
//         } while (SFileFindNextFile(handle, &findData));
//         SFileFindClose(handle);
//     }
//
//    const char *sheets[] = {
//        "Units\\unitUI.slk",
//        "Splats\\LightningData.slk",
//        "Units\\AbilityData.slk",
//        "Units\\UnitWeapons.slk",
//        "UI\\SoundInfo\\MIDISounds.slk",
//        "Splats\\T_SpawnData.slk",
//        "Splats\\SplatData.slk",
//        "Splats\\T_SplatData.slk",
//        "Splats\\SpawnData.slk",
//        "TerrainArt\\Weather.slk",
//        "Units\\ItemData.slk",
//        "UI\\SoundInfo\\AnimLookups.slk",
//        "TerrainArt\\CliffTypes.slk",
//        "Doodads\\Doodads.slk",
//        "Units\\UnitMetaData.slk",
//        "Splats\\UberSplatData.slk",
//        "UI\\SoundInfo\\EnvironmentSounds.slk",
//        "UI\\SoundInfo\\PortraitAnims.slk",
//        "Units\\UnitData.slk",
//        "UI\\SoundInfo\\AbilitySounds.slk",
//        "UI\\SoundInfo\\UnitCombatSounds.slk",
//        "Units\\UnitBalance.slk",
//        "Units\\UnitAbilities.slk",
//        "Units\\UpgradeData.slk",
//        "TerrainArt\\Water.slk",
//        "TerrainArt\\Terrain.slk",
//        "UI\\SoundInfo\\EAXDefs.slk",
//        "UI\\SoundInfo\\DialogSounds.slk",
//        "Units\\DestructableData.slk",
//        "UI\\SoundInfo\\AnimSounds.slk",
//        "UI\\SoundInfo\\AmbienceSounds.slk",
//        "UI\\SoundInfo\\UISounds.slk",
//        "UI\\SoundInfo\\UnitAckSounds.slk",
//        NULL
//    };

//    for (const char** s = sheets; *s; s++) {
//        printf("%s\n", *s);
//        FS_ReadSheet(*s);
//    }
//    FS_ReadSheet("Units\\unitUI.slk");

    stats.lpTerrainInfo = FS_ParseSheet("TerrainArt\\Terrain.slk", terrain_info, sizeof(struct TerrainInfo), FOFS(TerrainInfo, lpNext));
    stats.lpCliffInfo = FS_ParseSheet("TerrainArt\\CliffTypes.slk", cliff_info, sizeof(struct CliffInfo), FOFS(CliffInfo, lpNext));
    FixCliffInfo(stats.lpCliffInfo);
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

LPTERRAININFO FindTerrainInfo(int tileID) {
    FOR_EACH_LIST(struct TerrainInfo, info, stats.lpTerrainInfo) {
        if (info->tileID == tileID)
            return info;
    }
    return NULL;
}

LPCLIFFINFO FindCliffInfo(int cliffID) {
    FOR_EACH_LIST(struct CliffInfo, info, stats.lpCliffInfo) {
        if (info->cliffID == cliffID)
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
