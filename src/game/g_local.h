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
    animationType_t animation;
    void (*think)(LPEDICT self);
    void (*endfunc)(LPEDICT self);
} mmove_t;

typedef struct {
    mmove_t *currentmove;
    DWORD aiflags;
    DWORD health;
    DWORD movespeed;
    struct UnitWeapons const *weapon;
    struct UnitBalance const *balance;
    void (*stand)(LPEDICT self);
    void (*walk)(LPEDICT self);
    void (*run)(LPEDICT self);
    void (*melee)(LPEDICT self);
    bool (*checkattack)(LPEDICT self);
} monsterinfo_t;

struct edict {
    entityState_t s;
    DWORD svflags;

    // keep above in sync with server.h

    DWORD class_id;
    DWORD variation;
    movetype_t movetype;
    LPEDICT goalentity;
    LPEDICT enemy;

    pathPoint_t *path;
    
    void (*prethink)(LPEDICT self);
    void (*think)(LPEDICT self);
    void (*die)(LPEDICT self, LPEDICT attacker);

    monsterinfo_t monsterinfo;
};

struct game_state {
    LPEDICT edicts;
};

struct game_locals {
    LPUNITUI UnitUI;
    struct UnitData *UnitData;
    LPDOODADINFO Doodads;
    LPDESTRUCTABLEDATA DestructableData;
};


LPEDICT G_Spawn(void);
void SP_SpawnUnit(LPEDICT edict, LPCUNITUI unit);
void SP_CallSpawn(LPEDICT edict);
void G_SpawnDoodads(LPCDOODAD doodads);
void G_SpawnUnits(LPCDOODAD units, DWORD num_units);
void G_RunEntity(LPEDICT edict);
void G_FindPath(LPEDICT edict, LPCVECTOR2 location);

LPEDICT Waypoint_add(VECTOR2 spot);
void M_CheckGround (LPEDICT self);
void monster_start(LPEDICT self);

void ai_melee(LPEDICT self);
void ai_walk(LPEDICT self);
void ai_stand(LPEDICT self);

void M_MoveToGoal(LPEDICT self);
void M_ChangeAngle(LPEDICT self);
bool M_CheckAttack(LPEDICT self);

extern struct game_locals game;
extern struct game_state game_state;
extern struct game_export globals;
extern struct game_import gi;

#endif
