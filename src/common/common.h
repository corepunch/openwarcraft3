#ifndef __common_h__
#define __common_h__

#include "shared.h"
#include "net.h"
#include "../cmath3/cmath3.h"

#define TMP_MAP "/tmp/map.w3m"
#define MAP_VERTEX_SIZE 7
#define MAX_SHEET_LINE 1024
#define MAX_PATHLEN 256
#define TILESIZE 128
#define HEIGHT_COR (TILESIZE * 2 + 5)
#define MIN(x, y) (((x)<(y))?(x):(y))
#define MAX(x, y) (((x)>(y))?(x):(y))
#define DECODE_HEIGHT(x) (((x) - 0x2000) / 4)
#define MAKEFOURCC(ch0, ch1, ch2, ch3) ((int)(char)(ch0) | ((int)(char)(ch1) << 8) | ((int)(char)(ch2) << 16) | ((int)(char)(ch3) << 24))
#define FOFS(type, x) (HANDLE)&(((struct type *)NULL)->x)

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

#define SFileReadArray(hFile, object, variable, elemsize, alloc) \
SFileReadFile(hFile, &object->num##variable, 4, NULL, NULL); \
if (object->num##variable > 0) {object->lp##variable = alloc(object->num##variable * elemsize); \
SFileReadFile(hFile, object->lp##variable, object->num##variable * elemsize, NULL, NULL); }

#define FOR_LOOP(property, max) for (DWORD property = 0, end = max; property < end; ++property)
#define PrintTag(tag)do { LPSTR ch = (char*)&tag; printf("%c%c%c%c\n", ch[0], ch[1], ch[2], ch[3]); } while(false);

#define FOR_EACH_LIST(type, property, list) \
for (type *property = list, *next = list ? (list)->lpNext : NULL; \
property; \
property = next, next = next ? next->lpNext : NULL)

#define FOR_EACH(type, property, array, num) \
for (type *property = array; property - array < num; property++)

#define KNOWN_AS(STRUCT, TYPE) \
typedef struct STRUCT TYPE; \
typedef struct STRUCT *LP##TYPE; \
typedef struct STRUCT const *LPC##TYPE;

#define FIND_SHEET_ENTRY(SHEET, VAR, ID_FIELD, VALUE) \
SHEET; for (; VAR->ID_FIELD != VALUE && VAR->ID_FIELD; VAR++); \
if (VAR->ID_FIELD == 0) VAR = NULL;

enum {
    U_ORIGIN1,
    U_ORIGIN2,
    U_ORIGIN3,
    U_ANGLE,
    U_SCALE,
    U_REMOVE,
    U_FRAME,
    U_MODEL,
    U_IMAGE,
    U_TEAM,
};

// server to client
enum svc_ops {
    svc_bad,
//    // these ops are known to the game dll
//    svc_muzzleflash,
//    svc_muzzleflash2,
//    svc_temp_entity,
//    svc_layout,
//    svc_inventory,
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
//    clc_userinfo,            // [[userinfo string]
//    clc_stringcmd            // [string] message
};

enum clientcommand {
    CMD_NO_COMMAND,
    CMD_MOVE,
    CMD_ATTACK,
};

struct client_message {
    enum clientcommand cmd;
    DWORD entity;
    DWORD targetentity;
    VECTOR2 location;
};

typedef enum t_attrib_id {
    attrib_position,
    attrib_color,
    attrib_texcoord,
    attrib_texcoord2,
    attrib_normal,
    attrib_skin,
    attrib_boneWeight,
} t_attrib_id;

struct tModel;
struct texture;

KNOWN_AS(tModel, MODEL);
KNOWN_AS(texture, TEXTURE);
KNOWN_AS(War3MapVertex, WAR3MAPVERTEX);
KNOWN_AS(SheetLayout, SHEETLAYOUT);
KNOWN_AS(SheetCell, SHEETCELL);
KNOWN_AS(war3map, WAR3MAP);
KNOWN_AS(Doodad, DOODAD);
KNOWN_AS(edict, EDICT);
KNOWN_AS(EntityState, ENTITYSTATE);
KNOWN_AS(vector3, VECTOR3);
KNOWN_AS(color32, COLOR32);
KNOWN_AS(size2, SIZE2);
KNOWN_AS(client_message, CLIENTMESSAGE);
KNOWN_AS(AnimationInfo, ANIMATION);

KNOWN_AS(TerrainInfo, TERRAININFO);
KNOWN_AS(CliffInfo, CLIFFINFO);
KNOWN_AS(AnimLookup, ANIMLOOKUP);

typedef char PATHSTR[MAX_PATHLEN];
typedef void const *LPCVOID;

struct color { float r, g, b, a; };
struct color32 { BYTE r, g, b, a; };
struct bounds { float min, max; };
struct rect { float x, y, width, height; };
struct edges { float left, top, right, bottom; };
struct transform2 { VECTOR2 translation, scale; float rotation; };
struct transform3 { VECTOR3 translation, rotation, scale; };

struct EntityState {
    DWORD number; // edict index
    VECTOR3 origin;
    float angle;
    float scale;
    DWORD model;
    DWORD image;
    DWORD sound;
    DWORD frame;
    DWORD event;
    DWORD team;
};

struct AnimationInfo {
    DWORD firstframe;
    DWORD lastframe;
    DWORD framerate;
};

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
    LPSHEETCELL lpNext;
};

HANDLE FS_ParseSheet(LPCSTR szFileName, LPCSHEETLAYOUT lpLayout, DWORD dwElementSize);
void LoadMap(LPCSTR pFilename);

void FS_Init(void);
void FS_Shutdown(void);

void Com_Quit(void);
void Sys_Quit(void);

HANDLE FS_OpenFile(LPCSTR szFileName);
bool FS_ExtractFile(LPCSTR szToExtract, LPCSTR szExtracted);
LPSHEETCELL FS_ReadSheet(LPCSTR szFileName);

void CL_Init(void);
void CL_Frame(DWORD msec);
void CL_Shutdown(void);

void SV_Init(void);
void SV_Frame(DWORD msec);
void SV_Shutdown(void);

HANDLE MemAlloc(long size);
void MemFree(HANDLE mem);

void Sys_MkDir(LPCSTR szDirectory);

#endif
