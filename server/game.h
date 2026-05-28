#ifndef game_h
#define game_h

#include "../common/shared.h"
#include "../common/mpq.h"

#define SVF_NOCLIENT 0x00000001    // don't send entity to clients, even if it has effects
#define SVF_DEADMONSTER 0x00000002    // treat as CONTENTS_DEADMONSTER for collision
#define SVF_MONSTER 0x00000004    // treat as CONTENTS_MONSTER for collision

KNOWN_AS(client_s, GAMECLIENT);
KNOWN_AS(edict_s, EDICT);
KNOWN_AS(link_s, LINK);

typedef struct edict_s edict_t;

struct link_s {
    LPLINK prev, next;
};

struct game_import {
    HANDLE (*MemAlloc)(long size);
    void (*MemFree)(HANDLE);
    int (*ModelIndex)(LPCSTR modelName);
    int (*SoundIndex)(LPCSTR soundName);
    int (*ImageIndex)(LPCSTR imageName);
    int (*FontIndex)(LPCSTR fontName, DWORD fontSize);
    LPCANIMATION (*GetAnimation)(DWORD modelindex, LPCSTR name);
    DWORD (*BuildHeatmap)(LPEDICT goalentity);
    BOOL (*ClosestPathablePointForRadius)(LPCVECTOR2 location, FLOAT radius, LPVECTOR2 out);
    void (*LinkEntity)(LPEDICT ent);
    void (*UnlinkEntity)(LPEDICT ent);
    DWORD (*BoxEdicts)(LPCBOX2 area, LPEDICT *list, DWORD maxcount, BOOL (*pred)(LPCEDICT));
    void (*MenuAction)(LPCSTR action, LPCSTR arg);
    VECTOR2 (*GetFlowDirection)(DWORD heatmapindex, FLOAT fx, FLOAT fy);
    FLOAT (*GetHeightAtPoint)(FLOAT x, FLOAT y);
    HANDLE (*ReadFile)(LPCSTR filename, LPDWORD size);
    DWORD (*GetTime)(void);
    sheetRow_t *(*ReadSheet)(LPCSTR sheetFilename);
    sheetRow_t *(*ReadConfig)(LPCSTR configFilename);
    LPCSTR (*FindSheetCell)(sheetRow_t *sheet, LPCSTR row, LPCSTR column);
    void (*multicast)(LPCVECTOR3 origin, multicast_t to);
    void (*unicast)(edict_t *ent);
    void (*WriteByte)(LONG c);
    void (*WriteShort)(LONG c);
    void (*WriteLong)(LONG c);
    void (*WriteFloat)(FLOAT f);
    void (*WriteString)(LPCSTR s);
    void (*WritePosition)(LPCVECTOR3 pos);
    void (*WriteDirection)(LPCVECTOR3 dir);
    void (*WriteAngle)(FLOAT f);
    void (*WriteEntity)(LPCENTITYSTATE ent);
    void (*WriteUIFrame)(LPCUIFRAME frame);

    void (*configstring)(DWORD index, LPCSTR string);
    void (*confignstring)(DWORD index, LPCSTR string, DWORD len);
    void (*error)(LPCSTR fmt, ...);

    /* Cvar access — allows the game library to read command-line/config values
     * without linking directly against common.  Returns fallback if not set. */
    LPCSTR (*CvarString)(LPCSTR name, LPCSTR fallback);
};

struct client;

/* Unit UI query result (Phase 8) */
typedef struct {
    char art[256];
    char tooltip[256];
    char ubertip[512];
    char command[256];
    char hotkey;
    BYTE x;
    BYTE y;
    BYTE research;
    BYTE active;
} gameCommandButton_t;

typedef struct {
    char art[256];
    char tooltip[256];
    char ubertip[512];
    BYTE slot;
} gameInventoryItem_t;

typedef struct {
    char art[256];
    DWORD starttime;
    DWORD endtime;
} gameQueueItem_t;

struct game_export {
    void (*Init)(void);
    void (*Shutdown)(void);
    void (*SpawnEntities)(LPCMAPINFO mapinfo, LPCDOODAD doodads);
    void (*RunFrame)(void);
    LPCSTR (*GetThemeValue)(LPCSTR filename);
    void (*ClientCommand)(LPEDICT ent, DWORD argc, LPCSTR argv[]);
    void (*ClientSetCameraPosition)(LPEDICT ent, LPCVECTOR2 position);
    void (*ClientBegin)(LPEDICT ent);
    
    edict_t *edicts;
    int num_edicts;
    int max_edicts;
    int max_clients;
    int edict_size;
};

struct game_export *GetGameAPI(struct game_import *game_import);

#endif
