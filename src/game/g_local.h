#ifndef g_local_h
#define g_local_h

#include <stdlib.h>

#include "../server/game.h"
#include "g_shared.h"
#include "g_unitdata.h"

#define SAFE_CALL(FUNC, ...) if (FUNC) FUNC(__VA_ARGS__)
#define ABILITY(NAME) void M_##NAME(edict_t *ent, edict_t *target)
#define UI_SCALE(x) ((x) * 10000)
#define SEL_SCALE 72

#define EDICT_FUNC(NAME) void NAME(edict_t *ent)

enum {
    UI_CONSOLE,
    UI_COMMANDBAR,
};

#define MAKE_UI_LAYER(LAYER) \
UI_Clear(); \
gi.WriteByte(svc_layout); \
gi.WriteByte(LAYER);

#define svc_bad 0
// these ops are known to the game dll
//    svc_muzzleflash,
//    svc_muzzleflash2,
//    svc_temp_entity,
#define svc_layout 1
#define svc_playerinfo 2
#define svc_cursor 3

typedef struct {
    void (*mouseup)(edict_t *ent, LPCVECTOR2 mouse);
    void (*cmdbutton)(edict_t *ent, DWORD code);
} menu_t;

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

typedef struct uiFrameDef_s uiFrameDef_t;
typedef uint32_t pointNames_t[FPP_COUNT];

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
} uiFramePointType_t;

struct uiFrameDef_s {
    uiFrame_t f;
    DWORD Name;
    struct uiFrameDef_s *parent;
    RECT rect;
    bool DecorateFileNames;
    bool inuse;
    struct {
        bool TileBackground;
        uiName_t Background;
        uiName_t CornerFlags;// "UL|UR|BL|BR|T|L|B|R",
        float CornerSize;
        float BackgroundSize;
        VECTOR4 BackgroundInsets;// 0.01 0.01 0.01 0.01,
        uiName_t EdgeFile;//  "EscMenuBorder",
        bool BlendAll;
    } Backdrop;
    DWORD DialogBackdrop;
    struct {
        uiFramePointType_t corner;
        int16_t x, y;
    } Anchor;
    struct {
        uiFramePointType_t type;
        uint32_t relativeTo;
        uiFramePointType_t target;
        int16_t x, y;
    } SetPoint;
    struct {
        pointNames_t x, y;
    } pointNames;
    struct {
        uiName_t Name;
        DWORD Size;
    } Font;
};

struct client_s {
    playerState_t ps;
    DWORD ping;
    menu_t menu;
    DWORD lastcode;
};

typedef struct {
    LPCSTR animation;
    void (*think)(edict_t *self);
    void (*endfunc)(edict_t *self);
} umove_t;

typedef struct ability_s {
    LPCSTR classname;
    void (*init)(struct ability_s *ability);
    void (*use)(edict_t *ent, edict_t *target);
    void (*cmd)(edict_t *ent);
    void *data;

    // client side info
    DWORD buttonimage;
    struct { DWORD x, y; } buttonpos;
} ability_t;

typedef struct {
    umove_t *currentmove;
    DWORD aiflags;
//    unitRace_t race;
    float wait;
} unitinfo_t;

struct edict_s {
    entityState_t s;
    gclient_t *client;
    DWORD svflags;
    DWORD selected;
    
    // keep above in sync with server.h
    DWORD class_id;
    DWORD variation;
    float health;
    DWORD harvested_lumber;
    movetype_t movetype;
    handle_t heatmap;
    edict_t *goalentity;
    edict_t *secondarygoal;
    animation_t const *animation;
    bool inuse;

    void (*stand)(edict_t *self);
    void (*birth)(edict_t *self);
    void (*prethink)(edict_t *self);
    void (*think)(edict_t *self);
    void (*pain)(edict_t *self);
    void (*die)(edict_t *self, edict_t *attacker);

    unitinfo_t unitinfo;
};

struct game_state {
    edict_t *edicts;
};

