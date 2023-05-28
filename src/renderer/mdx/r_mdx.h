#ifndef __r_mdx_h__
#define __r_mdx_h__

#include "../../common/common.h"

#define ID_MDLX ('M'|('D'<<8)|('L'<<16)|('X'<<24))

#define MAX_NODES 256
#define MAX_BONES 64

typedef char mdxObjectName_t[80];
typedef char mdxFileName_t[260];
typedef float mdxVec2_t[2];
typedef float mdxVec3_t[3];
typedef float vec_t;

typedef enum {
  TEXOP_LOAD = 0x0,
  TEXOP_TRANSPARENT = 0x1,
  TEXOP_BLEND = 0x2,
  TEXOP_ADD = 0x3,
  TEXOP_ADD_ALPHA = 0x4,
  TEXOP_MODULATE = 0x5,
  TEXOP_MODULATE2X = 0x6,
  NUMTEXOPS = 0x7,
} mdxTexOp_t;

typedef enum {
  MODEL_GEO_UNSHADED = 0x1,
  MODEL_GEO_SPHERE_ENV_MAP = 0x2,  // unused until v1500
  MODEL_GEO_WRAPWIDTH = 0x4,       // unused until v1500
  MODEL_GEO_WRAPHEIGHT = 0x8,      // unused until v1500
  MODEL_GEO_TWOSIDED = 0x10,
  MODEL_GEO_UNFOGGED = 0x20,
  MODEL_GEO_NO_DEPTH_TEST = 0x40,
  MODEL_GEO_NO_DEPTH_SET = 0x80,
  MODEL_GEO_NO_FALLBACK = 0x100,   // added in v1500. seen in ElwynnTallWaterfall01.mdx, FelwoodTallWaterfall01.mdx and LavaFallsBlackRock*.mdx
} mdxGeoFlags_t;

typedef enum {
    TRACK_NO_INTERP = 0x0,
    TRACK_LINEAR = 0x1,
    TRACK_HERMITE = 0x2,
    TRACK_BEZIER = 0x3,
    NUM_TRACK_TYPES = 0x4,
} MODELKEYTRACKTYPE;

typedef enum {
    TDATA_INT,
    TDATA_FLOAT,
    TDATA_VECTOR3,
    TDATA_QUATERNION,
} MODELKEYTRACKDATATYPE;

typedef struct mdxBounds_s {
    vec_t radius;
    mdxVec3_t min;
    mdxVec3_t max;
} mdxBounds_t;

typedef enum {
    TEXREPL_NONE,
    TEXREPL_TEAMCOLOR,
    TEXREPL_TEAMGLOW,
    TEXREPL_TEXTURE = 31,
} replaceableID_t;

typedef struct mdxSequence_s {
    mdxObjectName_t name;
    DWORD interval[2];
    vec_t movespeed;     // movement speed of the entity while playing this animation
    DWORD flags;      // &1: non looping
    vec_t rarity;
    int syncpoint;
    mdxBounds_t bounds;
} mdxSequence_t;

typedef struct mdxInfo_s {
    mdxObjectName_t name;
    mdxFileName_t animationFile;
    mdxBounds_t bounds;
    DWORD blendTime;
} mdxInfo_t;

typedef struct {
    int time;
    char data[];
} mdxKeyFrame_t;

typedef struct {
    DWORD keyframeCount;
    MODELKEYTRACKDATATYPE datatype;
    MODELKEYTRACKTYPE type;
    DWORD globalSeqId;        // GLBS index or 0xFFFFFFFF if none
    mdxKeyFrame_t values[];
} mdxKeyTrack_t;

typedef struct mdxGeosetAnim_s {
    vec_t staticAlpha;        // 0 is transparent, 1 is opaque
    DWORD flags;           // &1: color
    mdxVec3_t staticColor;
    DWORD geosetId;        // GEOS index or 0xFFFFFFFF if none
    mdxKeyTrack_t *alphas; // float
    mdxKeyTrack_t *colors; // vec3
    struct mdxGeosetAnim_s *next;
} mdxGeosetAnim_t;

typedef struct mdxNode_s {
    mdxObjectName_t name;
    DWORD object_id; // globally unique id, used as the index in the hierarchy. index into PIVT
    DWORD parent_id; // parent MDLGENOBJECT's objectId or 0xFFFFFFFF if none
    DWORD flags;
    mdxKeyTrack_t *translation; // vec3
    mdxKeyTrack_t *rotation; // quat
    mdxKeyTrack_t *scale; // vec3
} mdxNode_t;

