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
    bool (*ExtractFile)(LPCSTR toExtract, LPCSTR extracted);
    LPCANIMATION (*GetAnimation)(DWORD modelindex, LPCSTR name);
    DWORD (*BuildHeatmap)(LPEDICT goalentity);
    BOOL (*ClosestPathablePoint)(LPCVECTOR2 location, LPVECTOR2 out);
    BOOL (*ClosestPathablePointForRadius)(LPCVECTOR2 location, FLOAT radius, LPVECTOR2 out);
    void (*LinkEntity)(LPEDICT ent);
    void (*UnlinkEntity)(LPEDICT ent);
    DWORD (*BoxEdicts)(LPCBOX2 area, LPEDICT *list, DWORD maxcount, BOOL (*pred)(LPCEDICT));
    void (*MenuAction)(LPCSTR action, LPCSTR arg);
    VECTOR2 (*GetFlowDirection)(DWORD heatmapindex, FLOAT fx, FLOAT fy);
    FLOAT (*GetHeightAtPoint)(FLOAT x, FLOAT y);
    LPSTR (*ReadFileIntoString)(LPCSTR filename);
    HANDLE (*ReadFile)(LPCSTR filename, LPDWORD size);
    HANDLE (*FindFirstFile)(LPCSTR mask, SFILE_FIND_DATA *findData);
    BOOL (*FindNextFile)(HANDLE find, SFILE_FIND_DATA *findData);
    BOOL (*FindClose)(HANDLE find);
    BOOL (*OpenArchiveFromMemory)(const void *data, DWORD size, DWORD flags, HANDLE *archive);
    BOOL (*CloseArchive)(HANDLE archive);
    BOOL (*OpenFileEx)(HANDLE archive, LPCSTR fileName, DWORD searchScope, HANDLE *file);
    BOOL (*CloseFile)(HANDLE file);
    BOOL (*ReadArchiveFile)(HANDLE file, void *buffer, DWORD toRead, LPDWORD bytesRead, LPOVERLAPPED overlapped);
    DWORD (*GetArchiveFileSize)(HANDLE file, LPDWORD highSize);
    DWORD (*GetTime)(void);
    sheetRow_t *(*ReadSheet)(LPCSTR sheetFilename);
    sheetRow_t *(*ReadConfig)(LPCSTR configFilename);
    LPCSTR (*FindSheetCell)(sheetRow_t *sheet, LPCSTR row, LPCSTR column);
    void (*TextRemoveComments)(LPSTR buffer);
    BOMStatus (*TextRemoveBom)(LPSTR buffer);

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
