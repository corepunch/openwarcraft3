#include "common.h"
#include "tables.h"

#include <StormLib.h>

#define MPQ_PATH "/Users/igor/Documents/Warcraft3/war3.mpq"

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

struct SheetLayout CliffInfos[] = {
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

struct SheetLayout TerrainInfos[] = {
    { "tileID", ST_ID, FOFS(TerrainInfo, tileID) },
    { "dir", ST_STRING, FOFS(TerrainInfo, dir) },
    { "file", ST_STRING, FOFS(TerrainInfo, file) },
    { NULL }
};

struct SheetLayout AnimLookups[] = {
    { "AnimSoundEvent", ST_ID, FOFS(AnimLookup, AnimSoundEvent) },
    { "SoundLabel", ST_STRING, FOFS(AnimLookup, SoundLabel) },
    { NULL }
};

struct SheetLayout SpawnDatas[] = {
    { "Name", ST_ID, FOFS(SpawnData, Name) },
    { "Model", ST_STRING, FOFS(SpawnData, Model) },
};

struct SheetLayout SplatDatas[] = {
    { "Name", ST_ID, FOFS(SplatData, Name) },
    { "comment", ST_STRING, FOFS(SplatData, comment) },
    { "Dir", ST_STRING, FOFS(SplatData, Dir) },
    { "file", ST_INT, FOFS(SplatData, file) },
    { "Rows", ST_INT, FOFS(SplatData, Rows) },
    { "Columns", ST_INT, FOFS(SplatData, Columns) },
    { "BlendMode", ST_INT, FOFS(SplatData, BlendMode) },
    { "Scale", ST_INT, FOFS(SplatData, Scale) },
    { "Lifespan", ST_INT, FOFS(SplatData, Lifespan) },
    { "Decay", ST_INT, FOFS(SplatData, Decay) },
    { "UVLifespanStart", ST_INT, FOFS(SplatData, UVLifespanStart) },
    { "UVLifespanEnd", ST_INT, FOFS(SplatData, UVLifespanEnd) },
    { "LifespanRepeat", ST_INT, FOFS(SplatData, LifespanRepeat) },
    { "UVDecayStart", ST_INT, FOFS(SplatData, UVDecayStart) },
    { "UVDecayEnd", ST_INT, FOFS(SplatData, UVDecayEnd) },
    { "DecayRepeat", ST_INT, FOFS(SplatData, DecayRepeat) },
    { "Water", ST_STRING, FOFS(SplatData, Water) },
    { "Sound", ST_STRING, FOFS(SplatData, Sound) },
    { NULL }
};

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

//    const LPSTR sheets[] = {
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

//    const LPSTR sheets[] = {
////        "Units\\UnitAbilities.slk",
//        "Units\\UnitBalance.slk",
////        "Units\\UnitData.slk",
////        "Units\\UnitWeapons.slk",
//        NULL
//    };
//
//    for (LPCSTR* s = sheets; *s; s++) {
//        printf("%s\n", *s);
//        FS_ReadSheet(*s);
//        printf("\n", *s);
//    }
    
    extern struct SheetLayout UnitBalances[];
    
    stats.lpSpawnDatas = FS_ParseSheet("Splats\\T_SpawnData.slk", SpawnDatas, sizeof(struct SpawnData));
    stats.lpSplatDatas = FS_ParseSheet("Splats\\SplatData.slk", SplatDatas, sizeof(struct SplatData));
    stats.lpAnimLookups = FS_ParseSheet("UI\\SoundInfo\\AnimLookups.slk", AnimLookups, sizeof(struct AnimLookup));
    stats.lpTerrainInfo = FS_ParseSheet("TerrainArt\\Terrain.slk", TerrainInfos, sizeof(struct TerrainInfo));
    stats.lpCliffInfo = FS_ParseSheet("TerrainArt\\CliffTypes.slk", CliffInfos, sizeof(struct CliffInfo));
    stats.lpUnitBalances = FS_ParseSheet("Units\\UnitBalance.slk", UnitBalances, sizeof(struct UnitBalance));
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

LPTERRAININFO FindTerrainInfo(DWORD tileID) {
    LPTERRAININFO value = FIND_SHEET_ENTRY(stats.lpTerrainInfo, value, tileID, tileID);
    return value;
}

LPCLIFFINFO FindCliffInfo(DWORD cliffID) {
    LPCLIFFINFO value = FIND_SHEET_ENTRY(stats.lpCliffInfo, value, cliffID, cliffID);
    return value;
}

void Com_Quit(void) {
    CL_Shutdown();
    SV_Shutdown();
    FS_Shutdown();
    Sys_Quit();
}
