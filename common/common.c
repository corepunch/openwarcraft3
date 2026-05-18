#include "common.h"

#include "mpq.h"
#include <stdlib.h>

#ifdef _WIN32
#include <io.h>
#else
#include <dirent.h>
#include <strings.h>
#endif

#include <sys/stat.h>

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
#define MAX_GAME_DIRS 16

static HANDLE archives[MAX_ARCHIVES] = { 0 };
static PATHSTR archiveNames[MAX_ARCHIVES];
static PATHSTR gameDirs[MAX_GAME_DIRS];

typedef void (*fsDiskEntryFunc_t)(LPCSTR name, LPCSTR path, BOOL isDirectory, BOOL isFile, void *userData);

HANDLE FS_AddArchive(LPCSTR filename) {
    if (!filename || !*filename) {
        return NULL;
    }
    FOR_LOOP(i, MAX_ARCHIVES) {
        if (archives[i] && !strcasecmp(archiveNames[i], filename)) {
            return archives[i];
        }
    }
    FOR_LOOP(i, MAX_ARCHIVES) {
        if (archives[i])
            continue;
        SFileOpenArchive(filename, 0, 0, &archives[i]);
        if (!archives[i]) {
            fprintf(stderr, "Can't add archive %s\n", filename);
        } else {
            snprintf(archiveNames[i], sizeof(archiveNames[i]), "%s", filename);
            fprintf(stderr, "Added archive '%s'.\n", filename);
        }
        return archives[i];
    }
    return NULL;
}

static BOOL FS_StatPath(LPCSTR filename, BOOL *isDirectory, BOOL *isFile) {
#ifdef _WIN32
    struct _stat st;

    if (!filename || !*filename) {
        return false;
    }
    if (_stat(filename, &st) != 0) {
        return false;
    }
#else
    struct stat st;

    if (!filename || !*filename) {
        return false;
    }
    if (stat(filename, &st) != 0) {
        return false;
    }
#endif
    if (isDirectory) {
#ifdef _WIN32
        *isDirectory = (st.st_mode & _S_IFDIR) != 0;
#else
        *isDirectory = S_ISDIR(st.st_mode);
#endif
    }
    if (isFile) {
#ifdef _WIN32
        *isFile = (st.st_mode & _S_IFREG) != 0;
#else
        *isFile = S_ISREG(st.st_mode);
#endif
    }
    return true;
}

static BOOL FS_DirectoryExists(LPCSTR filename) {
    BOOL isDirectory = false;

    return FS_StatPath(filename, &isDirectory, NULL) && isDirectory;
}

static BOOL FS_FileOnDiskExists(LPCSTR filename) {
    BOOL isFile = false;

    return FS_StatPath(filename, NULL, &isFile) && isFile;
}

static BOOL FS_HasExtension(LPCSTR filename, LPCSTR extension) {
    size_t filenameLen;
    size_t extensionLen;

    if (!filename || !extension) {
        return false;
    }
    filenameLen = strlen(filename);
    extensionLen = strlen(extension);
    if (filenameLen < extensionLen) {
        return false;
    }
    return !strcasecmp(filename + filenameLen - extensionLen, extension);
}

static void FS_MakeDiskPath(LPCSTR root, LPCSTR filename, LPSTR out, DWORD out_size) {
    DWORD len;

    if (!out || out_size == 0) {
        return;
    }
    out[0] = '\0';
    if (!root || !filename) {
        return;
    }
    snprintf(out, out_size, "%s/%s", root, filename);
    len = (DWORD)strlen(root);
    for (LPSTR p = out + len; *p; p++) {
        if (*p == '\\') {
            *p = '/';
        }
    }
}

