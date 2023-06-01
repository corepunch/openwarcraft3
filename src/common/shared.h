#ifndef shared_h
#define shared_h

#include <string.h>
#include <stdio.h>
#include <assert.h>

#include "../cmath3/cmath3.h"

#define MAX_PATHLEN 256
#define TOKEN_LEN 1024
#define FRAMETIME 100
#define MIN(x, y) (((x)<(y))?(x):(y))
#define MAX(x, y) (((x)>(y))?(x):(y))

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

#define MAX_COMMANDS 12

#define MAX_PACKET_ENTITIES 64
#define MAX_CLIENTS 256
#define MAX_LIGHTSTYLES 256
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
#define CS_LIGHTS (CS_IMAGES+MAX_IMAGES)
#define CS_ITEMS (CS_LIGHTS+MAX_LIGHTSTYLES)
#define CS_PLAYERSKINS (CS_ITEMS+MAX_ITEMS)
#define CS_GENERAL (CS_PLAYERSKINS+MAX_CLIENTS)
#define MAX_CONFIGSTRINGS (CS_GENERAL+MAX_GENERAL)

// Typedefs for ANSI C
typedef unsigned char  BYTE;
typedef unsigned short USHORT;
typedef int            LONG;
typedef unsigned int   DWORD;
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

KNOWN_AS(SheetLayout, SHEETLAYOUT);
KNOWN_AS(SheetCell, SHEETCELL);
KNOWN_AS(Doodad, DOODAD);
KNOWN_AS(vector3, VECTOR3);
KNOWN_AS(color32, COLOR32);
KNOWN_AS(size2, SIZE2);

typedef enum {
    MULTICAST_ALL,
    MULTICAST_PHS,
    MULTICAST_PVS,
    MULTICAST_ALL_R,
    MULTICAST_PHS_R,
    MULTICAST_PVS_R
} multicast_t;

typedef struct entityState_s {
    DWORD number; // edict index
    union {
        VECTOR3 origin;
        struct { VECTOR2 origin2; float z; };
    };
    float angle;
    float scale;
    DWORD model;
    DWORD image;
    DWORD sound;
    DWORD frame;
    DWORD event;
    DWORD player;
    BYTE commands[MAX_COMMANDS];
} entityState_t;

typedef struct {
    DWORD firstframe;
    DWORD lastframe;
    DWORD framerate;
    float movespeed;
} animationInfo_t;

struct size2 {
    DWORD width;
    DWORD height;
};

enum SheetType {
    ST_ID,
    ST_INT,
    ST_FLOAT,
    ST_STRING,
};

struct SheetLayout {
    const LPSTR column;
    enum SheetType type;
    HANDLE fofs;
};

struct SheetCell {
    DWORD column;
    DWORD row;
    LPSTR text;
    LPSHEETCELL next;
};

typedef struct configValue_s {
    char Section[64];
    char Name[64];
    LPSTR Value;
    struct configValue_s *next;
} configValue_t;

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
