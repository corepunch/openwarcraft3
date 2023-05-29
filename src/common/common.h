#ifndef __common_h__
#define __common_h__

#include "shared.h"
#include "net.h"
#include "../cmath3/cmath3.h"

//#define DEBUG_PATHFINDING

#define TMP_MAP "/tmp/map.w3m"
#define MAP_VERTEX_SIZE 7
#define MAX_SHEET_LINE 1024
#define MAX_PATHLEN 256
#define TILESIZE 128
#define MAX_COMMAND_ENTITIES 64
#define HEIGHT_COR (TILESIZE * 2 + 5)
#define MIN(x, y) (((x)<(y))?(x):(y))
#define MAX(x, y) (((x)>(y))?(x):(y))
#define DECODE_HEIGHT(x) (((x) - 0x2000) / 4)
#define MAKEFOURCC(ch0, ch1, ch2, ch3) ((int)(char)(ch0) | ((int)(char)(ch1) << 8) | ((int)(char)(ch2) << 16) | ((int)(char)(ch3) << 24))
#define FOFS(type, x) (HANDLE)&(((struct type *)NULL)->x)

#define PLAYER_START_ID MAKEFOURCC('P','l','s','t')

#define FRAMETIME 100
#define UPDATE_BACKUP 16
#define UPDATE_MASK (UPDATE_BACKUP-1)

#define MAX_PACKET_ENTITIES 64
#define MAX_CLIENTS 16
#define MAX_LIGHTSTYLES 256
#define MAX_MODELS 256
#define MAX_SOUNDS 256
#define MAX_IMAGES 256
#define MAX_ITEMS 256
#define MAX_GENERAL (MAX_CLIENTS*2)

#define U_REMOVE 15

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
#define SAFE_DELETE(x, func) if (x) { func(x); (x) = NULL; }

