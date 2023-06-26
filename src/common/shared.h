#ifndef shared_h
#define shared_h

//#define DEBUG_PATHFINDING

#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <stdarg.h>

#include "../cmath3/cmath3.h"

#define TMP_MAP "/tmp/map.w3m"
#define MAX_PATHLEN 256
#define TOKEN_LEN 1024
#define FRAMETIME 100
#define MIN(x, y) (((x)<(y))?(x):(y))
#define MAX(x, y) (((x)>(y))?(x):(y))

#define IS_FOURCC(STRING) (STRING && strlen(STRING) == 4)

#ifndef __cplusplus
  #define bool char
  #define true 1
  #define false 0
#endif

#define KNOWN_AS(STRUCT, TYPE) \
typedef struct STRUCT TYPE; \
typedef struct STRUCT *LP##TYPE; \
typedef struct STRUCT const *LPC##TYPE;

#define FOR_LOOP(property, max) \
for (DWORD property = 0, end = max; property < end; ++property)

#define PrintTag(tag) \
do { \
LPSTR ch = (char*)&tag; \
printf("%c%c%c%c\n", ch[0], ch[1], ch[2], ch[3]); \
} while(false);

#define FOR_EACH_LIST(type, property, list) \
for (type *property = list, *next = list ? (list)->next : NULL; \
property; \
property = next, next = next ? next->next : NULL)

#define ADD_TO_LIST(VAR, LIST) VAR->next = LIST; LIST = VAR;

#define FOR_EACH(type, property, array, num) \
for (type *property = array; property - array < num; property++)

#define MAKEFOURCC(ch0, ch1, ch2, ch3) ((int)(char)(ch0) | ((int)(char)(ch1) << 8) | ((int)(char)(ch2) << 16) | ((int)(char)(ch3) << 24))

#define FOFS(type, x) (HANDLE)&(((struct type *)NULL)->x)

#define SAFE_DELETE(x, func) if (x) { func(x); (x) = NULL; }

