#ifndef game_h
#define game_h

#include "../common/common.h"

typedef enum {
    ANIM_STAND,
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
    LPCANIMATION (*GetAnimation)(int modelindex, animationType_t animtype);
    HANDLE (*ParseSheet)(LPCSTR sheetFilename, LPCSHEETLAYOUT layout, DWORD elementSize);
    float (*GetHeightAtPoint)(float x, float y);
    void (*error) (char *fmt, ...);
};

struct game_export {
    void (*Init)(void);
    void (*Shutdown)(void);
    void (*SpawnDoodads)(LPCDOODAD doodads);
    void (*RunFrame)(void);
    void (*ClientCommand)(LPCCLIENTMESSAGE message);

    LPEDICT edicts;
    int num_edicts;
    int max_edicts;
    int edict_size;
};

struct game_export *GetGameAPI(struct game_import *game_import);

#endif
