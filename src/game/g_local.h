#ifndef g_local_h
#define g_local_h

#include "../server/game.h"

#define SAFE_CALL(FUNC, ...) if (FUNC) FUNC(__VA_ARGS__)

enum {
    AI_HOLD_FRAME = 1 << 0,
};

typedef enum {
    MOVETYPE_NONE,            // never moves
    MOVETYPE_NOCLIP,          // origin and angles change with no interaction
    MOVETYPE_PUSH,            // no clip to world, push on box contact
    MOVETYPE_STOP,            // no clip to world, stops on box contact
    MOVETYPE_WALK,            // gravity
    MOVETYPE_STEP,            // gravity, special edge handling
    MOVETYPE_FLY,
    MOVETYPE_TOSS,            // gravity
    MOVETYPE_FLYMISSILE,      // extra size to monsters
    MOVETYPE_BOUNCE
} movetype_t;

KNOWN_AS(UnitData, UNITDATA);
KNOWN_AS(UnitUI, UNITUI);
KNOWN_AS(DoodadInfo, DOODADINFO);
KNOWN_AS(DestructableData, DESTRUCTABLEDATA);

typedef struct {
    LPCSTR animation;
    void (*think)(LPEDICT self);
    void (*endfunc)(LPEDICT self);
} mmove_t;

typedef struct {
    mmove_t *currentmove;
    DWORD aiflags;
    int health;
    void (*stand)(LPEDICT self);
    void (*walk)(LPEDICT self);
    void (*run)(LPEDICT self);
    void (*melee)(LPEDICT self);
    bool (*checkattack)(LPEDICT self);
} monsterinfo_t;

struct edict {
    ENTITYSTATE s;
    DWORD class_id;
    int variation;
    movetype_t movetype;
    LPEDICT goalentity;
    LPEDICT enemy;

    void (*prethink)(LPEDICT self);
    void (*think)(LPEDICT self);
    void (*die)(LPEDICT self, LPEDICT attacker);

    monsterinfo_t monsterinfo;
};

struct game_state {
    LPEDICT edicts;
};

struct UnitData {
    DWORD unitID;
    SHEETSTR sort;
    SHEETSTR comment;
    SHEETSTR race;
    DWORD prio;
    DWORD threat;
    DWORD type;
    DWORD valid;
    DWORD deathType;
    float death;
    DWORD canSleep;
    DWORD cargoSize;
    SHEETSTR movetp;
    DWORD moveHeight;
    DWORD moveFloor;
    DWORD launchX;
    DWORD launchY;
    DWORD launchZ;
    DWORD impactZ;
    float turnRate;
    DWORD propWin;
    DWORD orientInterp;
    DWORD formation;
    float castpt;
    float castbsw;
    SHEETSTR targType;
    SHEETSTR pathTex;
    DWORD fatLOS;
    DWORD collision;
    DWORD points;
    DWORD buffType;
    DWORD buffRadius;
    DWORD nameCount;
    DWORD InBeta;
    struct UnitData *lpNext;
};

struct UnitUI {
    DWORD unitUIID;
    SHEETSTR file;
    SHEETSTR unitSound;
    SHEETSTR tilesets;
    DWORD tilesetSpecific;
    SHEETSTR name;
    SHEETSTR unitClass;
    DWORD special;
    DWORD inEditor;
    DWORD hiddenInEditor;
    DWORD hostilePal;
    DWORD dropItems;
    DWORD nbrandom;
    DWORD nbmmIcon;
    DWORD useClickHelper;
    float blend;
    float scale;
    DWORD scaleBull;
    SHEETSTR preventPlace;
    DWORD requirePlace;
    DWORD isbldg;
    DWORD maxPitch;
    DWORD maxRoll;
    DWORD elevPts;
    DWORD elevRad;
    DWORD fogRad;
    DWORD walk;
    DWORD run;
    DWORD selZ;
    DWORD weap1;
    DWORD weap2;
    DWORD teamColor;
    DWORD customTeamColor;
    SHEETSTR armor;
    DWORD modelScale;
    DWORD red;
    DWORD green;
    DWORD blue;
    SHEETSTR uberSplat;
    SHEETSTR unitShadow;
    SHEETSTR buildingShadow;
    DWORD shadowW;
    DWORD shadowH;
    DWORD shadowX;
    DWORD shadowY;
    DWORD occH;
    DWORD InBeta;
    LPUNITUI lpNext;
};

struct DoodadInfo {
    DWORD doodID;
    char dir[64];
    char file[64];
    char pathTex[64];
    LPDOODADINFO lpNext;
    LPCMODEL lpModel;
};

struct DestructableData {
    DWORD DestructableID;
    char category[64];
    char tilesets[64];
    char tilesetSpecific[64];
    char dir[64];
    char file[64];
    DWORD lightweight;
    DWORD fatLOS;
    DWORD texID;
    char texFile[64];
    LPDESTRUCTABLEDATA lpNext;
    LPCMODEL lpModel;
    LPTEXTURE lpTexture;
};

struct game_locals {
    LPUNITUI UnitUI;
    struct UnitData *UnitData;
    LPDOODADINFO Doodads;
    LPDESTRUCTABLEDATA DestructableData;
};


LPEDICT G_Spawn(void);
LPDOODADINFO G_FindDoodadInfo(int doodID);
LPDESTRUCTABLEDATA G_FindDestructableData(int DestructableID);
LPUNITUI G_FindUnitUI(int unitUIID);
void SP_SpawnUnit(LPEDICT lpEdict, LPCUNITUI lpUnit);
void SP_CallSpawn(LPEDICT lpEdict);
void G_SpawnEntities(LPCDOODAD doodads, DWORD numDoodads);
void G_InitUnits(void);
void G_InitDestructables(void);
void G_InitDoodads(void);
void G_RunEntity(LPEDICT lpEdict);

LPEDICT Waypoint_add(VECTOR2 spot);
void M_CheckGround (LPEDICT self);
void monster_start(LPEDICT self);

void ai_attack(LPEDICT self);
void ai_walk(LPEDICT self);
void ai_run(LPEDICT self);
void ai_stand(LPEDICT self);

void M_MoveToGoal(LPEDICT self);
void M_ChangeAngle(LPEDICT self);
bool M_CheckAttack(LPEDICT self);

extern struct game_locals game;
extern struct game_state game_state;
extern struct game_export globals;
extern struct game_import gi;

#endif