struct game_locals {
    LPUNITUI UnitUI;
    LPDOODADINFO Doodads;
    DWORD max_clients;
    DWORD num_abilities;
    gclient_t *clients;
    sheetRow_t *theme;
};

typedef struct sheetMetaData_s {
    LPCSTR id;
    LPCSTR field;
    LPCSTR slk;
    sheetRow_t *table;
} sheetMetaData_t;

edict_t *G_Spawn(void);
void SP_SpawnUnit(edict_t *edict);
void SP_CallSpawn(edict_t *edict);
void G_SpawnEntities(LPCDOODAD doodads);
void G_SpawnUnits(LPCDOODAD units, DWORD num_units);
void G_BuildHeatmap(edict_t *edict, LPCVECTOR2 location);
void G_ClientCommand(edict_t *ent, DWORD argc, LPCSTR argv[]);
playerState_t *G_GetPlayerByNumber(DWORD number);

edict_t *Waypoint_add(LPCVECTOR2 spot);
void M_CheckGround (edict_t *self);
void monster_start(edict_t *self);

// g_ai.c
void ai_birth(edict_t *self);
void ai_stand(edict_t *self);
void ai_pain(edict_t *self);
void M_RunWait(edict_t *self, void (*callback)(edict_t *));

// g_monster.c
void M_MoveInDirection(edict_t *self);
void M_ChangeAngle(edict_t *self);
bool M_CheckAttack(edict_t *self);
void M_SetAnimation(edict_t *self, LPCSTR anim);
void M_SetMove(edict_t *self, umove_t *move);
float M_DistanceToGoal(edict_t *ent);
float M_MoveDistance(edict_t *self);

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
LPCSTR FindConfigValue(LPCSTR category, LPCSTR field);
LPCSTR GetClassName(DWORD class_id);

// p_hud.c
edict_t *GetMainSelectedEntity(gclient_t *client);
void Get_Commands_f(edict_t *ent);
void UI_AddAbilityButton(uiFrameDef_t *root, LPCSTR ability);
void UI_AddCancelButton(edict_t *edict);
void Add_CommandButtonCoded(uiFrameDef_t *root, DWORD code, LPCSTR art, LPCSTR buttonpos);

// p_fdf.c
uiFrameDef_t *UI_Clear(void);
uiFrameDef_t *UI_Spawn(uiFrameType_t type, uiFrameDef_t *parent);
uiFrameDef_t *FDF_ParseFile(uiFrameDef_t const *root, LPCSTR fileName);
uiFrameDef_t *UI_FindFrameByName(uiFrameDef_t *frame, LPCSTR name);
void UI_WriteLayout(edict_t *ent, uiFrameDef_t const *frames, DWORD layer);
void UI_SetPoint(uiFrameDef_t *frame, uiFramePointType_t framePoint, uiFrameDef_t *other, uiFramePointType_t otherPoint, float x, float y);

// g_metadata.c
LPCSTR UnitStringField(sheetMetaData_t *meta, DWORD unit_id, LPCSTR name);
int UnitIntegerField(sheetMetaData_t *meta, DWORD unit_id, LPCSTR name);
bool UnitBooleanField(sheetMetaData_t *meta, DWORD unit_id, LPCSTR name);
float UnitRealField(sheetMetaData_t *meta, DWORD unit_id, LPCSTR name);

void InitUnitData(void);
void ShutdownUnitData(void);

// g_command.c
edict_t *client_getentityatpoint(gclient_t *client, LPCVECTOR2 point);
void client_selectentity(gclient_t *client, edict_t *ent);
bool client_isentityselected(gclient_t *client, edict_t *ent);
VECTOR2 client_getpointlocation(gclient_t *client, LPCVECTOR2 point);

// globals
extern struct game_locals game;
extern struct game_state game_state;
extern struct game_export globals;
extern struct game_import gi;

extern sheetRow_t *AbilityData;

extern sheetMetaData_t UnitsMetaData[];
extern sheetMetaData_t DestructableMetaData[];
extern sheetMetaData_t DoodadsMetaData[];

#endif
