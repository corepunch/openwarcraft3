#ifndef game_h
#define game_h

#include "../common/common.h"

#define SVF_NOCLIENT 0x00000001    // don't send entity to clients, even if it has effects
#define SVF_DEADMONSTER 0x00000002    // treat as CONTENTS_DEADMONSTER for collision
#define SVF_MONSTER 0x00000004    // treat as CONTENTS_MONSTER for collision

#define MAX_ANIMS_IN_TYPE 16

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
    void (*error) (char *fmt, ...);
};

struct game_export {
    void (*Init)(void);
    void (*Shutdown)(void);
    void (*SpawnDoodads)(LPCDOODAD doodads);
    void (*RunFrame)(void);
    void (*ClientCommand)(clientMessage_t const *message);

    LPEDICT edicts;
    int num_edicts;
    int max_edicts;
    int edict_size;
};

struct game_export *GetGameAPI(struct game_import *game_import);

#endif
