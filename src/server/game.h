#ifndef game_h
#define game_h

#include "../common/common.h"

struct game_import {
    HANDLE (*MemAlloc)(long size);
    void (*MemFree)(HANDLE);
    int (*ModelIndex)(LPCSTR szModelName);
    int (*SoundIndex)(LPCSTR szSoundName);
    int (*ImageIndex)(LPCSTR szImageName);
    struct AnimationInfo (*GetAnimation)(int modelindex, LPCSTR animation);
    HANDLE (*ParseSheet)(LPCSTR szSheetFilename, LPCSHEETLAYOUT lpLayout, DWORD dwElementSize, HANDLE lpNextFieldOffset);
};

struct game_export {
    void (*Init)(void);
    void (*Shutdown)(void);
    void (*SpawnEntities)(LPCDOODAD doodads, DWORD numDoodads);
    void (*RunFrame)(DWORD msec);
    
    LPEDICT edicts;
    int num_edicts;
    int max_edicts;
    int edict_size;
};

struct game_export *GetGameAPI(struct game_import *game_import);

#endif