static void FS_ForEachDiskEntry(LPCSTR dirname, fsDiskEntryFunc_t func, void *userData) {
    if (!dirname || !*dirname || !func) {
        return;
    }

#ifdef _WIN32
    char pattern[MAX_PATHLEN * 2];
    struct _finddata_t entry;
    intptr_t handle;

    snprintf(pattern, sizeof(pattern), "%s/*", dirname);
    handle = _findfirst(pattern, &entry);
    if (handle == -1) {
        return;
    }
    do {
        char path[MAX_PATHLEN * 2];
        BOOL isDirectory;

        if (!strcmp(entry.name, ".") || !strcmp(entry.name, "..")) {
            continue;
        }
        FS_MakeDiskPath(dirname, entry.name, path, sizeof(path));
        isDirectory = (entry.attrib & _A_SUBDIR) != 0;
        func(entry.name, path, isDirectory, !isDirectory, userData);
    } while (_findnext(handle, &entry) == 0);
    _findclose(handle);
#else
    DIR *dir = opendir(dirname);
    struct dirent *entry;

    if (!dir) {
        return;
    }
    while ((entry = readdir(dir)) != NULL) {
        char path[MAX_PATHLEN * 2];
        BOOL isDirectory = false;
        BOOL isFile = false;

        if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, "..")) {
            continue;
        }
        FS_MakeDiskPath(dirname, entry->d_name, path, sizeof(path));
        if (!FS_StatPath(path, &isDirectory, &isFile)) {
            continue;
        }
        func(entry->d_name, path, isDirectory, isFile, userData);
    }
    closedir(dir);
#endif
}

static void FS_AddGameDirectory(LPCSTR dirname) {
    char mapsDir[MAX_PATHLEN * 2];

    if (!dirname || !*dirname) {
        return;
    }
    snprintf(mapsDir, sizeof(mapsDir), "%s/Maps", dirname);
    if (!FS_DirectoryExists(mapsDir)) {
        return;
    }
    FOR_LOOP(i, MAX_GAME_DIRS) {
        if (gameDirs[i][0] && !strcasecmp(gameDirs[i], dirname)) {
            return;
        }
    }
    FOR_LOOP(i, MAX_GAME_DIRS) {
        if (gameDirs[i][0]) {
            continue;
        }
        snprintf(gameDirs[i], sizeof(gameDirs[i]), "%s", dirname);
        fprintf(stderr, "Added game directory '%s'.\n", dirname);
        return;
    }
}

static int FS_ComparePaths(const void *a, const void *b) {
    PATHSTR const *pa = a;
    PATHSTR const *pb = b;

    return strcasecmp(*pa, *pb);
}

typedef struct {
    PATHSTR *paths;
    DWORD maxPaths;
    DWORD count;
} fsArchiveScan_t;

static void FS_AddArchiveScanEntry(LPCSTR name, LPCSTR path, BOOL isDirectory, BOOL isFile, void *userData) {
    fsArchiveScan_t *scan = userData;

    (void)isDirectory;
    if (!scan || !isFile || scan->count >= scan->maxPaths) {
        return;
    }
    if (!FS_HasExtension(name, ".mpq")) {
        return;
    }
    snprintf(scan->paths[scan->count++], sizeof(PATHSTR), "%s", path);
}