typedef struct mdxBone_s {
    mdxNode_t node;
    DWORD geoset_id;
    DWORD geoset_animation_id;
    struct mdxBone_s *next;
} mdxBone_t;

typedef struct mdxHelper_s {
    mdxNode_t node;
    struct mdxHelper_s *next;
} mdxHelper_t;

typedef struct mdxGlobalSequence_s {
    DWORD value;
} mdxGlobalSequence_t;

typedef struct mdxEvent_s {
    mdxNode_t node;
    DWORD num_keys;
    DWORD globalSeqId;
    DWORD *keys;
    struct mdxEvent_s *next;
} mdxEvent_t;

typedef struct mdxTexture_s {
    replaceableID_t replaceableID;
    char path[256];
    int texid;
    int nWrapping; //(1:WrapWidth; 2:WrapHeight; 3:Both)
} mdxTexture_t;

typedef struct mdxMaterialLayer_s {
    mdxTexOp_t blendMode;
    mdxGeoFlags_t flags;
    DWORD textureId;        // TEXS index or 0xFFFFFFFF for none
    DWORD transformId;      // TXAN index or 0xFFFFFFFF for none
    int coordId;           // UAVS index or -1 for none, defines vertex buffer format coordId == -1 ? GxVBF_PN : GxVBF_PNT0
    vec_t staticAlpha;
    mdxKeyTrack_t *alpha; // float
    mdxKeyTrack_t *flipbook; // int
} mdxMaterialLayer_t;

typedef struct mdxMaterial_s {
    int priority;
    int flags;
    int num_layers;
    mdxMaterialLayer_t *layers;
    mdxKeyTrack_t *emission; // float
    mdxKeyTrack_t *alpha; // float
    mdxKeyTrack_t *flipbook; // int
    struct mdxMaterial_s *next;
} mdxMaterial_t;

typedef struct mdxCamera_s {
    mdxObjectName_t name;
    mdxVec3_t pivot;
    vec_t fieldOfView;      // default is 0.9500215
    vec_t farClip;          // default is 27.7777786
    vec_t nearClip;         // default is 0.222222224
    mdxVec3_t targetPivot;
    mdxKeyTrack_t *translation; // vec3
    mdxKeyTrack_t *roll; // float
    mdxKeyTrack_t *targetTranslation; // vec3
    struct mdxCamera_s *next;
} mdxCamera_t;

typedef struct mdxGeoset_s {
    mdxVec3_t *vertices;
    mdxVec3_t *normals;
    mdxVec2_t *texcoord;
    mdxBounds_t *bounds;
    mdxBounds_t default_bounds;
    mdxGeosetAnim_t *geosetAnim;
    int *matrices;
    int *primitiveTypes;
    int *primitiveCounts;
    short *triangles;
    char *vertexGroups;
    int *matrixGroupSizes;
    int materialID;
    int group;
    int selectable;// (0:none;4:Unselectable)
    int num_vertices;
    int num_normals;
    int num_texcoord;
    int num_matrices;
    int num_primitiveTypes;
    int num_primitiveCounts;
    int num_triangles;
    int num_vertexGroups;
    int num_matrixGroupSizes;
    int num_bounds;
    int num_texcoordChannels;
    void *userdata;
    struct mdxGeoset_s *next;
} mdxGeoset_t;

typedef struct mdxModel_s {
    DWORD version;
    mdxInfo_t info;
    mdxGeoset_t *geosets;
    mdxTexture_t *textures;
    mdxSequence_t *sequences;
    mdxEvent_t *events;
    mdxVec3_t *pivots;
    mdxMaterial_t *materials;
    mdxBone_t *bones;
    mdxGeosetAnim_t *geosetAnims;
    mdxHelper_t *helpers;
    mdxCamera_t *cameras;
    mdxGlobalSequence_t *globalSequences;
    mdxNode_t *nodes[MAX_NODES];
    int num_textures;
    int num_sequences;
    int num_globalSequences;
    int num_pivots;
} mdxModel_t;

mdxModel_t *MDX_LoadBuffer(void *buffer, DWORD size);
void MDX_Release(mdxModel_t *model);

#endif
