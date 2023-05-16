#ifndef g_local_h
#define g_local_h

#include "../server/game.h"

struct monsterinfo {
    struct animation_info animation;
};

struct edict {
    struct entity_state s;
    uint32_t class_id;
    int variation;
    
    void (*think)(struct edict *, int msec);

    struct monsterinfo monsterinfo;
};

struct game_state {
    struct edict *edicts;
};

struct UnitData {
    int unitID;
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

struct UnitUI {
    int unitUIID;
    sheetString_t file;
    sheetString_t unitSound;
    sheetString_t tilesets;
    int tilesetSpecific;
    sheetString_t name;
    sheetString_t unitClass;
    int special;
    int inEditor;
    int hiddenInEditor;
    int hostilePal;
    int dropItems;
    int nbrandom;
    int nbmmIcon;
    int useClickHelper;
    float blend;
    float scale;
    int scaleBull;
    sheetString_t preventPlace;
    int requirePlace;
    int isbldg;
    int maxPitch;
    int maxRoll;
    int elevPts;
    int elevRad;
    int fogRad;
    int walk;
    int run;
    int selZ;
    int weap1;
    int weap2;
    int teamColor;
    int customTeamColor;
    sheetString_t armor;
    int modelScale;
    int red;
    int green;
    int blue;
    sheetString_t uberSplat;
    sheetString_t unitShadow;
    sheetString_t buildingShadow;
    int shadowW;
    int shadowH;
    int shadowX;
    int shadowY;
    int occH;
    int InBeta;
    struct UnitUI *lpNext;
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
    struct UnitUI *UnitUI;
    struct UnitData *UnitData;
    struct DoodadInfo *Doodads;
    struct DestructableData *DestructableData;
};

struct edict *G_Spawn(void);
void SP_CallSpawn(struct edict *ent);
void G_SpawnEntities(struct Doodad const *doodads, int numDoodads);
void G_InitUnits(void);
void G_InitDestructables(void);
void G_InitDoodads(void);
struct DoodadInfo *G_FindDoodadInfo(int doodID);
struct DestructableData *G_FindDestructableData(int DestructableID);
struct UnitUI *G_FindUnitUI(int unitUIID);

extern struct game_locals game;
extern struct game_state game_state;
extern struct game_export globals;
extern struct game_import gi;

#endif