BOOL FS_AddDataDirectory(LPCSTR dirname) {
    PATHSTR archivePaths[MAX_ARCHIVES];
    fsArchiveScan_t scan = { archivePaths, MAX_ARCHIVES, 0 };
    DWORD mountedCount = 0;

    if (!FS_DirectoryExists(dirname)) {
        return false;
    }

    FS_AddGameDirectory(dirname);
    FS_ForEachDiskEntry(dirname, FS_AddArchiveScanEntry, &scan);

    qsort(archivePaths, scan.count, sizeof(archivePaths[0]), FS_ComparePaths);
    FOR_LOOP(i, scan.count) {
        if (FS_AddArchive(archivePaths[i])) {
            mountedCount++;
        }
    }
    return mountedCount > 0;
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

static BOOL filelock = false;
void PF_Sleep(DWORD msec);

typedef struct fsFind_s {
    DWORD archiveIndex;
    HANDLE current;
    PATHSTR mask;
    PATHSTR *looseFiles;
    DWORD looseCount;
    DWORD looseIndex;
} fsFind_t;

static BOOL FS_FindFilenameMatches(LPCSTR filename, LPCSTR mask) {
    size_t mask_len;

    if (!mask || !mask[0] || !strcmp(mask, "*")) {
        return true;
    }
    mask_len = strlen(mask);
    if (mask_len > 0 && mask[mask_len - 1] == '*') {
        return !strncasecmp(filename, mask, mask_len - 1);
    }
    return !strcasecmp(filename, mask);
}

static BOOL FS_FindAppendLoose(fsFind_t *find, LPCSTR name) {
    PATHSTR *next;

    if (!find || !name || !*name) {
        return false;
    }
    next = realloc(find->looseFiles, (find->looseCount + 1) * sizeof(*next));
    if (!next) {
        return false;
    }
    find->looseFiles = next;
    snprintf(find->looseFiles[find->looseCount++], sizeof(PATHSTR), "%s", name);
    return true;
}

typedef struct {
    fsFind_t *find;
    LPCSTR root;
    LPCSTR rel;
} fsLooseCollect_t;

static void FS_FindCollectLooseDir(fsFind_t *find, LPCSTR root, LPCSTR rel);

static void FS_FindCollectLooseEntry(LPCSTR name, LPCSTR path, BOOL isDirectory, BOOL isFile, void *userData) {
    fsLooseCollect_t *collect = userData;
    char childRel[MAX_PATHLEN * 2];

    (void)path;
    if (!collect || !collect->find || !name || !*name) {
        return;
    }
    snprintf(childRel,
             sizeof(childRel),
             "%s%s%s",
             collect->rel,
             *collect->rel ? "\\" : "",
             name);
    if (isDirectory) {
        FS_FindCollectLooseDir(collect->find, collect->root, childRel);
    } else if (isFile && FS_FindFilenameMatches(childRel, collect->find->mask)) {
        FS_FindAppendLoose(collect->find, childRel);
    }
}

static void FS_FindCollectLooseDir(fsFind_t *find, LPCSTR root, LPCSTR rel) {
    char path[MAX_PATHLEN * 2];
    fsLooseCollect_t collect = { find, root, rel };

    if (!find || !root || !rel) {
        return;
    }
    FS_MakeDiskPath(root, rel, path, sizeof(path));
    FS_ForEachDiskEntry(path, FS_FindCollectLooseEntry, &collect);
}

HANDLE FS_OpenFile(LPCSTR fileName) {
    while (filelock) {
        PF_Sleep(10);
    }
    filelock = true;
    if (!fileName || !*fileName) {
        filelock = false;
        return NULL;
    }
    FOR_LOOP(i, MAX_ARCHIVES) {
        HANDLE file;
        if (SFileOpenFileEx(archives[i], fileName, SFILE_OPEN_FROM_MPQ, &file)) {
            return file;
        }
    }
    filelock = false;
    return NULL;
}

bool FS_FileExists(LPCSTR fileName) {
    HANDLE file = FS_OpenFile(fileName);
    if (file) {
        FS_CloseFile(file);
        return true;
    }
    FOR_LOOP(i, MAX_GAME_DIRS) {
        char path[MAX_PATHLEN * 2];

        if (!gameDirs[i][0]) {
            continue;
        }
        FS_MakeDiskPath(gameDirs[i], fileName, path, sizeof(path));
        if (FS_FileOnDiskExists(path)) {
            return true;
        }
    }
    return false;
}

HANDLE FS_ReadLooseFile(LPCSTR filename, LPDWORD size, DWORD extraBytes) {
    FOR_LOOP(i, MAX_GAME_DIRS) {
        char path[MAX_PATHLEN * 2];
        FILE *file;
        long fileSize;
        LPBYTE buffer;

        if (!gameDirs[i][0]) {
            continue;
        }
        FS_MakeDiskPath(gameDirs[i], filename, path, sizeof(path));
        file = fopen(path, "rb");
        if (!file) {
            continue;
        }
        if (fseek(file, 0, SEEK_END) != 0) {
            fclose(file);
            continue;
        }
        fileSize = ftell(file);
        if (fileSize < 0 || fseek(file, 0, SEEK_SET) != 0) {
            fclose(file);
            continue;
        }
        buffer = MemAlloc(fileSize + extraBytes);
        if (!buffer) {
            fclose(file);
            return NULL;
        }
        if (fileSize > 0 && fread(buffer, 1, fileSize, file) != (size_t)fileSize) {
            MemFree(buffer);
            fclose(file);
            return NULL;
        }
        fclose(file);
        if (extraBytes > 0) {
            memset(buffer + fileSize, 0, extraBytes);
        }
        if (size) {
            *size = (DWORD)fileSize;
        }
        return buffer;
    }
    return NULL;
}

void FS_CloseFile(HANDLE file) {
    SFileCloseFile(file);
    filelock = false;
}

static BOOL FS_FindAdvance(fsFind_t *find, SFILE_FIND_DATA *findData) {
    if (find && findData && find->looseIndex < find->looseCount) {
        LPCSTR name = find->looseFiles[find->looseIndex++];

        memset(findData, 0, sizeof(*findData));
        snprintf(findData->cFileName, sizeof(findData->cFileName), "%s", name);
        findData->szPlainName = findData->cFileName;
        return true;
    }
    while (find && find->archiveIndex < MAX_ARCHIVES) {
        if (find->current && SFileFindNextFile(find->current, findData)) {
            return true;
        }
        if (find->current) {
            SFileFindClose(find->current);
            find->current = NULL;
        }
        find->archiveIndex++;
        while (find->archiveIndex < MAX_ARCHIVES && !archives[find->archiveIndex]) {
            find->archiveIndex++;
        }
        if (find->archiveIndex < MAX_ARCHIVES) {
            find->current = SFileFindFirstFile(archives[find->archiveIndex], find->mask, findData, NULL);
            if (find->current) {
                return true;
            }
        }
    }
    return false;
}

HANDLE FS_FindFirstFile(LPCSTR mask, SFILE_FIND_DATA *findData) {
    fsFind_t *find;

    while (filelock) {
        PF_Sleep(10);
    }
    filelock = true;
    if (!mask || !*mask || !findData) {
        filelock = false;
        return NULL;
    }

    find = calloc(1, sizeof(*find));
    if (!find) {
        filelock = false;
        return NULL;
    }
    snprintf(find->mask, sizeof(find->mask), "%s", mask);
    LPCSTR looseRoot = (!strncasecmp(mask, "Maps\\", 5) || !strncasecmp(mask, "Maps/", 5)) ? "Maps" : "";
    FOR_LOOP(i, MAX_GAME_DIRS) {
        if (gameDirs[i][0]) {
            FS_FindCollectLooseDir(find, gameDirs[i], looseRoot);
        }
    }
    find->archiveIndex = 0;
    while (find->archiveIndex < MAX_ARCHIVES && !archives[find->archiveIndex]) {
        find->archiveIndex++;
    }
    if (find->archiveIndex < MAX_ARCHIVES) {
        find->current = SFileFindFirstFile(archives[find->archiveIndex], find->mask, findData, NULL);
        if (find->current) {
            return find;
        }
    }
    if (FS_FindAdvance(find, findData)) {
        return find;
    }
    free(find);
    filelock = false;
    return NULL;
}

BOOL FS_FindNextFile(HANDLE handle, SFILE_FIND_DATA *findData) {
    return FS_FindAdvance(handle, findData);
}

BOOL FS_FindClose(HANDLE handle) {
    fsFind_t *find = handle;

    if (find) {
        if (find->current) {
            SFileFindClose(find->current);
        }
        free(find->looseFiles);
        free(find);
    }
    filelock = false;
    return true;
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
    
//    FS_AddArchive("/Users/igor/Documents/Warcraft III Demo/war3.mpq");
//    FS_AddArchive("/Users/igor/Documents/StarCraft2/Campaigns/Liberty.SC2Campaign/base.SC2Assets");

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
#if 1
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
        archives[i] = NULL;
        archiveNames[i][0] = '\0';
    }
    FOR_LOOP(i, MAX_GAME_DIRS) {
        gameDirs[i][0] = '\0';
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
    NET_Shutdown();
    FS_Shutdown();
    Sys_Quit();
}

void MenuAction(LPCSTR action, LPCSTR arg) {
    if (!strcmp(action, "load")) {
        if (!arg || !*arg) {
            return;
        }
        CL_SetGameplayBindings();
        SV_Map(arg);
    } else if (!strcmp(action, "quit")) {
        Com_Quit();
    }
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
