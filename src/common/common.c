#include "common.h"

#include <StormLib.h>

#define MAXPRINTMSG 4096

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

#define MAX_ARCHIVES 64

static HANDLE archives[MAX_ARCHIVES] = { 0 };

HANDLE FS_AddArchive(LPCSTR filename) {
    FOR_LOOP(i, MAX_ARCHIVES) {
        if (archives[i])
            continue;
        SFileOpenArchive(filename, 0, 0, &archives[i]);
        return archives[i];
    }
    return NULL;
}

#if 0
static void ExtractStarCraft2(void) {
    HANDLE archive;
    SFileOpenArchive("/Users/igor/Documents/SC2Install/Installer Tome 1.MPQ", 0, 0, &archive);
//    SFileOpenArchive("/Users/igor/Desktop/terrain.MPQ", 0, 0, &archive);
//    SFileOpenArchive("/Users/igor/Documents/StarCraft2/Campaigns/Liberty.SC2Campaign/base.SC2Assets", 0, 0, &archive);
//    SFileOpenArchive("/Users/igor/Documents/StarCraft2/Mods/Liberty.SC2Mod/base.SC2Assets", 0, 0, &archive);
//    SFileOpenArchive("/Users/igor/Documents/StarCraft2/Mods/Liberty.SC2Mod/base.SC2Data", 0, 0, &archive);
//    SFileOpenArchive("/Users/igor/Downloads/War3-1.27-Installer-enUS-TFT/Installer Tome.mpq", 0, 0, &archive);
//    SFileExtractFile(archive, "Assets\\Units\\Terran\\MarineTychus\\MarineTychus.m3", "/Users/igor/Desktop/MarineTychus.m3", 0);
//    SFileExtractFile(archive, "Assets\\Textures\\SpecialOps_Dropship_Diffuse.dds", "/Users/igor/Desktop/SpecialOps_Dropship_Diffuse.dds", 0);
    
    SFILE_FIND_DATA findData;
    HANDLE handle = SFileFindFirstFile(archive, "*", &findData, 0);
    if (handle) {
        LPCSTR skip[] = { NULL };// ".m3", ".ogg", ".ogv", ".fx", ".bls", ".gfx", ".wav", ".dds", ".tga", "\\Cache\\", NULL };
        do {
            for (LPCSTR *s = skip; *s; s++) {
                if (strstr(findData.cFileName, *s))
                    goto skip_print;
            }
            if (strstr(findData.cFileName, ".galaxy")) {
//                printf("%s\n", findData.cFileName);
            }
            if (strstr(findData.cFileName, "MapScript.galaxy")) {
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
    FOR_LOOP(i, MAX_ARCHIVES) {
        HANDLE file;
        if (SFileOpenFileEx(archives[i], fileName, SFILE_OPEN_FROM_MPQ, &file))
            return file;
    }
    return NULL;
}

bool FS_FileExists(LPCSTR fileName) {
    HANDLE file = FS_OpenFile(fileName);
    if (file) {
        FS_CloseFile(file);
        return true;
    } else {
        return false;
    }
}

void FS_CloseFile(HANDLE file) {
    SFileCloseFile(file);
}

bool FS_ExtractFile(LPCSTR toExtract, LPCSTR extracted) {
    FOR_LOOP(i, MAX_ARCHIVES) {
        if (SFileExtractFile(archives[i], toExtract, extracted, 0))
            return true;
    }
    return false;
}

void FS_Init(void) {
//    ExtractStarCraft2();
    
    FS_AddArchive("/Users/igor/Documents/Warcraft3/war3.mpq");
    FS_AddArchive("/Users/igor/Documents/StarCraft2/Campaigns/Liberty.SC2Campaign/base.SC2Assets");

//    FS_ExtractFile("Scripts\\common.j", "/Users/igor/Desktop/common.j");
//    FS_ExtractFile("Units\\UnitAbilities.slk", "/Users/igor/Desktop/UnitAbilities.slk");
//    FS_ExtractFile("Units\\UnitData.slk", "/Users/igor/Desktop/UnitData.slk");
//    FS_ExtractFile("Units\\UnitUI.slk", "/Users/igor/Desktop/UnitUI.slk");
//    FS_ExtractFile("Units\\UnitMetaData.slk", "/Users/igor/Desktop/UnitMetaData.slk");
//    FS_ExtractFile("Units\\UnitBalance.slk", "/Users/igor/Desktop/UnitBalance.slk");
//    FS_ExtractFile("Units\\UnitWeapons.slk", "/Users/igor/Desktop/UnitWeapons.slk");
//    FS_ExtractFile("Units\\AbilityData.slk", "/Users/igor/Desktop/AbilityData.slk");
//    FS_ExtractFile("Units\\ItemData.slk", "/Users/igor/Desktop/ItemData.slk");
//    FS_ExtractFile("Splats\\SplatData.slk", "/Users/igor/Desktop/SplatData.slk");
//    FS_ExtractFile("Splats\\UberSplatData.slk", "/Users/igor/Desktop/UberSplatData.slk");
//    FS_ExtractFile("Units\\MiscData.txt", "/Users/igor/Desktop/MiscData.txt");
//    FS_ExtractFile("Abilities\\Weapons\\FireBallMissile\\FireBallMissile.mdx", "/Users/igor/Desktop/FireBallMissile.mdx");
//    FS_ExtractFile("UI\\FrameDef\\GlobalStrings.fdf", "/Users/igor/Desktop/GlobalStrings.fdf");
//    FS_ExtractFile("UI\\FrameDef\\UI\\ConsoleUI.fdf", "/Users/igor/Desktop/ConsoleUI.fdf");

#if 0
    SFILE_FIND_DATA findData;
    HANDLE handle = SFileFindFirstFile(archives[0], "*", &findData, 0);
    if (handle) {
         do {
//             if(strstr(findData.cFileName, ".fdf")) {
//                 printf("%s\n", findData.cFileName);
//             }
#if 0
//             if (strstr(findData.cFileName, "EscMenuTemplates")||strstr(findData.cFileName, "QuestDialog")){
//             if (strstr(findData.cFileName, "Blizzard.j")) {
//             if (strstr(findData.cFileName, "EscMenuTemplates") ||
//                strstr(findData.cFileName, "CinematicPanel")) {
              if (strstr(findData.cFileName, ".txt")){
                 HANDLE file;
                 SFileOpenFileEx(archives[0], findData.cFileName, SFILE_OPEN_FROM_MPQ, &file);
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
    FOR_LOOP(i, MAX_ARCHIVES) {
        SFileCloseArchive(archives[i]);
    }
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

void Com_Error(errorCode_t code, LPCSTR fmt, ...) {
    va_list argptr;
    static char msg[MAXPRINTMSG];
    static bool recursive;

    if (recursive) {
        fprintf(stderr, "recursive error after: %s", msg);
    }
    
    recursive = true;

    va_start(argptr,fmt);
    vsprintf(msg,fmt,argptr);
    va_end(argptr);
    
    switch (code) {
        case ERR_QUIT:
//            CL_Drop ();
            recursive = false;
//            longjmp (abortframe, -1);
            break;
        case ERR_DROP:
            break;
        default:
            break;
    }
    
    fprintf(stderr, "%s", msg);
}
