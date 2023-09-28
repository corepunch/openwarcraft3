#ifndef g_local_h
#define g_local_h

#include <stdlib.h>
#include <ctype.h>
#include <limits.h>

#include "../server/game.h"
#include "g_shared.h"
#include "g_unitdata.h"
#include "parser.h"

#define EDICTFIELD(x, type) { #x, FOFS(edict_s, x)-(HANDLE)NULL, type }

#define SAFE_CALL(FUNC, ...) if (FUNC) FUNC(__VA_ARGS__)
#define ABILITY(NAME) void M_##NAME(LPEDICT ent, LPEDICT target)
#define SEL_SCALE 72
#define MAX_BUILD_QUEUE 7
#define MAX_EVENT_QUEUE 256
#define MAX_ENTITIES 4096
#define MAX_REGION_SIZE 16
#define MAX_INVENTORY 6
#define FOV_ASPECT 1.7

#define FILTER_EDICTS(ENT, CONDITION) \
for (LPEDICT ENT = globals.edicts; \
ENT - globals.edicts < globals.num_edicts; \
ENT++) if (CONDITION)

#define PLAYER_NUM(PLAYER) (PLAYER->number)
#define PLAYER_ENT(PLAYER) G_GetPlayerEntityByNumber(PLAYER_NUM(PLAYER))
#define PLAYER_CLIENT(PLAYER) G_GetPlayerClientByNumber(PLAYER_NUM(PLAYER))

