#ifndef g_local_h
#define g_local_h

#include "../server/game.h"
#include "g_shared.h"

#define SAFE_CALL(FUNC, ...) if (FUNC) FUNC(__VA_ARGS__)

#define CMD_MOVE "CmdMove"
#define CMD_STOP "CmdStop"
#define CMD_HOLDPOS "CmdHoldPos"
#define CMD_PATROL "CmdPatrol"
#define CMD_ATTACK "CmdAttack"
#define CMD_BUILD_HUMAN "CmdBuildHuman"
#define CMD_BUILD_ORC "CmdBuildOrc"
#define CMD_HOLDPOS "CmdHoldPos"
#define CMD_PATROL "CmdPatrol"
#define CMD_STOP "CmdStop"

#define svc_bad 0
// these ops are known to the game dll
//    svc_muzzleflash,
//    svc_muzzleflash2,
//    svc_temp_entity,
#define svc_layout 1
#define svc_playerinfo 2
#define svc_inventory 3

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

struct gclient_s {
    playerState_t ps;
    DWORD ping;
};

typedef struct {
    animationType_t animation;
    void (*think)(edict_t *self);
    void (*endfunc)(edict_t *self);
} umove_t;

typedef struct ability_s {
    LPCSTR classname;
    void (*use)(edict_t *ent, struct ability_s const *ability);
} ability_t;

typedef struct {
    umove_t *currentmove;
    DWORD aiflags;
    DWORD health;
    DWORD wait;
    struct UnitWeapons const *weapon;
    struct UnitBalance const *balance;
    struct UnitUI const *ui;
    struct UnitAbilities const *abilities;
    struct UnitData const *data;
    void (*stand)(edict_t *self);
    void (*walk)(edict_t *self);
    void (*run)(edict_t *self);
    void (*melee)(edict_t *self);
    bool (*checkattack)(edict_t *self);
} unitinfo_t;

struct edict_s {
    entityState_t s;
    gclient_t *client;
    DWORD svflags;

    // keep above in sync with server.h
    DWORD variation;
    movetype_t movetype;
    handle_t heatmap;
    edict_t *goalentity;
    edict_t *enemy;
    bool inuse;
    animationInfo_t const *animation;
    
    void (*prethink)(edict_t *self);
    void (*think)(edict_t *self);
    void (*die)(edict_t *self, edict_t *attacker);

    unitinfo_t unitinfo;
};

struct game_state {
    edict_t *edicts;
};

struct game_locals {
    LPUNITUI UnitUI;
    struct UnitData *UnitData;
    LPDOODADINFO Doodads;
    LPDESTRUCTABLEDATA DestructableData;
    DWORD max_clients;
    DWORD num_abilities;
    gclient_t *clients;
};

edict_t *G_Spawn(void);
void SP_SpawnUnit(edict_t *edict, LPCUNITUI unit);
void SP_CallSpawn(edict_t *edict);
void G_SpawnEntities(LPCDOODAD doodads);
void G_SpawnUnits(LPCDOODAD units, DWORD num_units);
void G_BuildHeatmap(edict_t *edict, LPCVECTOR2 location);
void G_ClientCommand(edict_t *ent, DWORD argc, LPCSTR argv[]);

edict_t *Waypoint_add(VECTOR2 spot);
void M_CheckGround (edict_t *self);
void monster_start(edict_t *self);

// g_ai.c
void ai_melee(edict_t *self);
void ai_walk(edict_t *self);
void ai_stand(edict_t *self);
void ai_cooldown(edict_t *self);

// g_monster.c
void M_MoveToGoal(edict_t *self);
void M_ChangeAngle(edict_t *self);
bool M_CheckAttack(edict_t *self);

// g_move.c
bool SV_CloseEnough(edict_t *self, edict_t const *goal, float distance);

// g_phys.c
void G_RunEntity(edict_t *edict);
void G_SolveCollisions(void);

// g_abilities.c
ability_t *FindAbilityByClassname(LPCSTR classname);
ability_t *GetAbilityByIndex(DWORD index);
DWORD FindAbilityIndex(LPCSTR classname);
void InitAbilities(void);
void SetAbilityNames(void);

// g_config.c
void InitConfigFiles(void);
void ShutdownConfigFiles(void);
LPCSTR FindConfigValue(LPCSTR category, LPCSTR field);
LPCSTR GetClassName(DWORD class_id);

extern struct game_locals game;
extern struct game_state game_state;
extern struct game_export globals;
extern struct game_import gi;

#endif
