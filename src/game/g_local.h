#ifndef g_local_h
#define g_local_h

#include "../server/game.h"

struct monstermove {
    int firstframe;
    int lastframe;
};

struct monsterinfo {
    struct monstermove currentmove;
};

struct edict {
    struct entity_state s;

    LPCSTR classname;
    
    void (*think)(struct edict *, int msec);

    struct monsterinfo monsterinfo;
};

struct game_state {
    struct edict *edicts;
};

struct UnitData {
    DWORD unitID;
    sheetString_t sort;
    sheetString_t comment;
    sheetString_t race;
    int prio;
    int threat;
    int type;
    int valid;
    int deathType;
    float death;
    int canSleep;
    int cargoSize;
    sheetString_t movetp;
    int moveHeight;
    int moveFloor;
    int launchX;
    int launchY;
    int launchZ;
    int impactZ;
    float turnRate;
    int propWin;
    int orientInterp;
    int formation;
    float castpt;
    float castbsw;
    sheetString_t targType;
    sheetString_t pathTex;
    int fatLOS;
    int collision;
    int points;
    int buffType;
    int buffRadius;
    int nameCount;
    int InBeta;
    struct UnitData *lpNext;
};

struct DoodadInfo {
    int doodID;
    char dir[64];
    char file[64];
    char pathTex[64];
    struct DoodadInfo *lpNext;
    struct tModel const *lpModel;
};

struct DestructableData {
    int DestructableID;
    char category[64];
    char tilesets[64];
    char tilesetSpecific[64];
    char dir[64];
    char file[64];
    int lightweight;
    int fatLOS;
    int texID;
    char texFile[64];
    struct DestructableData *lpNext;
    struct tModel const *lpModel;
    struct texture *lpTexture;
};

struct game_locals {
    struct UnitData *UnitData;
    struct DoodadInfo *Doodads;
    struct DestructableData *DestructableData;
};

struct edict *G_Spawn(void);
int G_LoadModelDirFile(LPCSTR szDirectory, LPCSTR szFileName, int dwVariation);
void SP_CallSpawn(struct edict *ent);
void G_SpawnEntities(struct Doodad const *doodads, int numDoodads);
void G_InitUnits(void);
void G_InitDestructables(void);
void G_InitDoodads(void);
struct DoodadInfo *G_FindDoodadInfo(int doodID);
struct DestructableData *G_FindDestructableData(int DestructableID);

extern struct game_locals game;
extern struct game_state game_state;
extern struct game_export globals;
extern struct game_import gi;

#endif
