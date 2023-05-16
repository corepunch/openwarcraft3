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
#define MAKEFOURCC(ch0, ch1, ch2, ch3) ((int)(char)(ch0) | ((int)(char)(ch1) << 8) | ((int)(char)(ch2) << 16) | ((int)(char)(ch3) << 24 ))

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
#define PrintTag(tag)do { char *ch = (char*)&tag; printf("%c%c%c%c\n", ch[0], ch[1], ch[2], ch[3]); } while(false);

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

enum {
    kDoodadInfo_doodID = 1,
    kDoodadInfo_category,
    kDoodadInfo_tilesets,
    kDoodadInfo_tilesetSpecific,
    kDoodadInfo_dir,
    kDoodadInfo_file,
    kDoodadInfo_comment,
    kDoodadInfo_name,
    kDoodadInfo_doodClass,
    kDoodadInfo_soundLoop,
    kDoodadInfo_selSize,
    kDoodadInfo_defScale,
    kDoodadInfo_minScale,
    kDoodadInfo_maxScale,
    kDoodadInfo_canPlaceRandScale,
    kDoodadInfo_useClickHelper,
    kDoodadInfo_maxPitch,
    kDoodadInfo_maxRoll,
    kDoodadInfo_visRadius,
    kDoodadInfo_walkable,
    kDoodadInfo_numVar,
    kDoodadInfo_onCliffs,
    kDoodadInfo_onWater,
    kDoodadInfo_floats,
    kDoodadInfo_shadow,
    kDoodadInfo_showInFog,
    kDoodadInfo_animInFog,
    kDoodadInfo_fixedRot,
    kDoodadInfo_pathTex,
    kDoodadInfo_showInMM,
    kDoodadInfo_useMMColor,
    kDoodadInfo_MMRed,
    kDoodadInfo_MMGreen,
    kDoodadInfo_MMBlue,
};

enum {
    kDestructableInfo_DestructableID = 1,
    kDestructableInfo_category,
    kDestructableInfo_tilesets,
    kDestructableInfo_tilesetSpecific,
    kDestructableInfo_dir,
    kDestructableInfo_file,
    kDestructableInfo_lightweight,
    kDestructableInfo_fatLOS,
    kDestructableInfo_texID,
    kDestructableInfo_texFile,
    kDestructableInfo_comment,
    kDestructableInfo_name,
    kDestructableInfo_doodClass,
    kDestructableInfo_useClickHelper,
    kDestructableInfo_onCliffs,
    kDestructableInfo_onWater,
    kDestructableInfo_canPlaceDead,
    kDestructableInfo_walkable,
    kDestructableInfo_cliffHeight,
    kDestructableInfo_targType,
    kDestructableInfo_armor,
    kDestructableInfo_numVar,
    kDestructableInfo_HP,
    kDestructableInfo_occH,
    kDestructableInfo_flyH,
    kDestructableInfo_fixedRot,
    kDestructableInfo_selSize,
    kDestructableInfo_minScale,
    kDestructableInfo_maxScale,
    kDestructableInfo_canPlaceRandScale,
    kDestructableInfo_maxPitch,
    kDestructableInfo_maxRoll,
    kDestructableInfo_radius,
    kDestructableInfo_fogRadius,
    kDestructableInfo_fogVis,
    kDestructableInfo_pathTex,
    kDestructableInfo_pathTexDeath,
    kDestructableInfo_deathSnd,
    kDestructableInfo_shadow,
    kDestructableInfo_showInMM,
    kDestructableInfo_useMMColor,
    kDestructableInfo_MMRed,
    kDestructableInfo_MMGreen,
    kDestructableInfo_MMBlue,
    kDestructableInfo_InBeta,
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
} t_attrib_id;

typedef char path_t[MAX_PATHLEN];

struct color { float r, g, b, a; };
struct color32 { uint8_t r, g, b, a; };
struct bounds { float min, max; };
struct rect { float x, y, width, height; };
struct edges { float left, top, right, bottom; };
struct transform2 { struct vector2 translation, scale; float rotation; };
struct transform3 { struct vector3 translation, rotation, scale; };

