#include "common.h"

#include <StormLib.h>

#define MPQ_PATH "/Users/igor/Documents/Warcraft3/war3.mpq"

const LPSTR WarcraftSheets[] = {
    "Units\\unitUI.slk",
    "Units\\AbilityData.slk",
    "Units\\UnitWeapons.slk",
    "Units\\ItemData.slk",
    "Units\\UnitMetaData.slk",
    "Units\\UnitData.slk",
    "Units\\UnitBalance.slk",
    "Units\\UnitAbilities.slk",
    "Units\\UpgradeData.slk",
    "Units\\DestructableData.slk",

    "Doodads\\Doodads.slk",

    "TerrainArt\\Weather.slk",
    "TerrainArt\\CliffTypes.slk",
    "TerrainArt\\Water.slk",
    "TerrainArt\\Terrain.slk",

    "Splats\\UberSplatData.slk",
    "Splats\\LightningData.slk",
    "Splats\\T_SpawnData.slk",
    "Splats\\SplatData.slk",
    "Splats\\T_SplatData.slk",
    "Splats\\SpawnData.slk",

    "UI\\SoundInfo\\MIDISounds.slk",
    "UI\\SoundInfo\\AnimLookups.slk",
    "UI\\SoundInfo\\EnvironmentSounds.slk",
    "UI\\SoundInfo\\PortraitAnims.slk",
    "UI\\SoundInfo\\AbilitySounds.slk",
    "UI\\SoundInfo\\UnitCombatSounds.slk",
    "UI\\SoundInfo\\EAXDefs.slk",
    "UI\\SoundInfo\\DialogSounds.slk",
    "UI\\SoundInfo\\AnimSounds.slk",
    "UI\\SoundInfo\\AmbienceSounds.slk",
    "UI\\SoundInfo\\UISounds.slk",
    "UI\\SoundInfo\\UnitAckSounds.slk",
    
    NULL
};

static struct {
    LPTERRAININFO lpTerrainInfo;
    LPCLIFFINFO lpCliffInfo;
    struct AnimLookup *lpAnimLookups;
    struct SplatData *lpSplatDatas;
    struct SpawnData *lpSpawnDatas;
    struct UnitBalance *lpUnitBalances;
} stats = { NULL };

static HANDLE hArchive;

static void ExtractStarCraft2(void) {
//    SFileOpenArchive("/Users/igor/Documents/SC2Install/Installer Tome 1.MPQ", 0, 0, &hArchive);
//    SFileOpenArchive("/Users/igor/Documents/StarCraft2/Campaigns/Liberty.SC2Campaign/Base.SC2Maps", 0, 0, &hArchive);
//    SFileOpenArchive("/Users/igor/Documents/StarCraft2/Mods/Liberty.SC2Mod/base.SC2Assets", 0, 0, &hArchive);
    SFileOpenArchive(MPQ_PATH, 0, 0, &hArchive);

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

void FS_Init(void) {
//     ExtractStarCraft2();
    SFileOpenArchive(MPQ_PATH, 0, 0, &hArchive);
    // SFileExtractFile(hArchive, "Units\\DestructableData.slk", "/Users/igor/Desktop/DestructableData.slk", 0);
    // SFileExtractFile(hArchive, "Doodads\\Terrain\\WoodBridgeLarge45\\WoodBridgeLarge45.mdx", "/Users/igor/Desktop/WoodBridgeLarge450.mdx", 0);
    SFileExtractFile(hArchive, "Units\\UnitBalance.slk", "/Users/igor/Desktop/UnitBalance.slk", 0);

//     SFILE_FIND_DATA findData;
//     HANDLE handle = SFileFindFirstFile(hArchive, "*", &findData, 0);
//     if (handle) {
//         do {
//             printf("%s\n", findData.cFileName);
//         } while (SFileFindNextFile(handle, &findData));
//         SFileFindClose(handle);
//     }

//    for (LPCSTR* s = sheets; *s; s++) {
//        printf("%s\n", *s);
//        FS_ReadSheet(*s);
//    }

//    void FPrintSheetStructs(LPCSTR *szFileNames);
//    FPrintSheetStructs(WarcraftSheets);
}

void FS_Shutdown(void) {
    SFileCloseArchive(hArchive);
}

HANDLE MemAlloc(long size) {
    HANDLE mem = malloc(size);
//    printf("Alloc (%d) %llx\n", size, mem);
    memset(mem, 0, size);
    return mem;
}

void MemFree(HANDLE mem) {
//    printf("Free %llx\n", mem);
    free(mem);
}

void Com_Quit(void) {
    CL_Shutdown();
    SV_Shutdown();
    FS_Shutdown();
    Sys_Quit();
}
