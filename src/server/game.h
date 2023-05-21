#ifndef game_h
#define game_h

#include "../common/common.h"

struct game_import {
    HANDLE (*MemAlloc)(long size);
    void (*MemFree)(HANDLE);
    int (*ModelIndex)(LPCSTR szModelName);
    int (*SoundIndex)(LPCSTR szSoundName);
    int (*ImageIndex)(LPCSTR szImageName);
    ANIMATION (*GetAnimation)(int modelindex, LPCSTR animation);
    HANDLE (*ParseSheet)(LPCSTR szSheetFilename, LPCSHEETLAYOUT lpLayout, DWORD dwElementSize);
    float (*GetHeightAtPoint)(float x, float y);
    void (*error) (char *fmt, ...);
};

struct game_export {
    void (*Init)(void);
    void (*Shutdown)(void);
    void (*SpawnDoodads)(LPCDOODAD doodads, DWORD numDoodads);
    void (*SpawnUnits)(LPCDOODADUNIT units, DWORD numUnits);
    void (*RunFrame)(void);
    void (*ClientCommand)(LPCCLIENTMESSAGE lpMessage);
    
    LPEDICT edicts;
    int num_edicts;
    int max_edicts;
    int edict_size;
};

struct game_export *GetGameAPI(struct game_import *game_import);

#endif