struct entity_state {
    int number; // edict index
    struct vector3 origin;
    float angle;
    float scale;
    int model;
    int image;
    int sound;
    int frame;
    int event;
};

struct TerrainInfo {
    int dwTileID;
    char sDirectory[64];
    char sFilename[64];
    struct texture *lpTexture;
    struct TerrainInfo *lpNext;
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
    int cliffID;
    char cliffModelDir[64];
    char rampModelDir[64];
    char texDir[64];
    char texFile[64];
    char name[64];
    int groundTile;
    int upperTile;
    struct CliffInfo *lpNext;
    struct texture *texture;
};

struct DoodadInfo {
    int doodID;
    char dir[64];
    char file[64];
    char pathTex[64];
    struct DoodadInfo *lpNext;
    struct tModel const *lpModel;
};

struct DestructableInfo {
    int DestructableID;
    char category[64];
    char tilesets[64];
    char tilesetSpecific[64];
    char dir[64];
    char file[64];
    int lightweight;
    int fatLOS;
    int texID;
    char texFile[64];
    struct DestructableInfo *lpNext;
    struct tModel const *lpModel;
    struct texture *lpTexture;
};

struct Doodad {
    int doodID;
    int variation;
    struct vector3 position;
    float angle;
    struct vector3 scale;
    char nFlags;
    char lifetime;
    int id_num;
};

struct TerrainVertex {
    short accurate_height;
    short waterlevel;
    unsigned char ground:4;
    unsigned char ramp:1;
    unsigned char blight:1;
    unsigned char water:1;
    unsigned char boundary:1;
    unsigned char details:4;
    unsigned char unknown:4;
    unsigned char level:4;
    unsigned char cliff:4;
};

struct size2 {
    int width;
    int height;
};

struct terrain {
    int header;
    int version;
    char tileset;
    int custom;
    int *lpGrounds;
    int *lpCliffs;
    struct vector2 center;
    struct size2 size;
    struct TerrainVertex *vertices;
    int numGrounds;
    int numCliffs;
};

struct SheetCell {
    int x;
    int y;
    char *text;
    struct SheetCell *lpNext;
};

struct tModel;
struct texture;

struct TerrainVertex const *GetTerrainVertex(struct terrain const *heightmap, int x, int y);
struct terrain *FileReadTerrain(HANDLE hArchive);
struct TerrainInfo *MakeTerrainInfo(struct SheetCell *sheet);
struct CliffInfo *MakeCliffInfo(struct SheetCell *sheet);
struct DoodadInfo *MakeDoodadInfo(struct SheetCell *sheet);
struct DestructableInfo *MakeDestructableInfo(struct SheetCell *sheet);
void LoadMap(LPCSTR pFilename);

struct TerrainVertex const *GetTerrainVertex(struct terrain const *heightmap, int x, int y);
struct TerrainInfo *FindTerrainInfo(int tileID);
struct CliffInfo *FindCliffInfo(int cliffID);
struct DoodadInfo *FindDoodadInfo(int doodID);
struct DestructableInfo *FindDestructableInfo(int DestructableID);

int GetTile(struct TerrainVertex const *mv, int ground);
float GetTerrainVertexHeight(struct TerrainVertex const *vert);
float GetTerrainVertexWaterLevel(struct TerrainVertex const *vert);
void GetTileVertices(int x, int y, struct terrain const *heightmap, struct TerrainVertex *vertices);
int GetTileRamps(struct TerrainVertex const *vertices);
int IsTileCliff(struct TerrainVertex const *vertices);
int IsTileWater(struct TerrainVertex const *vertices);

void FS_Init(void);
void FS_Shutdown(void);

void Com_Quit(void);
void Sys_Quit(void);

HANDLE FS_OpenFile(LPCSTR szFileName);
bool FS_ExtractFile(LPCSTR szToExtract, LPCSTR szExtracted);
struct SheetCell *FS_ReadSheet(LPCSTR szFileName);

void CL_Init(void);
void CL_Frame(int msec);
void CL_Shutdown(void);

void SV_Init(void);
void SV_Frame(int msec);
void SV_Shutdown(void);

void *MemAlloc(long size);
void MemFree(void *mem);

#endif
