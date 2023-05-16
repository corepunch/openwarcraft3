#ifndef game_h
#define game_h

#include "../common/common.h"

struct game_import {
    void *(*MemAlloc)(long size);
    void (*MemFree)(void *);
    int (*ModelIndex)(LPCSTR szModelName);
    int (*SoundIndex)(LPCSTR szSoundName);
    int (*ImageIndex)(LPCSTR szImageName);
    struct animation_info (*GetAnimation)(LPCSTR modelname, LPCSTR animation);
    void *(*ParseSheet)(LPCSTR szSheetFilename, struct SheetLayout const *lpLayout, int dwElementSize, void *lpNextFieldOffset);
};

struct game_export {
    void (*Init)(void);
    void (*Shutdown)(void);
    void (*SpawnEntities)(struct Doodad const *doodads, int numDoodads);
    void (*RunFrame)(int msec);
    
    struct edict *edicts;
    int num_edicts;
    int max_edicts;
    int edict_size;
};

struct game_export *GetGameAPI(struct game_import *game_import);

#endif
