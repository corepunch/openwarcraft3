#ifndef __common_h__
#define __common_h__

#include <StormLib.h>

#include "net.h"
#include "../math/math.h"

#define TMP_MAP "/tmp/map.w3m"
#define MAP_VERTEX_SIZE 7
#define MAX_SHEET_LINE 1024
#define MAX_PATHLEN 256
#define TILESIZE 128
#define HEIGHT_COR (TILESIZE * 2 + 5)
#define MIN(x, y) (((x)<(y))?(x):(y))
#define MAX(x, y) (((x)>(y))?(x):(y))
#define DECODE_HEIGHT(x) (((x) - 0x2000) / 4)
#define DOODAD_SIZE 42
#define MAKEFOURCC(ch0, ch1, ch2, ch3) ((int)(char)(ch0) | ((int)(char)(ch1) << 8) | ((int)(char)(ch2) << 16) | ((int)(char)(ch3) << 24))
#define FOFS(type, x) (HANDLE)&(((struct type *)NULL)->x)

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

#define SFileReadArray(hFile, object, variable, elemsize) \
SFileReadFile(hFile, &object->num##variable, 4, NULL, NULL); \
if (object->num##variable > 0) {object->lp##variable = MemAlloc(object->num##variable * elemsize); \
SFileReadFile(hFile, object->lp##variable, object->num##variable * elemsize, NULL, NULL); }

#define FOR_LOOP(property, max) for (uint32_t property = 0, end = max; property < end; ++property)
#define EPSILON 0.001f
#define PrintTag(tag)do { LPSTR ch = (char*)&tag; printf("%c%c%c%c\n", ch[0], ch[1], ch[2], ch[3]); } while(false);

#define FOR_EACH_LIST(type, property, list) \
for (type *property = list, *next = list ? (list)->lpNext : NULL; \
property; \
property = next, next = next ? next->lpNext : NULL)

#define FOR_EACH(type, property, array, num) \
for (type *property = array; property - array < num; property++)

enum {
    kEntityChangeFlag_originX,
    kEntityChangeFlag_originY,
    kEntityChangeFlag_originZ,
    kEntityChangeFlag_angle,
    kEntityChangeFlag_scale,
    kEntityChangeFlag_remove,
    kEntityChangeFlag_frame,
    kEntityChangeFlag_model,
    kEntityChangeFlag_image,
};

enum {
    kTerrainInfo_tileID = 1,
    kTerrainInfo_dir,
    kTerrainInfo_file,
    kTerrainInfo_comment,
    kTerrainInfo_name,
    kTerrainInfo_buildable,
    kTerrainInfo_footprints,
    kTerrainInfo_walkable,
    kTerrainInfo_flyable,
    kTerrainInfo_blightPri,
    kTerrainInfo_convertTo,
    kTerrainInfo_InBeta,
};

enum {
    kCliffInfo_cliffID = 1,
    kCliffInfo_cliffModelDir,
    kCliffInfo_rampModelDir,
    kCliffInfo_texDir,
    kCliffInfo_texFile,
    kCliffInfo_name,
    kCliffInfo_groundTile,
    kCliffInfo_upperTile,
    kCliffInfo_cliffClass,
    kCliffInfo_oldID,
};

enum MDLTEXOP {
  TEXOP_LOAD = 0x0,
  TEXOP_TRANSPARENT = 0x1,
  TEXOP_BLEND = 0x2,
  TEXOP_ADD = 0x3,
  TEXOP_ADD_ALPHA = 0x4,
  TEXOP_MODULATE = 0x5,
  TEXOP_MODULATE2X = 0x6,
  NUMTEXOPS = 0x7,
};

enum MDLGEO {
  MODEL_GEO_UNSHADED = 0x1,
  MODEL_GEO_SPHERE_ENV_MAP = 0x2,  // unused until v1500
  MODEL_GEO_WRAPWIDTH = 0x4,       // unused until v1500
  MODEL_GEO_WRAPHEIGHT = 0x8,      // unused until v1500
  MODEL_GEO_TWOSIDED = 0x10,
  MODEL_GEO_UNFOGGED = 0x20,
  MODEL_GEO_NO_DEPTH_TEST = 0x40,
  MODEL_GEO_NO_DEPTH_SET = 0x80,
  MODEL_GEO_NO_FALLBACK = 0x100,   // added in v1500. seen in ElwynnTallWaterfall01.mdx, FelwoodTallWaterfall01.mdx and LavaFallsBlackRock*.mdx
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
//    clc_move,                // [[usercmd_t]
//    clc_userinfo,            // [[userinfo string]
    clc_stringcmd            // [string] message
};

typedef enum t_attrib_id {
    attrib_position,
    attrib_color,
    attrib_texcoord,
    attrib_texcoord2,
    attrib_skin,
    attrib_boneWeight,
} t_attrib_id;

struct tModel;
struct texture;

ADD_TYPEDEFS(tModel, MODEL);
ADD_TYPEDEFS(texture, TEXTURE);
ADD_TYPEDEFS(TerrainVertex, TERRAINVERTEX);
ADD_TYPEDEFS(SheetLayout, SHEETLAYOUT);
ADD_TYPEDEFS(TerrainInfo, TERRAININFO);
ADD_TYPEDEFS(CliffInfo, CLIFFINFO);
ADD_TYPEDEFS(terrain, TERRAIN);
ADD_TYPEDEFS(Doodad, DOODAD);
ADD_TYPEDEFS(edict, EDICT);
ADD_TYPEDEFS(EntityState, ENTITYSTATE);
ADD_TYPEDEFS(vector3, VECTOR3);

typedef char PATHSTR[MAX_PATHLEN];
typedef char SHEETSTR[64];
typedef void const *LPCVOID;

struct color { float r, g, b, a; };
struct color32 { uint8_t r, g, b, a; };
struct bounds { float min, max; };
struct rect { float x, y, width, height; };
struct edges { float left, top, right, bottom; };
struct transform2 { VECTOR2 translation, scale; float rotation; };
struct transform3 { VECTOR3 translation, rotation, scale; };

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

struct EntityState {
    int number; // edict index
    VECTOR3 origin;
    float angle;
    float scale;
    int model;
    int image;
    int sound;
    int frame;
    int event;
};

struct AnimationInfo {
    DWORD start_frame;
    DWORD end_frame;
    DWORD framerate;
};

struct TerrainInfo {
    DWORD tileID;
    SHEETSTR dir;
    SHEETSTR file;
    LPTEXTURE lpTexture;
    LPTERRAININFO lpNext;
};

struct PathMapNode {
    char unused:1;
    char nowalk:1;
    char nofly:1;
    char nobuild:1;
    char unused2:1;
    char blight:1;
    char nowater:1;
    char unknown:1;
};

struct CliffInfo {
    DWORD cliffID;
    SHEETSTR cliffModelDir;
    SHEETSTR rampModelDir;
    SHEETSTR texDir;
    SHEETSTR texFile;
    SHEETSTR name;
    DWORD groundTile;
    DWORD upperTile;
    LPCLIFFINFO lpNext;
    LPTEXTURE texture;
};

struct Doodad {
    DWORD doodID;
    DWORD variation;
    VECTOR3 position;
    float angle;
    VECTOR3 scale;
    char nFlags;
    char lifetime;
    DWORD id_num;
};

struct TerrainVertex {
    short accurate_height;
    short waterlevel;
    BYTE ground:4;
    BYTE ramp:1;
    BYTE blight:1;
    BYTE water:1;
    BYTE boundary:1;
    BYTE details:4;
    BYTE unknown:4;
    BYTE level:4;
    BYTE cliff:4;
};

struct size2 {
    DWORD width;
    DWORD height;
};

struct terrain {
    DWORD header;
    DWORD version;
    char tileset;
    DWORD custom;
    LPDWORD lpGrounds;
    LPDWORD lpCliffs;
    VECTOR2 center;
    struct size2 size;
    LPTERRAINVERTEX vertices;
    DWORD numGrounds;
    DWORD numCliffs;
};

struct SheetCell {
    DWORD column;
    DWORD row;
    LPSTR text;
    struct SheetCell *lpNext;
};

LPCTERRAINVERTEX GetTerrainVertex(LPCTERRAIN lpTerrain, DWORD x, DWORD y);
LPTERRAIN  FileReadTerrain(HANDLE hArchive);
HANDLE FS_ParseSheet(LPCSTR szFileName, LPCSHEETLAYOUT lpLayout, DWORD dwElementSize, HANDLE lpNextFieldOffset);
void LoadMap(LPCSTR pFilename);
LPCTERRAINVERTEX GetTerrainVertex(LPCTERRAIN lpTerrain, DWORD x, DWORD y);
LPTERRAININFO FindTerrainInfo(DWORD tileID);
LPCLIFFINFO FindCliffInfo(DWORD cliffID);
DWORD GetTile(LPCTERRAINVERTEX mv, DWORD ground);
float GetTerrainVertexHeight(LPCTERRAINVERTEX vert);
float GetTerrainVertexWaterLevel(LPCTERRAINVERTEX vert);
void GetTileVertices(DWORD x, DWORD y, LPCTERRAIN lpTerrain, LPTERRAINVERTEX vertices);
DWORD GetTileRamps(LPCTERRAINVERTEX vertices);
DWORD IsTileCliff(LPCTERRAINVERTEX vertices);
DWORD IsTileWater(LPCTERRAINVERTEX vertices);

void FS_Init(void);
void FS_Shutdown(void);

void Com_Quit(void);
void Sys_Quit(void);

HANDLE FS_OpenFile(LPCSTR szFileName);
bool FS_ExtractFile(LPCSTR szToExtract, LPCSTR szExtracted);
struct SheetCell *FS_ReadSheet(LPCSTR szFileName);

void CL_Init(void);
void CL_Frame(DWORD msec);
void CL_Shutdown(void);

void SV_Init(void);
void SV_Frame(DWORD msec);
void SV_Shutdown(void);

HANDLE MemAlloc(long size);
void MemFree(HANDLE mem);

#endif
