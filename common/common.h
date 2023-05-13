#ifndef __common_h__
#define __common_h__

#include <StormLib.h>

#include "net.h"

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

#define MAX_CLIENTS 256        // absolute limit
#define MAX_EDICTS 1024    // must change protocol to increase more
#define MAX_LIGHTSTYLES 256
#define MAX_MODELS 256        // these are sent over the net as bytes
#define MAX_SOUNDS 256        // so they cannot be blindly increased
#define MAX_IMAGES 256
#define MAX_ITEMS 256
#define MAX_GENERAL (MAX_CLIENTS*2)    // general config strings

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
//    svc_spawnbaseline,
//    svc_centerprint,            // [string] to put in center of the screen
//    svc_download,                // [short] size [size bytes]
//    svc_playerinfo,                // variable
    svc_packetentities,            // [...]
//    svc_deltapacketentities,    // [...]
//    svc_frame
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

struct vector2 { float x, y; };
struct vector3 { float x, y, z; };
struct vector4 { float x, y, z, w; };
struct quaternion { float x, y, z, w; };
struct matrix4 { union { float v[16]; struct vector4 column[4]; }; };
struct color { float r, g, b, a; };
struct color32 { uint8_t r, g, b, a; };
struct bounds { float min, max; };
struct rect { float x, y, width, height; };
struct edges { float left, top, right, bottom; };
struct transform2 { struct vector2 translation, scale; float rotation; };
struct transform3 { struct vector3 translation, rotation, scale; };

struct TerrainInfo {
    int dwTileID;
    char sDirectory[64];
    char sFilename[64];
    struct tTexture *lpTexture;
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
    struct tTexture *texture;
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
    struct tTexture *lpTexture;
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

struct Terrain {
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
struct tTexture;

enum rotation_order {
    ROTATE_XYZ,
    ROTATE_XZY,
    ROTATE_YZX,
    ROTATE_YXZ,
    ROTATE_ZXY,
    ROTATE_ZYX
};

struct TerrainVertex const *GetTerrainVertex(struct Terrain const *heightmap, int x, int y);
struct Terrain *FileReadTerrain(HANDLE hArchive);
struct TerrainInfo *MakeTerrainInfo(struct SheetCell *sheet);
struct CliffInfo *MakeCliffInfo(struct SheetCell *sheet);
struct DoodadInfo *MakeDoodadInfo(struct SheetCell *sheet);
struct DestructableInfo *MakeDestructableInfo(struct SheetCell *sheet);
void LoadMap(LPCSTR pFilename);

struct TerrainVertex const *GetTerrainVertex(struct Terrain const *heightmap, int x, int y);
struct TerrainInfo *FindTerrainInfo(int tileID);
struct CliffInfo *FindCliffInfo(int cliffID);
struct DoodadInfo *FindDoodadInfo(int doodID);
struct DestructableInfo *FindDestructableInfo(int DestructableID);

float vector3_dot(struct vector3 const *a, struct vector3 const *b);
float vector3_lengthsq(struct vector3 const *vec);
float vector3_len(struct vector3 const *vec);
bool vector3_eq(struct vector3 const *a, struct vector3 const *b);
struct vector3 vector3_lerp(struct vector3 const *a, struct vector3 const *b, float t);
struct vector3 vector3_cross(struct vector3 const *a, struct vector3 const *b);
struct vector3 vector3_sub(struct vector3 const *a, struct vector3 const *b);
struct vector3 vector3_add(struct vector3 const *a, struct vector3 const *b);
struct vector3 vector3_mad(struct vector3 const *v, float s, struct vector3 const *b);
struct vector3 vector3_mul(struct vector3 const *a, struct vector3 const *b);
struct vector3 vector3_scale(struct vector3 const *v, float s);
void vector3_normalize(struct vector3* v);
void vector3_set(struct vector3* v, float x, float y, float z);
void vector3_clear(struct vector3* v);
struct vector3 vector3_unm(struct vector3 const* v);
void vector2_set(struct vector2* v, float x, float y);
struct vector2 vector2_scale(struct vector2 const *v, float s);
float vector2_dot(struct vector2 const *a, struct vector2 const *b);
float vector2_lengthsq(struct vector2 const *vec);
float vector2_len(struct vector2 const *vec);
void vector4_set(struct vector4* v, float x, float y, float z, float w);
struct vector4 vector4_scale(struct vector4 const *v, float s);
struct vector4 vector4_add(struct vector4 const *a, struct vector4 const *b);
struct vector4 vector4_unm(struct vector4 const* v);
void matrix4_identity(struct matrix4 *m);
void matrix4_translate(struct matrix4 *m, struct vector3 const *v);
void matrix4_rotate(struct matrix4 *m, struct vector3 const *v, enum rotation_order order);
void matrix4_scale(struct matrix4 *m, struct vector3 const *v);
void matrix4_multiply(struct matrix4 const *m1, struct matrix4 const *m2, struct matrix4 *out);
void matrix4_multiply_vector3(struct matrix4 const *m1, struct vector3 const *v, struct vector3 *out);
void matrix4_ortho(struct matrix4 *m, float left, float right, float bottom, float top, float znear, float zfar);
void matrix4_perspective(struct matrix4 *m, float angle, float aspect, float znear, float zfar);
void matrix4_lookat(struct matrix4 *m, struct vector3 const *eye, struct vector3 const *direction, struct vector3 const *up);
void matrix4_inverse(struct matrix4 const *m, struct matrix4 *out);
void matrix4_transpose(struct matrix4 const *m, struct matrix4 *out);
void matrix4_rotate4(struct matrix4 *m, struct vector4 const *quat);

int GetTile(struct TerrainVertex const *mv, int ground);
float GetTerrainVertexHeight(struct TerrainVertex const *vert);
float GetTerrainVertexWaterLevel(struct TerrainVertex const *vert);
void GetTileVertices(int x, int y, struct Terrain const *heightmap, struct TerrainVertex *vertices);
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
