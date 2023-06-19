#include "common.h"

#include <StormLib.h>

#define MPQ_PATH "/Users/igor/Documents/Warcraft3/war3.mpq"

const LPCSTR WarcraftSheets[] = {
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

static HANDLE archive;

#if 0
static void ExtractStarCraft2(void) {
    SFileOpenArchive("/Users/igor/Documents/SC2Install/Installer Tome 1.MPQ", 0, 0, &archive);
    
//    SFileOpenArchive("/Users/igor/Desktop/terrain.MPQ", 0, 0, &archive);
//    SFileOpenArchive("/Users/icherna/Documents/StarCraft2/Campaigns/Liberty.SC2Campaign/base.SC2Assets", 0, 0, &archive);
//    SFileOpenArchive("/Users/igor/Documents/StarCraft2/Mods/Liberty.SC2Mod/base.SC2Assets", 0, 0, &archive);
//    SFileOpenArchive("/Users/igor/Downloads/War3-1.27-Installer-enUS-TFT/Installer Tome.mpq", 0, 0, &archive);

    SFILE_FIND_DATA findData;
    HANDLE handle = SFileFindFirstFile(archive, "*", &findData, 0);
    if (handle) {
        LPCSTR skip[] = { NULL };// ".m3", ".ogg", ".ogv", ".fx", ".bls", ".gfx", ".wav", ".dds", ".tga", "\\Cache\\", NULL };
        do {
            for (LPCSTR *s = skip; *s; s++) {
                if (strstr(findData.cFileName, *s))
                    goto skip_print;
            }
//            if (strstr(findData.cFileName, "Azeroth")){
                printf("%s\n", findData.cFileName);
//            }
            
            if (strstr(findData.cFileName, "AbilData.xml")){
                HANDLE file;
                SFileOpenFileEx(archive, findData.cFileName, SFILE_OPEN_FROM_MPQ, &file);
                char ch;
                while (SFileReadFile(file, &ch, 1, NULL, NULL)) {
                    printf("%c", ch);
                }
                SFileCloseFile(file);
                printf("\n");
            }

        skip_print:;
        } while (SFileFindNextFile(handle, &findData));
        SFileFindClose(handle);
    }

//    LoadModel(archive, "Assets\\Cliffs\\CliffMade0\\CliffMade0_ABAB_01.m3");
//    SFileExtractFile(archive, "Repack-MPQ\\fileset.base#Campaigns#Liberty.SC2Campaign#Base.SC2Maps\\Maps\\Campaign\\TRaynor01.SC2Map\\t3Terrain.xml", "/Users/igor/Desktop/t3Terrain.xml", 0);

    exit(0);
}
#endif

HANDLE FS_OpenFile(LPCSTR fileName) {
    HANDLE file = NULL;
    SFileOpenFileEx(archive, fileName, SFILE_OPEN_FROM_MPQ, &file);
    return file;
}

void FS_CloseFile(HANDLE file) {
    SFileCloseFile(file);
}

bool FS_ExtractFile(LPCSTR toExtract, LPCSTR extracted) {
    return SFileExtractFile(archive, toExtract, extracted, 0);
}

void FS_Init(void) {
//    ExtractStarCraft2();
    SFileOpenArchive(MPQ_PATH, 0, 0, &archive);
//    SFileExtractFile(archive, "Units\\AbilityData.slk", "/Users/igor/Desktop/AbilityData.slk", 0);
//    SFileExtractFile(archive, "Units\\UnitData.slk", "/Users/igor/Desktop/UnitData.slk", 0);
//    SFileExtractFile(archive, "Units\\UnitUI.slk", "/Users/igor/Desktop/UnitUI.slk", 0);
//    SFileExtractFile(archive, "Units\\UnitMetaData.slk", "/Users/igor/Desktop/UnitMetaData.slk", 0);
//    SFileExtractFile(archive, "Units\\UnitBalance.slk", "/Users/igor/Desktop/UnitBalance.slk", 0);
//    SFileExtractFile(archive, "Units\\UnitWeapons.slk", "/Users/igor/Desktop/UnitWeapons.slk", 0);
#if 0
    SFILE_FIND_DATA findData;
    HANDLE handle = SFileFindFirstFile(archive, "*", &findData, 0);
    if (handle) {
         do {
//             if (!strstr(findData.cFileName, ".mdx") &&
//                 !strstr(findData.cFileName, ".w3m") &&
//                 !strstr(findData.cFileName, ".ai") &&
//             if (strstr(findData.cFileName, ".blp")) {
//                 !strstr(findData.cFileName, ".tga") &&
//                 !strstr(findData.cFileName, ".MDX") &&
//                 !strstr(findData.cFileName, ".wav") &&
//                 !strstr(findData.cFileName, ".mp3") &&
//                 !strstr(findData.cFileName, ".mdx") &&
//                 if(strstr(findData.cFileName, ".mdx")) {
                 printf("%s\n", findData.cFileName);
//             }
#if 1
             if (strstr(findData.cFileName, ".txt")){
                 HANDLE file;
                 SFileOpenFileEx(archive, findData.cFileName, SFILE_OPEN_FROM_MPQ, &file);
                 char ch;
                 while (SFileReadFile(file, &ch, 1, NULL, NULL)) {
                     printf("%c", ch);
                 }
                 SFileCloseFile(file);
                 printf("\n");
             }
#endif
         } while (SFileFindNextFile(handle, &findData));
         SFileFindClose(handle);
     }
#endif
//    for (LPCSTR* s = WarcraftSheets; *s; s++) {
//        printf("%s\n", *s);
//        FS_ReadSheet(*s);
//    }

//    void FPrintSheetStructs(LPCSTR *fileNames);
//    FPrintSheetStructs(WarcraftSheets);
}

void FS_Shutdown(void) {
    SFileCloseArchive(archive);
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

void Com_Init(void) {
    Cbuf_Init();
    FS_Init();
    SV_Init();
    CL_Init();
}
