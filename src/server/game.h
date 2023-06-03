#ifndef game_h
#define game_h

#include "../common/shared.h"

#define SVF_NOCLIENT 0x00000001    // don't send entity to clients, even if it has effects
#define SVF_DEADMONSTER 0x00000002    // treat as CONTENTS_DEADMONSTER for collision
#define SVF_MONSTER 0x00000004    // treat as CONTENTS_MONSTER for collision

#define MAX_ANIMS_IN_TYPE 16

typedef struct edict_s edict_t;
typedef struct gclient_s gclient_t;

typedef enum {
    ANIM_STAND,
    ANIM_STAND_READY,
    ANIM_STAND_VICTORY,
    ANIM_STAND_CHANNEL,
    ANIM_STAND_HIT,
    ANIM_WALK,
    ANIM_ATTACK,
    ANIM_DEATH,
    NUM_ANIM_TYPES
}  animationType_t;

typedef struct {
    VECTOR3 viewangles;        // for fixed views
    VECTOR3 viewoffset;        // add to pmovestate->origin

    float fov;            // horizontal field of view

    int rdflags;        // refdef flags

//    short stats[MAX_STATS];        // fast status bar updates
} playerState_t;

struct game_import {
    HANDLE (*MemAlloc)(long size);
    void (*MemFree)(HANDLE);
    int (*ModelIndex)(LPCSTR modelName);
    int (*SoundIndex)(LPCSTR soundName);
    int (*ImageIndex)(LPCSTR imageName);
    animationInfo_t const *(*GetAnimation)(int modelindex, animationType_t animtype);
    HANDLE (*ParseSheet)(LPCSTR sheetFilename, LPCSHEETLAYOUT layout, DWORD elementSize);
    configValue_t *(*ParseConfig)(LPCSTR configFilename);
    LPCSTR (*FindConfigValue)(configValue_t *config, LPCSTR sectionName, LPCSTR valueName);
    handle_t (*BuildHeatmap)(LPCVECTOR2 target);
    VECTOR2 (*GetFlowDirection)(DWORD heatmapindex, float fx, float fy);
    float (*GetHeightAtPoint)(float x, float y);
    LPSTR (*ParserGetToken)(parser_t *p);

    void (*multicast)(LPCVECTOR3 origin, multicast_t to);
    void (*unicast)(edict_t *ent);
    void (*WriteByte)(int c);
    void (*WriteShort)(int c);
    void (*WriteLong)(int c);
    void (*WriteFloat)(float f);
    void (*WriteString)(char *s);
    
    void (*configstring)(DWORD index, LPCSTR string);
    void (*error)(char *fmt, ...);
};

struct client;

struct game_export {
    void (*Init)(void);
    void (*Shutdown)(void);
    void (*SpawnEntities)(LPCDOODAD doodads);
    void (*RunFrame)(void);
    void (*ClientCommand)(edict_t *ent, DWORD argc, LPCSTR argv[]);

    edict_t *edicts;
    int num_edicts;
    int max_edicts;
    int max_clients;
    int edict_size;
};

struct game_export *GetGameAPI(struct game_import *game_import);

#endif
