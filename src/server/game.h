#ifndef game_h
#define game_h

#include "../common/shared.h"

#define SVF_NOCLIENT 0x00000001    // don't send entity to clients, even if it has effects
#define SVF_DEADMONSTER 0x00000002    // treat as CONTENTS_DEADMONSTER for collision
#define SVF_MONSTER 0x00000004    // treat as CONTENTS_MONSTER for collision

#define MAX_ANIMS_IN_TYPE 16

typedef struct edict_s edict_t;
typedef struct client_s gclient_t;

struct game_import {
    HANDLE (*MemAlloc)(long size);
    void (*MemFree)(HANDLE);
    int (*ModelIndex)(LPCSTR modelName);
    int (*SoundIndex)(LPCSTR soundName);
    int (*ImageIndex)(LPCSTR imageName);
    int (*FontIndex)(LPCSTR fontName, DWORD fontSize);
    animation_t const *(*GetAnimation)(int modelindex, LPCSTR name);
    handle_t (*BuildHeatmap)(LPCVECTOR2 target);
    VECTOR2 (*GetFlowDirection)(DWORD heatmapindex, float fx, float fy);
    float (*GetHeightAtPoint)(float x, float y);
    LPSTR (*ParserGetToken)(parser_t *p);
    LPSTR (*ReadFileIntoString)(LPCSTR filename);
    
    sheetRow_t *(*ReadSheet)(LPCSTR sheetFilename);
    sheetRow_t *(*ReadConfig)(LPCSTR configFilename);
    LPCSTR (*FindSheetCell)(sheetRow_t *sheet, LPCSTR row, LPCSTR column);

    void (*multicast)(LPCVECTOR3 origin, multicast_t to);
    void (*unicast)(edict_t *ent);
    void (*WriteByte)(int c);
    void (*WriteShort)(int c);
    void (*WriteLong)(int c);
    void (*WriteFloat)(float f);
    void (*WriteString)(LPCSTR s);
    void (*WritePosition)(LPCVECTOR3 pos);
    void (*WriteDirection)(LPCVECTOR3 dir);
    void (*WriteAngle)(float f);
    void (*WriteEntity)(entityState_t const *ent);
    void (*WriteUIFrame)(uiFrame_t const *frame);

    void (*configstring)(DWORD index, LPCSTR string);
    void (*confignstring)(DWORD index, LPCSTR string, DWORD len);
    void (*error)(LPCSTR fmt, ...);
};

struct client;

struct game_export {
    void (*Init)(void);
    void (*Shutdown)(void);
    void (*SpawnEntities)(LPCDOODAD doodads);
    void (*RunFrame)(void);
    LPCSTR (*GetThemeValue)(LPCSTR filename);

    void (*ClientCommand)(edict_t *ent, DWORD argc, LPCSTR argv[]);
    void (*ClientBegin)(edict_t *ent);

    edict_t *edicts;
    int num_edicts;
    int max_edicts;
    int max_clients;
    int edict_size;
};

struct game_export *GetGameAPI(struct game_import *game_import);

#endif
