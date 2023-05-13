#ifndef game_h
#define game_h

#include "../common/common.h"

struct entity_state {
    struct vector3 position;
    float angle;
    struct vector3 scale;
    int model;
    int image;
};

struct game_import {
    void *(*MemAlloc)(long size);
    void (*MemFree)(void *);
    int (*ModelIndex)(LPCSTR szModelName);
    int (*SoundIndex)(LPCSTR szSoundName);
    int (*ImageIndex)(LPCSTR szImageName);
};

struct game_export {
    void (*Init)(void);
    void (*Shutdown)(void);
    void (*SpawnEntities)(struct Doodad const *doodads, int numDoodads);
    
    struct edict *edicts;
    int num_edicts;
    int max_edicts;
    int edict_size;
};

struct game_export *GetGameAPI(struct game_import *gi);

#endif