#define PUSH_BACK(TYPE, VAR, LIST) \
if (LIST) { \
    TYPE *last##TYPE = LIST; \
    while (last##TYPE->next) last##TYPE = last##TYPE->next; \
    last##TYPE->next = VAR; \
} else { \
    LIST = VAR; \
}

#define PARSE_LIST(LIST, ITEM, PARSER) \
parser_t parser = { 0 }; \
parser.tok = parser.token; \
parser.str = LIST; \
parser.comma_space = true; \
for (LPCSTR ITEM = PARSER(&parser); ITEM; ITEM = PARSER(&parser))

enum {
    RF_SELECTED = 1 << 0,
    RF_HAS_LUMBER = 1 << 1,
    RF_HAS_GOLD = 1 << 2,
    RF_HIDDEN = 1 << 3,
    RF_NO_UBERSPLAT = 1 << 4,
    RF_NO_FOGOFWAR = 1 << 5,
};

#define MAX_COMMANDS 12

#define STAT_GOLD 1
#define STAT_LUMBER 2
#define STAT_FOOD 3
#define STAT_FOOD_MADE 4
#define STAT_FOOD_USED 5

#define MAX_STATS 32

#define MAX_PACKET_ENTITIES 64
#define MAX_CLIENTS 256
#define MAX_FONTSTYLES 256
#define MAX_MODELS 256
#define MAX_SOUNDS 256
#define MAX_IMAGES 256
#define MAX_ITEMS 256
#define MAX_GENERAL (MAX_CLIENTS*2)

#define CS_NAME 0
#define CS_CDTRACK 1
#define CS_SKY 2
#define CS_SKYAXIS 3        // %f %f %f format
#define CS_SKYROTATE 4
#define CS_STATUSBAR 5        // display program string
#define CS_AIRACCEL 29        // air acceleration control
#define CS_MAXCLIENTS 30
#define CS_MAPCHECKSUM 31        // for catching cheater maps
#define CS_MODELS 32
#define CS_SOUNDS (CS_MODELS+MAX_MODELS)
#define CS_IMAGES (CS_SOUNDS+MAX_SOUNDS)
#define CS_FONTS (CS_IMAGES+MAX_IMAGES)
#define CS_ITEMS (CS_FONTS+MAX_FONTSTYLES)
#define CS_PLAYERSKINS (CS_ITEMS+MAX_ITEMS)
#define CS_GENERAL (CS_PLAYERSKINS+MAX_CLIENTS)
#define MAX_CONFIGSTRINGS (CS_GENERAL+MAX_GENERAL)

// Typedefs for ANSI C
typedef unsigned char  BYTE;
typedef unsigned short USHORT;
typedef int            LONG;
typedef unsigned int   DWORD;
typedef unsigned short WORD;
typedef unsigned long  DWORD_PTR;
typedef long           LONG_PTR;
typedef long           INT_PTR;
typedef long long      LONGLONG;
typedef unsigned long long ULONGLONG;
typedef void         * HANDLE;
typedef void         * LPOVERLAPPED; // Unsupported on Linux and Mac
typedef char           TCHAR;
typedef unsigned int   LCID;
typedef LONG         * PLONG;
typedef DWORD        * LPDWORD;
typedef BYTE         * LPBYTE;
typedef const char   * LPCTSTR;
typedef const char   * LPCSTR;
typedef char         * LPTSTR;
typedef char         * LPSTR;
typedef unsigned int   handle_t;
typedef char           PATHSTR[MAX_PATHLEN];
typedef void const   * LPCVOID;
typedef struct color { float r, g, b, a; } color_t;
typedef struct color32 { BYTE r, g, b, a; } color32_t;
typedef struct bounds { float min, max; } bounds_t;
typedef struct edges { float left, top, right, bottom; } edges_t;
typedef struct transform2 { VECTOR2 translation, scale; float rotation; } transform2_t;
typedef struct transform3 { VECTOR3 translation, rotation, scale; } transform3_t;
typedef char uiName_t[80];

KNOWN_AS(SheetLayout, SHEETLAYOUT);
KNOWN_AS(SheetCell, SHEET);
KNOWN_AS(Doodad, DOODAD);
KNOWN_AS(vector3, VECTOR3);
KNOWN_AS(color32, COLOR32);

typedef enum {
    MULTICAST_ALL,
    MULTICAST_PHS,
    MULTICAST_PVS,
    MULTICAST_ALL_R,
    MULTICAST_PHS_R,
    MULTICAST_PVS_R
} multicast_t;

typedef struct {
    DWORD number;
    VECTOR3 viewangles;
    VECTOR3 origin;
    float distance;
    DWORD fov;
    int rdflags;
    short stats[MAX_STATS];
} playerState_t;

typedef struct entityState_s {
    DWORD number; // edict index
    union {
        VECTOR3 origin;
        struct { VECTOR2 origin2; float z; };
    };
    float angle;
    float scale;
    float radius;
    float health;
    DWORD model;
    DWORD image;
    DWORD sound;
    DWORD frame;
    DWORD event;
    DWORD player;
    DWORD flags;
    DWORD renderfx;
    DWORD splat;
} entityState_t;

typedef struct {
    char name[80];
    DWORD interval[2];
    float movespeed;     // movement speed of the entity while playing this animation
    DWORD flags;      // &1: non looping
    float rarity;
    int syncpoint;
    float radius;
    VECTOR3 min;
    VECTOR3 max;
} animation_t;

typedef struct {
    DWORD width;
    DWORD height;
} size2_t;

typedef enum {
    TE_GUNSHOT,
    TE_BLOOD,
    TE_MOVE_CONFIRMATION,
} tempEvent_t;

typedef enum {
    FT_NONE,
    FT_BACKDROP,
    FT_BUTTON,
    FT_CHATDISPLAY,
    FT_CHECKBOX,
    FT_CONTROL,
    FT_DIALOG,
    FT_EDITBOX,
    FT_FRAME,
    FT_GLUEBUTTON,
    FT_GLUECHECKBOX,
    FT_GLUEEDITBOX,
    FT_GLUEPOPUPMENU,
    FT_GLUETEXTBUTTON,
    FT_HIGHLIGHT,
    FT_LISTBOX,
    FT_MENU,
    FT_MODEL,
    FT_POPUPMENU,
    FT_SCROLLBAR,
    FT_SIMPLEBUTTON,
    FT_SIMPLECHECKBOX,
    FT_SIMPLEFRAME,
    FT_SIMPLESTATUSBAR,
    FT_SLASHCHATBOX,
    FT_SLIDER,
    FT_SPRITE,
    FT_TEXT,
    FT_TEXTAREA,
    FT_TEXTBUTTON,
    FT_TIMERTEXT,
    FT_TEXTURE,
    FT_STRING,
    FT_LAYER,
    FT_SCREEN,
    FT_COMMANDBUTTON,
    FT_PORTRAIT,
} uiFrameType_t;

typedef enum {
    AM_ALPHAKEY,
} uiAlphaMode_t;

typedef enum {
    FPP_MIN,
    FPP_MID,
    FPP_MAX,
    FPP_COUNT,
} uiFramePointPos_t;

typedef enum {
    FONT_JUSTIFYCENTER,
    FONT_JUSTIFYLEFT,
    FONT_JUSTIFYRIGHT,
} uiFontJustificationH_t;

typedef enum {
    FONT_JUSTIFYMIDDLE,
    FONT_JUSTIFYTOP,
    FONT_JUSTIFYBOTTOM,
} uiFontJustificationV_t;

typedef struct { // serialized as 4 bytes
    uiFramePointPos_t targetPos: 7;
    bool used: 1;
    uint8_t relativeTo: 8;
    int16_t offset: 16;
} uiFramePoint_t;

typedef uiFramePoint_t uiFramePoints_t[FPP_COUNT];

typedef struct {
    DWORD number;
    DWORD parent;
    struct { uiFramePoints_t x, y; } points;
    struct { uint16_t width, height; } size;
    
    // Texture
    struct {
        DWORD index;
        uint8_t coord[4];
    } tex;

    // String
    union {
        struct {
            uiFrameType_t type: 8;
            uiAlphaMode_t alphaMode: 2;
            uiFontJustificationH_t textalignx: 2;
            uiFontJustificationV_t textaligny: 2;
        } flags;
        DWORD flagsvalue;
    };
    
    struct {
        DWORD index;
    } font;
    DWORD textLength;
    DWORD stat;
    uiName_t text;

    // Command Button
    DWORD code;
} uiFrame_t;

typedef struct sheetField_s {
    LPCSTR name, value;
    struct sheetField_s *next;
} sheetField_t;

typedef struct sheetRow_s {
    LPCSTR name;
    struct sheetField_s *fields;
    struct sheetRow_s *next;
} sheetRow_t;

typedef struct {
    LPSTR tok;
    LPCSTR str;
    bool reading_string;
    bool error;
    bool comma_space;
    bool equals_space;
    char token[TOKEN_LEN];
} parser_t;

typedef struct {
    DWORD itemID;
    int chanceToDrop;
} droppableItem_t;

typedef struct {
    int num_droppableItems;
    droppableItem_t *droppableItems;
} droppableItemSet_t;

typedef struct {
    int x, y;
} point2_t;

typedef struct {
    DWORD level; // (set to 1 for non hero units and items)
    DWORD str;
    DWORD agi;
    DWORD intel;
} doodadHero_t;

typedef struct {
    DWORD slot;
    DWORD itemID;
} inventoryItem_t;

typedef struct {
    DWORD abilityID;
    DWORD active;
    DWORD level;
} modifiedAbility_t;

typedef struct {
    WORD width;
    WORD height;
    COLOR32 map[];
} pathTex_t;

struct Doodad {
    DWORD doodID;
    DWORD variation;
    VECTOR3 position;
    float angle;
    VECTOR3 scale;
    BYTE flags;
    DWORD player;
    BYTE treeLife; // integer stored in %, 100% is 0x64, 170% is 0xAA for example
    BYTE unknown1;
    BYTE unknown2;
    DWORD hitPoints; // (-1 = use default)
    DWORD manaPoints; // (-1 = use default, 0 = unit doesn't have mana)
    DWORD droppedItemSetPtr;
    DWORD num_droppedItemSets;
    DWORD goldAmount; // (default = 12500)
    float targetAcquisition; // (-1 = normal, -2 = camp)
    doodadHero_t hero;
    DWORD num_inventoryItems;
    DWORD num_modifiedAbilities;
    droppableItemSet_t *droppableItemSets;
    inventoryItem_t *inventoryItems;
    modifiedAbility_t *modifiedAbilities;
    DWORD randomUnitFlag; // "r" (for uDNR units and iDNR items)
    DWORD levelOfRandomItem; //    byte[3]: level of the random unit/item,-1 = any (this is actually interpreted as a 24-bit number)
    //    byte: item class of the random item, 0 = any, 1 = permanent ... (this is 0 for units)
    DWORD randomUnitGroupNumber; //    DWORD: unit group number (which group from the global table)
    DWORD randomUnitPositionNumber; //    DWORD: position number (which column of this group)
    DWORD num_diffAvailUnits;
    droppableItem_t *diffAvailUnits;
    COLOR32 color;
    DWORD waygate;
    DWORD unitID;
    struct Doodad *next;
};

//#define NULL 0

#endif