#define SFileReadArray(file, object, variable, elemsize, alloc) \
SFileReadFile(file, &object->num_##variable, 4, NULL, NULL); \
if (object->num_##variable > 0) {object->variable = alloc(object->num_##variable * elemsize); \
SFileReadFile(file, object->variable, object->num_##variable * elemsize, NULL, NULL); }

#define FOR_LOOP(property, max) for (DWORD property = 0, end = max; property < end; ++property)
#define PrintTag(tag)do { LPSTR ch = (char*)&tag; printf("%c%c%c%c\n", ch[0], ch[1], ch[2], ch[3]); } while(false);

#define FOR_EACH_LIST(type, property, list) \
for (type *property = list, *next = list ? (list)->next : NULL; \
property; \
property = next, next = next ? next->next : NULL)

#define ADD_TO_LIST(VAR, LIST) VAR->next = LIST; LIST = VAR;

#define FOR_EACH(type, property, array, num) \
for (type *property = array; property - array < num; property++)

#define KNOWN_AS(STRUCT, TYPE) \
typedef struct STRUCT TYPE; \
typedef struct STRUCT *LP##TYPE; \
typedef struct STRUCT const *LPC##TYPE;

#define FIND_SHEET_ENTRY(SHEET, VAR, ID_FIELD, VALUE) \
SHEET; for (; VAR->ID_FIELD != VALUE && VAR->ID_FIELD; VAR++); \
if (VAR->ID_FIELD == 0) VAR = NULL;

// server to client
enum svc_ops {
    svc_bad,
//    // these ops are known to the game dll
//    svc_muzzleflash,
//    svc_muzzleflash2,
//    svc_temp_entity,
//    svc_layout,
    svc_playerinfo,
//
//    // the rest are private to the client and server
//    svc_nop,
//    svc_disconnect,
//    svc_reconnect,
//    svc_sound,                    // <see code>
//    svc_print,                    // [byte] id [string] null terminated string
//    svc_stufftext,                // [string] stuffed into client's console buffer, should be \n terminated
//    svc_serverdata,                // [long] protocol ...
    svc_configstring,            // [short] [string]
    svc_spawnbaseline,
//    svc_centerprint,            // [string] to put in center of the screen
//    svc_download,                // [short] size [size bytes]
//    svc_playerinfo,                // variable
    svc_packetentities,            // [...]
//    svc_deltapacketentities,    // [...]
    svc_frame
};

// client to server
enum clc_ops {
    clc_bad,
//    clc_nop,
    clc_move,
    clc_command,
//    clc_userinfo,            // [[userinfo string]
//    clc_stringcmd            // [string] message
};

enum clientcommand {
    CMD_NO_COMMAND,
    CMD_MOVE,
    CMD_ATTACK,
};

typedef struct {
    enum clientcommand cmd;
    DWORD targetentity;
    VECTOR2 location;
    DWORD num_entities;
    DWORD entities[MAX_COMMAND_ENTITIES];
} clientMessage_t;

typedef enum t_attrib_id {
    attrib_position,
    attrib_color,
    attrib_texcoord,
    attrib_texcoord2,
    attrib_normal,
    attrib_skin1,
    attrib_skin2,
    attrib_boneWeight1,
    attrib_boneWeight2,
} t_attrib_id;

typedef struct {
    int x;
    int y;
} point2_t;

struct texture;

typedef struct {
    unsigned int modeltype;
    struct mdxModel_s *mdx;
} model_t;

KNOWN_AS(mdx, MODEL);
KNOWN_AS(texture, TEXTURE);
KNOWN_AS(War3MapVertex, WAR3MAPVERTEX);
KNOWN_AS(SheetLayout, SHEETLAYOUT);
KNOWN_AS(SheetCell, SHEETCELL);
KNOWN_AS(war3map, WAR3MAP);
KNOWN_AS(Doodad, DOODAD);
KNOWN_AS(edict, EDICT);
KNOWN_AS(vector3, VECTOR3);
KNOWN_AS(color32, COLOR32);
KNOWN_AS(size2, SIZE2);
KNOWN_AS(rect, RECT);

KNOWN_AS(TerrainInfo, TERRAININFO);
KNOWN_AS(CliffInfo, CLIFFINFO);
KNOWN_AS(AnimLookup, ANIMLOOKUP);

typedef unsigned int handle_t;
typedef char PATHSTR[MAX_PATHLEN];
typedef void const *LPCVOID;

typedef struct color { float r, g, b, a; } color_t;
typedef struct color32 { BYTE r, g, b, a; } color32_t;
typedef struct bounds { float min, max; } bounds_t;
typedef struct rect { float x, y, width, height; } rect_t;
typedef struct edges { float left, top, right, bottom; } edges_t;
typedef struct transform2 { VECTOR2 translation, scale; float rotation; } transform2_t;
typedef struct transform3 { VECTOR3 translation, rotation, scale; } transform3_t;

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

#include "cmodel.h"

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

#define TOKEN_LEN 1024

typedef struct configValue_s {
    char Section[64];
    char Name[64];
    LPSTR Value;
    struct configValue_s *next;
} configValue_t;

typedef struct {
    LPSTR tok;
    LPCSTR str;
    bool error;
    bool comma_space;
    char token[TOKEN_LEN];
} parser_t;

HANDLE FS_ParseSheet(LPCSTR fileName, LPCSHEETLAYOUT layout, DWORD elementSize);
void LoadMap(LPCSTR pFilename);

void FS_Init(void);
void FS_Shutdown(void);

void Com_Quit(void);
void Sys_Quit(void);

HANDLE FS_OpenFile(LPCSTR fileName);
bool FS_ExtractFile(LPCSTR toExtract, LPCSTR extracted);
LPSHEETCELL FS_ReadSheet(LPCSTR fileName);

void CL_Init(void);
void CL_Frame(DWORD msec);
void CL_Shutdown(void);

void SV_Init(void);
void SV_Frame(DWORD msec);
void SV_Shutdown(void);

HANDLE MemAlloc(long size);
void MemFree(HANDLE mem);

void Sys_MkDir(LPCSTR directory);

handle_t CM_BuildHeatmap(LPCVECTOR2 target);

// INI
LPSTR ParserGetToken(parser_t *p);
bool ParserDone(parser_t *p);
configValue_t *FS_ParseConfig(LPCSTR filename);
LPCSTR INI_FindValue(configValue_t *config, LPCSTR sectionName, LPCSTR valueName);
LPSTR FS_ReadFileIntoString(LPCSTR fileName);

#endif