#define UI_FRAME(NAME) LPFRAMEDEF NAME = UI_FindFrame(#NAME);
#define UI_CHILD_FRAME(NAME, PARENT) LPFRAMEDEF NAME = UI_FindChildFrame(PARENT, #NAME);
#define UI_CHILD_VALUE(NAME, PARENT, VALUE, ...) \
LPFRAMEDEF NAME = UI_FindChildFrame(PARENT, #NAME); \
if (NAME) { \
    UI_Set##VALUE(NAME, __VA_ARGS__); \
} else { \
    fprintf(stderr, #NAME " not found");\
}

#define UI_WRITE_LAYER(ent, BuildUI, layer, ...) \
    UI_WriteStart(layer); \
    BuildUI(ent->client, ##__VA_ARGS__); \
    gi.WriteLong(0); \
    gi.unicast(ent);


#define FOR_SELECTED_UNITS(CLIENT, ENT) \
FILTER_EDICTS(ENT, G_IsEntitySelected(CLIENT, ENT))

enum {
    LAYER_PORTRAIT,
    LAYER_CINEMATIC,
    LAYER_CONSOLE,
    LAYER_COMMANDBAR,
    LAYER_INFOPANEL,
    LAYER_INVENTORY,
    LAYER_MESSAGE,
    LAYER_QUESTDIALOG,
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
KNOWN_AS(jass_s, JASS);
KNOWN_AS(gcamerasetup_s, CAMERASETUP);
KNOWN_AS(gregion_s, REGION);
KNOWN_AS(gevent_s, EVENT);
KNOWN_AS(gtrigger_s, TRIGGER);
KNOWN_AS(gquest_s, QUEST);
KNOWN_AS(gquestitem_s, QUESTITEM);

typedef struct {
    BOOL (*on_entity_selected)(LPEDICT, LPEDICT);
    BOOL (*on_location_selected)(LPEDICT, LPCVECTOR2);
    void (*cmdbutton)(LPEDICT, DWORD);
} menu_t;

enum {
    AI_HOLD_FRAME = 1 << 0,
};

typedef enum {
    F_INT,
    F_FLOAT,
    F_LSTRING,            // string on disk, pointer in memory, TAG_LEVEL
    F_GSTRING,            // string on disk, pointer in memory, TAG_GAME
    F_VECTOR,
    F_ANGLEHACK,
    F_EDICT,            // index on disk, pointer in memory
    F_ITEM,                // index on disk, pointer in memory
    F_CLIENT,            // index on disk, pointer in memory
    F_FUNCTION,
    F_MMOVE,
    F_IGNORE
} fieldtype_t;

typedef struct {
    LPCSTR name;
    DWORD ofs;
    fieldtype_t type;
    DWORD flags;
} field_t;

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
    PLAYERSTATE_GAME_RESULT = 0,
    PLAYERSTATE_RESOURCE_GOLD = 1,
    PLAYERSTATE_RESOURCE_LUMBER = 2,
    PLAYERSTATE_RESOURCE_HERO_TOKENS = 3,
    PLAYERSTATE_RESOURCE_FOOD_CAP = 4,
    PLAYERSTATE_RESOURCE_FOOD_USED = 5,
    PLAYERSTATE_FOOD_CAP_CEILING = 6,
    PLAYERSTATE_GIVES_BOUNTY = 7,
    PLAYERSTATE_ALLIED_VICTORY = 8,
    PLAYERSTATE_PLACED = 9,
    PLAYERSTATE_OBSERVER_ON_DEATH = 10,
    PLAYERSTATE_OBSERVER = 11,
    PLAYERSTATE_UNFOLLOWABLE = 12,
    PLAYERSTATE_GOLD_UPKEEP_RATE = 13,
    PLAYERSTATE_LUMBER_UPKEEP_RATE = 14,
    PLAYERSTATE_GOLD_GATHERED = 15,
    PLAYERSTATE_LUMBER_GATHERED = 16,
} PLAYERSTATE;

typedef enum {
    PLAYERTEXT_SPEAKER,
    PLAYERTEXT_DIALOGUE,
} PLAYERTEXT;

typedef enum {
    ALLIANCE_PASSIVE = 0,
    ALLIANCE_HELP_REQUEST = 1,
    ALLIANCE_HELP_RESPONSE = 2,
    ALLIANCE_SHARED_XP = 3,
    ALLIANCE_SHARED_SPELLS = 4,
    ALLIANCE_SHARED_VISION = 5,
    ALLIANCE_SHARED_CONTROL = 6,
    ALLIANCE_SHARED_ADVANCED_CONTROL = 7,
    ALLIANCE_RESCUABLE = 8,
    ALLIANCE_SHARED_VISION_FORCED = 9,
} PLAYERALLIANCE;

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

typedef enum {
    EVENT_GAME_VICTORY = 0,
    EVENT_GAME_END_LEVEL = 1,
    EVENT_GAME_VARIABLE_LIMIT = 2,
    EVENT_GAME_STATE_LIMIT = 3,
    EVENT_GAME_TIMER_EXPIRED = 4,
    EVENT_GAME_ENTER_REGION = 5,
    EVENT_GAME_LEAVE_REGION = 6,
    EVENT_GAME_TRACKABLE_HIT = 7,
    EVENT_GAME_TRACKABLE_TRACK = 8,
    EVENT_GAME_SHOW_SKILL = 9,
    EVENT_GAME_BUILD_SUBMENU = 10,
    EVENT_PLAYER_STATE_LIMIT = 11,
    EVENT_PLAYER_ALLIANCE_CHANGED = 12,
    EVENT_PLAYER_DEFEAT = 13,
    EVENT_PLAYER_VICTORY = 14,
    EVENT_PLAYER_LEAVE = 15,
    EVENT_PLAYER_CHAT = 16,
    EVENT_PLAYER_END_CINEMATIC = 17,
    EVENT_PLAYER_UNIT_ATTACKED = 18,
    EVENT_PLAYER_UNIT_RESCUED = 19,
    EVENT_PLAYER_UNIT_DEATH = 20,
    EVENT_PLAYER_UNIT_DECAY = 21,
    EVENT_PLAYER_UNIT_DETECTED = 22,
    EVENT_PLAYER_UNIT_HIDDEN = 23,
    EVENT_PLAYER_UNIT_SELECTED = 24,
    EVENT_PLAYER_UNIT_DESELECTED = 25,
    EVENT_PLAYER_UNIT_CONSTRUCT_START = 26,
    EVENT_PLAYER_UNIT_CONSTRUCT_CANCEL = 27,
    EVENT_PLAYER_UNIT_CONSTRUCT_FINISH = 28,
    EVENT_PLAYER_UNIT_UPGRADE_START = 29,
    EVENT_PLAYER_UNIT_UPGRADE_CANCEL = 30,
    EVENT_PLAYER_UNIT_UPGRADE_FINISH = 31,
    EVENT_PLAYER_UNIT_TRAIN_START = 32,
    EVENT_PLAYER_UNIT_TRAIN_CANCEL = 33,
    EVENT_PLAYER_UNIT_TRAIN_FINISH = 34,
    EVENT_PLAYER_UNIT_RESEARCH_START = 35,
    EVENT_PLAYER_UNIT_RESEARCH_CANCEL = 36,
    EVENT_PLAYER_UNIT_RESEARCH_FINISH = 37,
    EVENT_PLAYER_UNIT_ISSUED_ORDER = 38,
    EVENT_PLAYER_UNIT_ISSUED_POINT_ORDER = 39,
    EVENT_PLAYER_UNIT_ISSUED_TARGET_ORDER = 40,
    EVENT_PLAYER_UNIT_ISSUED_UNIT_ORDER = 40,    // for compat
    EVENT_PLAYER_HERO_LEVEL = 41,
    EVENT_PLAYER_HERO_SKILL = 42,
    EVENT_PLAYER_HERO_REVIVABLE = 43,
    EVENT_PLAYER_HERO_REVIVE_START = 44,
    EVENT_PLAYER_HERO_REVIVE_CANCEL = 45,
    EVENT_PLAYER_HERO_REVIVE_FINISH = 46,
    EVENT_PLAYER_UNIT_SUMMON = 47,
    EVENT_PLAYER_UNIT_DROP_ITEM = 48,
    EVENT_PLAYER_UNIT_PICKUP_ITEM = 49,
    EVENT_PLAYER_UNIT_USE_ITEM = 50,
    EVENT_PLAYER_UNIT_LOADED = 51,
    EVENT_UNIT_DAMAGED = 52,
    EVENT_UNIT_DEATH = 53,
    EVENT_UNIT_DECAY = 54,
    EVENT_UNIT_DETECTED = 55,
    EVENT_UNIT_HIDDEN = 56,
    EVENT_UNIT_SELECTED = 57,
    EVENT_UNIT_DESELECTED = 58,
    EVENT_UNIT_STATE_LIMIT = 59,
    EVENT_UNIT_ACQUIRED_TARGET = 60,
    EVENT_UNIT_TARGET_IN_RANGE = 61,
    EVENT_UNIT_ATTACKED = 62,
    EVENT_UNIT_RESCUED = 63,
    EVENT_UNIT_CONSTRUCT_CANCEL = 64,
    EVENT_UNIT_CONSTRUCT_FINISH = 65,
    EVENT_UNIT_UPGRADE_START = 66,
    EVENT_UNIT_UPGRADE_CANCEL = 67,
    EVENT_UNIT_UPGRADE_FINISH = 68,
    EVENT_UNIT_TRAIN_START = 69,
    EVENT_UNIT_TRAIN_CANCEL = 70,
    EVENT_UNIT_TRAIN_FINISH = 71,
    EVENT_UNIT_RESEARCH_START = 72,
    EVENT_UNIT_RESEARCH_CANCEL = 73,
    EVENT_UNIT_RESEARCH_FINISH = 74,
    EVENT_UNIT_ISSUED_ORDER = 75,
    EVENT_UNIT_ISSUED_POINT_ORDER = 76,
    EVENT_UNIT_ISSUED_TARGET_ORDER = 77,
    EVENT_UNIT_HERO_LEVEL = 78,
    EVENT_UNIT_HERO_SKILL = 79,
    EVENT_UNIT_HERO_REVIVABLE = 80,
    EVENT_UNIT_HERO_REVIVE_START = 81,
    EVENT_UNIT_HERO_REVIVE_CANCEL = 82,
    EVENT_UNIT_HERO_REVIVE_FINISH = 83,
    EVENT_UNIT_SUMMON = 84,
    EVENT_UNIT_DROP_ITEM = 85,
    EVENT_UNIT_PICKUP_ITEM = 86,
    EVENT_UNIT_USE_ITEM = 87,
    EVENT_UNIT_LOADED = 88,
    EVENT_WIDGET_DEATH = 89,
    EVENT_DIALOG_BUTTON_CLICK = 90,
    EVENT_DIALOG_CLICK = 91,
    
    EVENT_UNIT_IN_RANGE,
} EVENTTYPE;

typedef enum {
    FILETEXTURE,
} HIGHLIGHTTYPE;

typedef enum {
    AUTOTRACK = 1,
    HIGHLIGHTONFOCUS = 2,
    HIGHLIGHTONMOUSEOVER = 4,
} CONTROLSTYLE;

typedef enum {
    LAYOUT_HORIZONTAL,
    LAYOUT_VERTICAL,
} LAYOUTDIRECTION;

typedef struct {
    UINAME frame;
    UINAME text;
} BUTTONTEXT;

typedef struct {
    uiFramePointPos_t targetPos;
    bool used;
    LPCFRAMEDEF relativeTo;
    FLOAT offset;
} FRAMEPOINT;

struct uiFrameDef_s {
    LPCFRAMEDEF Parent;
    FRAMETYPE Type;
    UINAME Name;
    UINAME TextStorage;
    UINAME OnClick;
    LPCSTR Text, Tooltip;
    FLOAT Width, Height;
    COLOR32 Color;
    ALPHAMODE AlphaMode;
    BOOL DecorateFileNames;
    BOOL inuse;
    BOOL AnyPointsSet;
    BOOL hidden;
    DWORD TextLength;
    DWORD Stat;
    struct {
        FRAMEPOINT x[FPP_COUNT];
        FRAMEPOINT y[FPP_COUNT];
    } Points;
    struct {
        DWORD Image;
        DWORD Image2;
        BOX2 TexCoord;
    } Texture;
    struct {
        BOOL TileBackground;
        DWORD Background;
        DWORD CornerFlags;// "UL|UR|BL|BR|T|L|B|R",
        FLOAT CornerSize;
        FLOAT BackgroundSize;
        FLOAT BackgroundInsets[4];// 0.01 0.01 0.01 0.01,
        DWORD EdgeFile;//  "EscMenuBorder",
        BOOL BlendAll;
    } Backdrop;
    LPCFRAMEDEF DialogBackdrop;
    struct {
        DWORD model;
    } Portrait;
    struct {
        UIFRAMEPOINT corner;
        FLOAT x, y;
    } Anchor;
    struct {
        UIFRAMEPOINT type;
        LPCFRAMEDEF relativeTo;
        UIFRAMEPOINT target;
        FLOAT x, y;
    } SetPoint;
    struct {
        UINAME Name;
        UINAME Unknown;
        UIFONTFLAGS FontFlags;
        FLOAT Size;
        DWORD Index;
        COLOR32 Color;
        COLOR32 HighlightColor;
        COLOR32 DisabledColor;
        COLOR32 ShadowColor;
        VECTOR2 ShadowOffset;
        struct {
            VECTOR2 Offset;
            uiFontJustificationH_t Horizontal;
            uiFontJustificationV_t Vertical;
        } Justification;
    } Font;
    struct {
        HIGHLIGHTTYPE Type;
        DWORD AlphaFile;
        ALPHAMODE AlphaMode;
    } Highlight;
    struct {
        VECTOR2 PushedTextOffset;
//        UINAME Text;
        UINAME NormalTexture;
        UINAME PushedTexture;
        UINAME DisabledTexture;
        UINAME UseHighlight;
        BUTTONTEXT NormalText;
        BUTTONTEXT DisabledText;
        BUTTONTEXT HighlightText;
    } Button;
    struct {
        DWORD Style;
        struct {
            UINAME Normal;
            UINAME Pushed;
            UINAME Disabled;
            UINAME MouseOver;
            UINAME DisabledPushed;
        } Backdrop;
    } Control;
    struct {
        FLOAT InitialValue;
        LAYOUTDIRECTION Layout;
        FLOAT MaxValue;
        FLOAT MinValue;
        FLOAT StepSize;
        UINAME ThumbButtonFrame;
    } Slider;
    struct {
        FLOAT Border;
        struct {
            UINAME Text;
            DWORD Value;
            FLOAT Height;
        } Item;
        COLOR32 TextHighlightColor;
    } Menu;
    struct {
        FLOAT BorderSize;
        COLOR32 CursorColor;
        COLOR32 HighlightColor;
        BOOL HighlightInitial;
        DWORD MaxChars;
        BOOL Focus;
        UINAME Text;
        COLOR32 TextColor;
        UINAME TextFrame;
        VECTOR2 TextOffset;
    } Edit;
    struct {
        UINAME ArrowFrame;
        UINAME MenuFrame;
        UINAME TitleFrame;
        FLOAT ButtonInset;
    } Popup;
    struct {
        FLOAT LineHeight;
        FLOAT LineGap;
        FLOAT Inset;
        DWORD MaxLines;
        UINAME ScrollBar;
    } TextArea;
    struct {
        UINAME CheckHighlight;
        UINAME DisabledCheckHighlight;
    } CheckBox;
    struct {
        LPCFRAMEDEF FirstItem;
        LPCFRAMEDEF BuildTimer;
        DWORD ItemOffset;
        DWORD NumQueue;
        uiBuildQueueItem_t Queue[MAX_BUILD_QUEUE];
    } BuildQueue;
    struct {
        DWORD HpBar;
        DWORD ManaBar;
        VECTOR2 Offset;
        DWORD NumColumns;
        DWORD NumItems;
        uiBuildQueueItem_t Items[MAX_SELECTED_ENTITIES];
    } Multiselect;
};

struct gregion_s {
    BOX2 rects[MAX_REGION_SIZE];
    DWORD num_rects;
};

struct gcamerasetup_s {
    FLOAT target_distance;
    FLOAT far_z;
//    FLOAT angle_of_attack;
    FLOAT fov;
//    FLOAT roll;
//    FLOAT rotations;
    FLOAT z_offset;
    VECTOR3 viewangles;
    VECTOR2 position;
};

struct client_s {
    PLAYER ps;
    LPCMAPPLAYER mapplayer;
    DWORD ping;
    BOOL no_control;
    menu_t menu;
    struct {
        CAMERASETUP state;
        CAMERASETUP old_state;
        DWORD start_time;
        DWORD end_time;
    } camera;
};

typedef struct {
    LPCSTR animation;
    void (*think)(LPEDICT);
    void (*endfunc)(LPEDICT);
    struct ability_s *ability;
} umove_t;

typedef struct ability_s {
    void (*init)(LPCSTR, struct ability_s *);
    void (*cmd)(LPEDICT);
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

typedef struct {
    float MoveSpeed;
    float FlyHeight;
//    float FlyRate;
    float TurnSpeed;
    float PropWindow;
    float AcquireRange;
} UNITINFO;

typedef struct {
    EVENTTYPE type;
    LPEDICT edict;
    LPEVENT responseTo;
} GAMEEVENT;

typedef struct {
    DWORD class_id;
    VECTOR2 origin;
} gitem_t;

struct gquestitem_s {
    LPSTR description;
    LPQUESTITEM next;
    BOOL completed;
};

struct gquest_s {
    LPSTR title;
    LPSTR description;
    LPSTR iconPath;
    LPQUESTITEM items;
    LPQUEST next;
    BOOL discovered;
    BOOL required;
    BOOL completed;
    BOOL failed;
    BOOL enabled;
};

typedef struct {
    struct { FLOAT day, night; } sight_radius;
} unitbalance_t;

struct edict_s {
    entityState_t s;
    LPGAMECLIENT client;
    pathTex_t *pathtex;
    FLOAT collision;
    BOX2 bounds;
    DWORD svflags;
    DWORD selected;
    DWORD areanum;
    LINK area;
    BOOL inuse;

    // keep above in sync with server.h
    DWORD class_id;
    DWORD variation;
    DWORD build_project;
    DWORD spawn_time;
    DWORD harvested_lumber;
    DWORD harvested_gold;
    DWORD heatmap2;
    DWORD peonsinside;
    DWORD aiflags;
    DWORD damage;
    DWORD resources;
    DWORD freetime;
    DWORD inventory[MAX_INVENTORY];
    FLOAT velocity;
    doodadHero_t hero;
    VECTOR2 old_origin;
    EDICTSTAT health;
    EDICTSTAT mana;
    MOVETYPE movetype;
    TARGTYPE targtype;
    LPEDICT goalentity;
    LPEDICT secondarygoal;
    LPEDICT owner;
    LPEDICT build;
    LPCANIMATION animation;
    unitbalance_t balance;
    umove_t *currentmove;
//    unitRace_t race;
    FLOAT wait;
    UNITINFO unitinfo;
    unitAttack_t attack1;
    unitAttack_t attack2;

    void (*stand)(LPEDICT);
    void (*birth)(LPEDICT);
    void (*prethink)(LPEDICT);
    void (*think)(LPEDICT);
    void (*pain)(LPEDICT);
    void (*die)(LPEDICT, LPEDICT);
};

struct game_locals {
    DWORD max_clients;
    DWORD num_abilities;
    LPGAMECLIENT clients;
    struct {
        sheetRow_t *abilities;
        sheetRow_t *items;
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

struct gevent_s {
    LPEVENT next;
    LPEDICT subject;
    EVENTTYPE type;
    LPTRIGGER trigger;
    REGION region;
    FLOAT range;
};

typedef struct {
    LPEVENT handlers;
    GAMEEVENT queue[MAX_EVENT_QUEUE];
    DWORD write, read;
} LEVELEVENTS;

struct level_locals {
    LPJASS vm;
    LPCMAPINFO mapinfo;
    LEVELEVENTS events;
    LPQUEST quests;
    USHORT alliances[MAX_PLAYERS][MAX_PLAYERS];
    DWORD framenum;
    DWORD time;
    BOOL started;
    BOOL scriptsStarted;
};

typedef struct sheetMetaData_s {
    LPCSTR id;
    LPCSTR field;
    LPCSTR slk;
    sheetRow_t *table;
} sheetMetaData_t;

typedef struct {
    LPCSTR name;
    void (*callback)(LPEDICT, LPCFRAMEDEF);
} uiTrigger_t;

// g_main.c
LPPLAYER G_GetPlayerByNumber(DWORD);
LPEDICT G_GetPlayerEntityByNumber(DWORD);
LPGAMECLIENT G_GetPlayerClientByNumber(DWORD);
TARGTYPE G_GetTargetType(LPCSTR);
LPCSTR G_LevelString(LPCSTR);
GAMEEVENT *G_PublishEvent(LPEDICT, EVENTTYPE);

// g_spawn.c
LPEDICT G_Spawn(void);
void SP_CallSpawn(LPEDICT);
void G_SpawnEntities(LPCMAPINFO, LPCDOODAD);
BOOL SP_FindEmptySpaceAround(LPEDICT, DWORD, LPVECTOR2, FLOAT *);
LPEDICT SP_SpawnAtLocation(DWORD, DWORD, LPCVECTOR2);

LPEDICT Waypoint_add(LPCVECTOR2);
void M_CheckGround (LPEDICT);
void monster_start(LPEDICT);

// g_ai.c
void ai_birth(LPEDICT);
void ai_stand(LPEDICT);
void ai_pain(LPEDICT);
void ai_idle(LPEDICT);
void M_RunWait(LPEDICT, void (*callback)(LPEDICT ));

// g_monster.c
void M_MoveInDirection(LPEDICT);
void M_ChangeAngle(LPEDICT);
BOOL M_CheckAttack(LPEDICT);
void M_SetAnimation(LPEDICT, LPCSTR);
void M_SetMove(LPEDICT, umove_t *);
umove_t const *M_GetCurrentMove(LPCEDICT);
FLOAT M_DistanceToGoal(LPEDICT);
FLOAT M_MoveDistance(LPEDICT);
DWORD M_RefreshHeatmap(LPEDICT);
BOOL M_IsDead(LPEDICT);
void SP_SpawnUnit(LPEDICT);
void SP_TrainUnit(LPEDICT, DWORD);
BOOL player_pay(LPPLAYER, DWORD);
BYTE compress_stat(EDICTSTAT const *);

// g_pathing.c
pathTex_t *LoadTGA(BYTE const*, size_t);

// g_move.c
BOOL SV_CloseEnough(LPEDICT, LPCEDICT, FLOAT);

// g_phys.c
void G_RunEntity(LPEDICT);
void G_SolveCollisions(void);
BOOL M_CheckCollision(LPCVECTOR2, FLOAT);

// g_abilities.c
ability_t const *FindAbilityByClassname(LPCSTR);
ability_t const *GetAbilityByIndex(DWORD);
DWORD FindAbilityIndex(LPCSTR);
void InitAbilities(void);
void SetAbilityNames(void);

// g_config.c
LPCSTR FindConfigValue(LPCSTR, LPCSTR);
LPCSTR GetClassName(DWORD);

// p_hud.c
LPEDICT G_GetMainSelectedUnit(LPGAMECLIENT);
void Get_Commands_f(LPEDICT);
void Get_Portrait_f(LPEDICT);
void UI_AddCancelButton(LPEDICT);
void UI_AddCommandButton(LPCSTR);
void UI_ShowInterface(LPEDICT, BOOL, FLOAT);
void UI_ShowText(LPEDICT, LPCVECTOR2, LPCSTR, FLOAT);
LPCSTR GetBuildCommand(unitRace_t);

// p_fdf.c
void UI_PrintClasses(void);
void UI_ClearTemplates(void);
void UI_ParseFDF(LPCSTR);
void UI_ParseFDF_Buffer(LPCSTR, LPSTR);
void UI_SetAllPoints(LPFRAMEDEF);
void UI_SetParent(LPFRAMEDEF, LPCFRAMEDEF);
void UI_SetText(LPFRAMEDEF, LPCSTR, ...);
void UI_SetOnClick(LPFRAMEDEF, LPCSTR, ...);
void UI_SetTextPointer(LPFRAMEDEF, LPCSTR);
void UI_SetSize(LPFRAMEDEF, FLOAT, FLOAT);
void UI_SetTexture(LPFRAMEDEF, LPCSTR, BOOL);
void UI_SetTexture2(LPFRAMEDEF, LPCSTR, BOOL);
void UI_WriteLayout(LPEDICT, LPCFRAMEDEF, DWORD);
void UI_WriteStart(DWORD);
void UI_WriteWithTriggers(LPEDICT, LPCFRAMEDEF, DWORD, uiTrigger_t const *);
void UI_SetPoint(LPFRAMEDEF, UIFRAMEPOINT, LPCFRAMEDEF, UIFRAMEPOINT, FLOAT, FLOAT);
void UI_InitFrame(LPFRAMEDEF, FRAMETYPE);
void UI_SetHidden(LPFRAMEDEF, BOOL);
DWORD UI_FindFrameNumber(LPCSTR);
DWORD UI_LoadTexture(LPCSTR, BOOL);
LPCSTR UI_GetString(LPCSTR);
LPFRAMEDEF UI_Spawn(FRAMETYPE, LPFRAMEDEF);
LPFRAMEDEF UI_FindFrame(LPCSTR);
LPFRAMEDEF UI_FindChildFrame(LPFRAMEDEF, LPCSTR);

LPCSTR Theme_String(LPCSTR, LPCSTR);
FLOAT Theme_Float(LPCSTR, LPCSTR);

// ui_write.c
void UI_WriteFrame(LPCFRAMEDEF);
void UI_WriteFrameWithChildren(LPCFRAMEDEF, LPCFRAMEDEF);
void UI_WriteFrameWithChildrenWithTriggers(LPEDICT, LPCFRAMEDEF, LPCFRAMEDEF, uiTrigger_t const *);

// g_metadata.c
LPCSTR UnitStringField(sheetMetaData_t *, DWORD, LPCSTR);
LONG UnitIntegerField(sheetMetaData_t *, DWORD, LPCSTR);
BOOL UnitBooleanField(sheetMetaData_t *, DWORD, LPCSTR);
FLOAT UnitRealField(sheetMetaData_t *, DWORD, LPCSTR);

void InitUnitData(void);
void ShutdownUnitData(void);

// g_command.c
void G_SelectEntity(LPGAMECLIENT, LPEDICT);
void G_DeselectEntity(LPGAMECLIENT, LPEDICT);
BOOL G_IsEntitySelected(LPGAMECLIENT, LPEDICT);
void G_ClientCommand(LPEDICT, DWORD, LPCSTR[]);
void G_ClientPanCamera(LPEDICT, LPVECTOR2);

//  s_skills.c
FLOAT AB_Number(LPCSTR, LPCSTR);
DWORD GetAbilityIndex(ability_t const *);

// g_combat.c
void T_Damage(LPEDICT, LPEDICT, int);

// g_utils.c
void G_FreeEdict(LPEDICT);
LPEVENT G_MakeEvent(EVENTTYPE);
LPQUEST G_MakeQuest(void);
BOOL G_RegionContains(LPCREGION, LPCVECTOR2);
void G_RemoveQuest(LPQUEST);
void G_SetPlayerAlliance(LPCPLAYER, LPCPLAYER, PLAYERALLIANCE, BOOL);
BOOL G_GetPlayerAlliance(LPCPLAYER, LPCPLAYER, PLAYERALLIANCE);

// m_unit.c
BOOL unit_issueorder(LPEDICT, LPCSTR, LPCVECTOR2);
BOOL unit_issueimmediateorder(LPEDICT, LPCSTR);
LPEDICT unit_createorfind(DWORD, DWORD, LPCVECTOR2, FLOAT);
BOOL unit_additemtoslot(LPEDICT, DWORD, DWORD);
BOOL unit_additem(LPEDICT, DWORD);

// p_jass.c
LPJASS jass_newstate(void);
void jass_close(LPJASS);
BOOL jass_dofile(LPJASS, LPCSTR);
BOOL jass_dofilenative(LPJASS, LPCSTR);
void jass_callbyname(LPJASS, LPCSTR, BOOL);
BOOL jass_dobuffer(LPJASS, LPSTR);

// g_events.c
void G_RunEntities(void);
void G_RunEvents(void);

// g_items.c
void SP_SpawnItem(LPEDICT);

void *find_in_array(void *array, long, LPCSTR);

// ui_init
void UI_Init(void);

// globals
extern struct game_locals game;
extern struct game_export globals;
extern struct game_import gi;
extern struct level_locals level;
extern struct edict_s *g_edicts;

extern sheetMetaData_t UnitsMetaData[];
extern sheetMetaData_t DestructableMetaData[];
extern sheetMetaData_t DoodadsMetaData[];
extern sheetMetaData_t ItemsMetaData[];

#endif
