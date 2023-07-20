#ifndef g_local_h
#define g_local_h

#include <stdlib.h>
#include <ctype.h>
#include <limits.h>

#include "../server/game.h"
#include "g_shared.h"
#include "g_unitdata.h"
#include "parser.h"

#define SAFE_CALL(FUNC, ...) if (FUNC) FUNC(__VA_ARGS__)
#define ABILITY(NAME) void M_##NAME(LPEDICT ent, LPEDICT target)
#define UI_SCALE(x) ((x) * 10000)
#define SEL_SCALE 72
#define MAX_BUILD_QUEUE 7

#define FILTER_EDICTS(ENT, CONDITION) \
for (LPEDICT ENT = globals.edicts; \
ENT - globals.edicts < globals.num_edicts; \
ENT++) if (CONDITION)

#define UI_FRAME(NAME) LPFRAMEDEF NAME = UI_FindFrame(#NAME);
#define UI_CHILD_FRAME(NAME, PARENT) LPFRAMEDEF NAME = UI_FindChildFrame(PARENT, #NAME);
#define UI_CHILD_VALUE(NAME, PARENT, VALUE, ...) \
LPFRAMEDEF NAME = UI_FindChildFrame(PARENT, #NAME); \
if (NAME) { \
    UI_Set##VALUE(NAME, __VA_ARGS__); \
} else { \
    fprintf(stderr, #NAME " not found");\
}

#define FOR_SELECTED_UNITS(CLIENT, ENT) \
FILTER_EDICTS(ENT, G_IsEntitySelected(CLIENT, ENT))

enum {
    LAYER_PORTRAIT,
    LAYER_CONSOLE,
    LAYER_COMMANDBAR,
    LAYER_INFOPANEL,
};

#define svc_bad 0
// these ops are known to the game dll
//    svc_muzzleflash,
//    svc_muzzleflash2,
#define svc_temp_entity 1
#define svc_layout 2
#define svc_playerinfo 3
#define svc_cursor 4

KNOWN_AS(uiFrameDef_s, FRAMEDEF);

typedef struct {
    BOOL (*on_entity_selected)(LPEDICT clent, LPEDICT selected);
    BOOL (*on_location_selected)(LPEDICT clent, LPCVECTOR2 location);
    void (*cmdbutton)(LPEDICT clent, DWORD code);
} menu_t;

enum {
    AI_HOLD_FRAME = 1 << 0,
};

typedef enum {
    ATK_NONE,
    ATK_NORMAL,
    ATK_PIERCE,
    ATK_SIEGE,
    ATK_SPELLS,
    ATK_CHAOS,
    ATK_MAGIC,
    ATK_HERO,
} attackType_t;

typedef enum {
    WPN_NONE,
    WPN_NORMAL,
    WPN_INSTANT,
    WPN_ARTILLERY,
    WPN_ALINE,
    WPN_MISSILE,
    WPN_MSPLASH,
    WPN_MBOUNCE,
    WPN_MLINE,
} weaponType_t;

typedef enum {
    TARG_NONE,
    TARG_AIR,
    TARG_ALIVE,
    TARG_ALLIES,
    TARG_DEAD,
    TARG_DEBRIS,
    TARG_ENEMIES,
    TARG_GROUND,
    TARG_HERO,
    TARG_INVULNERABLE,
    TARG_ITEM,
    TARG_MECHANICAL,
    TARG_NEUTRAL,
    TARG_NONHERO,
    TARG_NONSAPPER,
    TARG_NOTSELF,
    TARG_ORGANIC,
    TARG_PLAYERUNITS,
    TARG_SAPPER,
    TARG_SELF,
    TARG_STRUCTURE,
    TARG_TERRAIN,
    TARG_TREE,
    TARG_VULNERABLE,
    TARG_WALL,
    TARG_WARD,
    TARG_ANCIENT,
    TARG_NONANCIENT,
    TARG_FRIEND,
    TARG_BRIDGE,
    TARG_DECORATION,
} TARGTYPE;

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
} MOVETYPE;

typedef enum { // Keep in sync with uiFramePointPos_t
    FRAMEPOINT_TOPLEFT,
    FRAMEPOINT_TOP,
    FRAMEPOINT_TOPRIGHT,
    FRAMEPOINT_UNUSED1,
    FRAMEPOINT_LEFT,
    FRAMEPOINT_CENTER,
    FRAMEPOINT_RIGHT,
    FRAMEPOINT_UNUSED2,
    FRAMEPOINT_BOTTOMLEFT,
    FRAMEPOINT_BOTTOM,
    FRAMEPOINT_BOTTOMRIGHT,
    FRAMEPOINT_UNUSED3,
} UIFRAMEPOINT;

typedef enum {
    FONTFLAGS_FIXEDSIZE,
    FONTFLAGS_PASSWORDFIELD,
} UIFONTFLAGS;

struct uiFrameDef_s {
    uiFrame_t f;
    UINAME Name;
    UINAME Text;
    RECT rect;
    BOOL DecorateFileNames;
    BOOL inuse;
    BOOL AnyPointsSet;
    BOOL hidden;
    struct {
        BOOL TileBackground;
        DWORD Background;
        UINAME CornerFlags;// "UL|UR|BL|BR|T|L|B|R",
        SHORT CornerSize;
        SHORT BackgroundSize;
        SHORT BackgroundInsets[4];// 0.01 0.01 0.01 0.01,
        DWORD EdgeFile;//  "EscMenuBorder",
        BOOL BlendAll;
    } Backdrop;
    DWORD DialogBackdrop;
    struct {
        UIFRAMEPOINT corner;
        USHORT x, y;
    } Anchor;
    struct {
        UIFRAMEPOINT type;
        DWORD relativeTo;
        UIFRAMEPOINT target;
        USHORT x, y;
    } SetPoint;
    struct {
        UINAME Name;
        UINAME Unknown;
        UIFONTFLAGS FontFlags;
        DWORD Size;
        COLOR32 Color;
        COLOR32 HighlightColor;
        COLOR32 DisabledColor;
        COLOR32 ShadowColor;
        VECTOR2 ShadowOffset;
    } Font;
};

struct client_s {
    playerState_t ps;
    DWORD ping;
    menu_t menu;
};

typedef struct {
    LPCSTR animation;
    void (*think)(LPEDICT self);
    void (*endfunc)(LPEDICT self);
} umove_t;

typedef struct ability_s {
    LPCSTR classname;
    void (*init)(struct ability_s *ability);
    void (*cmd)(LPEDICT ent);
    void *data;

    // client side info
    DWORD buttonimage;
    struct { DWORD x, y; } buttonpos;
} ability_t;

typedef struct {
    attackType_t type;
    weaponType_t weapon;
    VECTOR3 origin;
    DWORD damageBase;
    DWORD numberOfDice;
    DWORD sidesPerDie;
    FLOAT damagePoint;
    FLOAT cooldown;
    struct {
        DWORD model;
        FLOAT arc;
        FLOAT speed;
    } projectile;
} unitAttack_t;

typedef struct {
    FLOAT value;
    FLOAT max_value;
} EDICTSTAT;

struct edict_s {
    entityState_t s;
    LPGAMECLIENT client;
    pathTex_t *pathtex;
    DWORD svflags;
    DWORD selected;

    // keep above in sync with server.h
    DWORD class_id;
    DWORD variation;
    DWORD build_project;
    DWORD spawn_time;
    doodadHero_t hero;
    FLOAT collision;
    FLOAT velocity;
    EDICTSTAT health;
    EDICTSTAT mana;
    DWORD harvested_lumber;
    DWORD harvested_gold;
    MOVETYPE movetype;
    TARGTYPE targtype;
    DWORD heatmap2;
    LPEDICT goalentity;
    LPEDICT secondarygoal;
    LPEDICT owner;
    LPEDICT build;
    LPCANIMATION animation;
    BOOL inuse;
    DWORD peonsinside;
    umove_t *currentmove;
    DWORD aiflags;
    DWORD damage;
//    unitRace_t race;
    FLOAT wait;
    unitAttack_t attack1;
    unitAttack_t attack2;

    void (*stand)(LPEDICT self);
    void (*birth)(LPEDICT self);
    void (*prethink)(LPEDICT self);
    void (*think)(LPEDICT self);
    void (*pain)(LPEDICT self);
    void (*die)(LPEDICT self, LPEDICT attacker);
};

struct game_state {
    LPEDICT edicts;
};

struct game_locals {
    DWORD max_clients;
    DWORD num_abilities;
    LPGAMECLIENT clients;
    struct {
        sheetRow_t *abilities;
        sheetRow_t *theme;
        sheetRow_t *splats;
        sheetRow_t *uberSplats;
        sheetRow_t *misc;
    } config;
    struct {
        FLOAT attackHalfAngle;
        FLOAT maxCollisionRadius;
        FLOAT decayTime;
        FLOAT boneDecayTime;
        FLOAT dissipateTime;
        FLOAT structureDecayTime;
        FLOAT bulletDeathTime;
        FLOAT closeEnoughRange;
        FLOAT dawnTimeGameHours;
        FLOAT duskTimeGameHours;
        FLOAT gameDayHours;
        FLOAT gameDayLength;
        FLOAT buildingAngle;
        FLOAT rootAngle;
    } constants;
};

struct level_locals {
    
};

typedef struct sheetMetaData_s {
    LPCSTR id;
    LPCSTR field;
    LPCSTR slk;
    sheetRow_t *table;
} sheetMetaData_t;

// g_main.c
void G_ClientCommand(LPEDICT ent, DWORD argc, LPCSTR argv[]);
playerState_t *G_GetPlayerByNumber(DWORD number);
TARGTYPE G_GetTargetType(LPCSTR str);

// g_spawn.c
LPEDICT G_Spawn(void);
void G_FreeEdict(LPEDICT ent);
void SP_CallSpawn(LPEDICT edict);
void G_SpawnEntities(LPCSTR mapname, LPCDOODAD doodads);
BOOL SP_FindEmptySpaceAround(LPEDICT townhall, DWORD class_id, LPVECTOR2 out, FLOAT *angle);
LPEDICT SP_SpawnAtLocation(DWORD class_id, DWORD player, LPCVECTOR2 location);

LPEDICT Waypoint_add(LPCVECTOR2 spot);
void M_CheckGround (LPEDICT self);
void monster_start(LPEDICT self);

// g_ai.c
void ai_birth(LPEDICT self);
void ai_stand(LPEDICT self);
void ai_pain(LPEDICT self);
void M_RunWait(LPEDICT self, void (*callback)(LPEDICT ));

// g_monster.c
void M_MoveInDirection(LPEDICT self);
void M_ChangeAngle(LPEDICT self);
BOOL M_CheckAttack(LPEDICT self);
void M_SetAnimation(LPEDICT self, LPCSTR anim);
void M_SetMove(LPEDICT self, umove_t *move);
FLOAT M_DistanceToGoal(LPEDICT ent);
FLOAT M_MoveDistance(LPEDICT self);
DWORD M_RefreshHeatmap(LPEDICT self);
BOOL M_IsDead(LPEDICT ent);
void SP_SpawnUnit(LPEDICT edict);
void SP_TrainUnit(LPEDICT townhall, DWORD class_id);
BOOL player_pay(playerState_t *ps, DWORD project);
BYTE compress_stat(EDICTSTAT const *stat);

// g_pathing.c
pathTex_t *LoadTGA(BYTE const* mem, size_t size);

// g_move.c
BOOL SV_CloseEnough(LPEDICT self, LPCEDICT goal, FLOAT distance);

// g_phys.c
void G_RunEntity(LPEDICT edict);
void G_SolveCollisions(void);
BOOL M_CheckCollision(LPCVECTOR2 origin, FLOAT radius);

// g_abilities.c
ability_t *FindAbilityByClassname(LPCSTR classname);
ability_t *GetAbilityByIndex(DWORD index);
DWORD FindAbilityIndex(LPCSTR classname);
void InitAbilities(void);
void SetAbilityNames(void);

// g_config.c
LPCSTR FindConfigValue(LPCSTR category, LPCSTR field);
LPCSTR GetClassName(DWORD class_id);

// p_hud.c
LPEDICT G_GetMainSelectedUnit(LPGAMECLIENT client);
void Get_Commands_f(LPEDICT ent);
void Get_Portrait_f(LPEDICT ent);
void UI_AddCancelButton(LPEDICT edict);

void UI_AddCommandButton(LPCSTR ability);

// p_fdf.c
void UI_PrintClasses(void);
void UI_ClearTemplates(void);
void UI_ParseFDF(LPCSTR fileName);
void UI_ParseFDF_Buffer(LPCSTR fileName, LPSTR buffer);
void UI_SetAllPoints(LPFRAMEDEF frame);
void UI_SetParent(LPFRAMEDEF frame, LPFRAMEDEF parent);
void UI_SetText(LPFRAMEDEF frame, LPCSTR format, ...);
void UI_SetSize(LPFRAMEDEF frame, DWORD width, DWORD height);
void UI_SetTexture(LPFRAMEDEF frame, LPCSTR name, BOOL decorate);
void UI_SetTexture2(LPFRAMEDEF frame, LPCSTR name, BOOL decorate);
void UI_WriteLayout(LPEDICT ent, LPCFRAMEDEF frames, DWORD layer);
void UI_SetPoint(LPFRAMEDEF frame, UIFRAMEPOINT framePoint, LPFRAMEDEF other, UIFRAMEPOINT otherPoint, int16_t x, int16_t y);
void UI_SetPointByNumber(LPFRAMEDEF frame, UIFRAMEPOINT framePoint, DWORD otherNumber, UIFRAMEPOINT otherPoint, SHORT x, SHORT y);
void UI_InitFrame(LPFRAMEDEF frame, DWORD number, uiFrameType_t type);
void UI_WriteFrame(LPCFRAMEDEF frame);
void UI_WriteFrameWithChildren(LPCFRAMEDEF frame);
void UI_WriteLayout2(LPEDICT ent, void (*BuildUI)(LPGAMECLIENT ), DWORD layer);
void UI_SetHidden(LPFRAMEDEF frame, BOOL value);
DWORD UI_FindFrameNumber(LPCSTR name);
DWORD UI_LoadTexture(LPCSTR file, BOOL decorate);
LPCSTR UI_GetString(LPCSTR textID);
LPFRAMEDEF UI_Spawn(uiFrameType_t type, LPFRAMEDEF parent);
LPFRAMEDEF UI_FindFrame(LPCSTR name);
LPFRAMEDEF UI_FindChildFrame(LPFRAMEDEF frame, LPCSTR name);

// g_metadata.c
LPCSTR UnitStringField(sheetMetaData_t *meta, DWORD unit_id, LPCSTR name);
LONG UnitIntegerField(sheetMetaData_t *meta, DWORD unit_id, LPCSTR name);
BOOL UnitBooleanField(sheetMetaData_t *meta, DWORD unit_id, LPCSTR name);
FLOAT UnitRealField(sheetMetaData_t *meta, DWORD unit_id, LPCSTR name);

void InitUnitData(void);
void ShutdownUnitData(void);

// g_command.c
void G_SelectEntity(LPGAMECLIENT client, LPEDICT ent);
void G_DeselectEntity(LPGAMECLIENT client, LPEDICT ent);
BOOL G_IsEntitySelected(LPGAMECLIENT client, LPEDICT ent);

//  s_skills.c
FLOAT AB_Number(ability_t const *ability, LPCSTR field);

// g_combat.c
void T_Damage(LPEDICT target, LPEDICT attacker, int damage);

// globals
extern struct game_locals game;
extern struct game_state game_state;
extern struct game_export globals;
extern struct game_import gi;
extern struct level_locals level;

extern sheetMetaData_t UnitsMetaData[];
extern sheetMetaData_t DestructableMetaData[];
extern sheetMetaData_t DoodadsMetaData[];

#endif
