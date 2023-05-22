#ifndef r_mdx_h
#define r_mdx_h

#include "../common/common.h"

#define MODELTRACKINFOSIZE 16

KNOWN_AS(tModelGeoset, MODELGEOSET);
KNOWN_AS(tModelKeyTrack, MODELKEYTRACK);
KNOWN_AS(tModelKeyFrame, MODELKEYFRAME);
KNOWN_AS(tModelNode, MODELNODE);
KNOWN_AS(tModelMaterial, MODELMATERIAL);
KNOWN_AS(tModelMaterialLayer, MODELLAYER);
KNOWN_AS(tModelSequence, MODELSEQUENCE);

typedef char tModelObjectName[80];
typedef char tModelFileName[260];

typedef enum {
    TRACK_NO_INTERP = 0x0,
    TRACK_LINEAR = 0x1,
    TRACK_HERMITE = 0x2,
    TRACK_BEZIER = 0x3,
    NUM_TRACK_TYPES = 0x4,
} MODELKEYTRACKTYPE;

typedef enum {
    kModelKeyTrackDataTypeFloat,
    kModelKeyTrackDataTypeVector3,
    kModelKeyTrackDataTypeQuaternion,
} MODELKEYTRACKDATATYPE;

struct tModelBounds {
    float radius;
    VECTOR3 min;
    VECTOR3 max;
};

struct tModelGeoset {
    LPVECTOR3 vertices;
    LPVECTOR3 normals;
    LPVECTOR2 texcoord;
    struct tModelBounds *bounds;
    struct tModelBounds default_bounds;
    LPMODELGEOSET next;
    LPBUFFER buffer;
    struct tModelGeosetAnim *geosetAnim;
    int *matrices;
    int *primitiveTypes;
    int *primitiveCounts;
    short *triangles;
    LPSTR vertexGroups;
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
};

typedef enum {
    TEXREPL_NONE,
    TEXREPL_TEAMCOLOR,
    TEXREPL_TEAMGLOW,
    TEXREPL_TEXTURE = 31,
} replaceableID_t;

struct tModelTexture {
    replaceableID_t replaceableID;
    char path[256];
    int texid;
    int nWrapping; //(1:WrapWidth; 2:WrapHeight; 3:Both)
};

struct tModelMaterialLayer {
    enum MDLTEXOP blendMode;
    enum MDLGEO flags;
    DWORD textureId;        // TEXS index or 0xFFFFFFFF for none
    DWORD transformId;      // TXAN index or 0xFFFFFFFF for none
    int coordId;           // UAVS index or -1 for none, defines vertex buffer format coordId == -1 ? GxVBF_PN : GxVBF_PNT0
    float staticAlpha;
};

struct tModelMaterial {
    int priority;
    int flags;
    int num_layers;
    LPMODELLAYER layers;
    LPMODELMATERIAL next;
};

struct tModelSequence {
    tModelObjectName name;
    DWORD interval[2];
    float movespeed;     // movement speed of the entity while playing this animation
    DWORD flags;      // &1: non looping
    float rarity;
    int syncpoint;
    struct tModelBounds bounds;
};

struct tModelInfo {
    tModelObjectName name;
    tModelFileName animationFile;
    struct tModelBounds bounds;
    DWORD blendTime;
};

struct tModelPivot {
    float x, y, z;
};

struct tModelColor {
    float r, g, b;
};

struct tModelKeyFrame {
    int time;
    char data[];
};

struct tModelKeyTrack {
    DWORD keyframeCount;
    MODELKEYTRACKDATATYPE datatype;
    MODELKEYTRACKTYPE type;
    DWORD globalSeqId;        // GLBS index or 0xFFFFFFFF if none
    struct tModelKeyFrame values[];
};

struct tModelGeosetAnim {
    float staticAlpha;        // 0 is transparent, 1 is opaque
    DWORD flags;           // &1: color
    struct tModelColor staticColor;
    DWORD geosetId;        // GEOS index or 0xFFFFFFFF if none
    struct tModelGeosetAnim *next;
    LPMODELKEYTRACK alphas; // float
};

struct tModelNode {
    tModelObjectName name;
    DWORD objectId; // globally unique id, used as the index in the hierarchy. index into PIVT
    DWORD parentId; // parent MDLGENOBJECT's objectId or 0xFFFFFFFF if none
    DWORD flags;
    LPMODELKEYTRACK translation; // vec3
    LPMODELKEYTRACK rotation; // vec4
    LPMODELKEYTRACK scale; // vec3
    LPMODELNODE parent;
    struct tModelPivot *pivot;
    MATRIX4 localMatrix;
    MATRIX4 globalMatrix;
};

struct tModelBone {
    struct tModelNode node;
    struct tModelBone *next;
    DWORD geoset_id;
    DWORD geoset_animation_id;
};

struct tModelHelper {
    struct tModelNode node;
    struct tModelHelper *next;
};

struct tModelGlobalSequence {
    DWORD value;
};

struct tModelEvent {
    struct tModelNode node;
    DWORD num_keys;
    DWORD globalSeqId;
    DWORD *keys;
    struct tModelEvent *next;
};

struct tModel {
    struct tModelInfo info;
    LPMODELGEOSET geosets;
    struct tModelTexture *textures;
    LPMODELSEQUENCE sequences;
    struct tModelEvent *events;
    struct tModelPivot *pivots;
    LPMODELMATERIAL materials;
    struct tModelBone *bones;
    struct tModelGeosetAnim *geosetAnims;
    struct tModelHelper *helpers;
    struct tModelGlobalSequence *globalSequences;
    LPCMODELSEQUENCE currentAnimation;
    int num_textures;
    int num_sequences;
    int num_globalSequences;
    int num_pivots;
};

#endif
