#ifndef r_mdx_h
#define r_mdx_h

#include "../common/common.h"

#define MODELTRACKINFOSIZE 16

typedef char tModelObjectName[80];
typedef char tModelFileName[260];

enum tModelKeyTrackType {
  TRACK_NO_INTERP = 0x0,
  TRACK_LINEAR = 0x1,
  TRACK_HERMITE = 0x2,
  TRACK_BEZIER = 0x3,
  NUM_TRACK_TYPES = 0x4,
};

enum tModelKeyTrackDataType {
    kModelKeyTrackDataTypeFloat,
    kModelKeyTrackDataTypeVector3,
    kModelKeyTrackDataTypeQuaternion,
};

struct tModelBounds {
    float radius;
    VECTOR3 min;
    VECTOR3 max;
};

struct tModelGeoset {
    LPVECTOR3 lpVertices;
    LPVECTOR3 lpNormals;
    LPVECTOR2 lpTexcoord;
    struct tModelBounds *lpBounds;
    struct tModelBounds default_bounds;
    struct tModelGeoset *lpNext;
    LPBUFFER lpBuffer;
    struct tModelGeosetAnim *lpGeosetAnim;
    int *lpMatrices;
    int *lpPrimitiveTypes;
    int *lpPrimitiveCounts;
    short *lpTriangles;
    LPSTR lpVertexGroups;
    int *lpMatrixGroupSizes;
    int material;
    int group;
    int selectable;// (0:none;4:Unselectable)
    int numVertices;
    int numNormals;
    int numTexcoord;
    int numMatrices;
    int numPrimitiveTypes;
    int numPrimitiveCounts;
    int numTriangles;
    int numVertexGroups;
    int numMatrixGroupSizes;
    int numBounds;
    int numTexcoordChannels;
};

struct tModelTexture {
    int ident;
    char path[256];
    int texid;
    int nWrapping; //(1:WrapWidth; 2:WrapHeight; 3:Both)
};

struct tModelMaterialLayer {
    enum MDLTEXOP blendMode;
    enum MDLGEO flags;
    uint32_t textureId;        // TEXS index or 0xFFFFFFFF for none
    uint32_t transformId;      // TXAN index or 0xFFFFFFFF for none
    int32_t coordId;           // UAVS index or -1 for none, defines vertex buffer format coordId == -1 ? GxVBF_PN : GxVBF_PNT0
    float staticAlpha;
};

struct tModelMaterial {
    int priority;
    int flags;
    int num_layers;
    struct tModelMaterialLayer *layers;
    struct tModelMaterial *lpNext;
};

struct tModelSequence {
    tModelObjectName name;
    uint32_t interval[2];
    float movespeed;     // movement speed of the entity while playing this animation
    uint32_t flags;      // &1: non looping
    float rarity;
    int syncpoint;
    struct tModelBounds bounds;
};

struct tModelInfo {
    tModelObjectName name;
    tModelFileName animationFile;
    struct tModelBounds bounds;
    uint32_t blendTime;
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
    uint32_t dwKeyframeCount;
    enum tModelKeyTrackDataType datatype;
    enum tModelKeyTrackType type;
    uint32_t globalSeqId;        // GLBS index or 0xFFFFFFFF if none
    struct tModelKeyFrame values[];
};

struct tModelGeosetAnim {
    float staticAlpha;        // 0 is transparent, 1 is opaque
    uint32_t flags;           // &1: color
    struct tModelColor staticColor;
    uint32_t geosetId;        // GEOS index or 0xFFFFFFFF if none
    struct tModelGeosetAnim *lpNext;
    struct tModelKeyTrack *lpAlphas; // float
};

struct tModelNode {
    tModelObjectName name;
    uint32_t objectId; // globally unique id, used as the index in the hierarchy. index into PIVT
    uint32_t parentId; // parent MDLGENOBJECT's objectId or 0xFFFFFFFF if none
    uint32_t flags;
    struct tModelKeyTrack *lpTranslation; // vec3
    struct tModelKeyTrack *lpRotation; // vec4
    struct tModelKeyTrack *lpScale; // vec3
    struct tModelNode *lpParent;
    struct tModelPivot *lpPivot;
    struct matrix4 localMatrix;
    struct matrix4 globalMatrix;
};

struct tModelBone {
    struct tModelNode node;
    struct tModelBone *lpNext;
    uint32_t geoset_id;
    uint32_t geoset_animation_id;
};

struct tModelHelper {
    struct tModelNode node;
    struct tModelHelper *lpNext;
};

struct tModelGlobalSequence {
    uint32_t value;
};

struct tModel {
    struct tModelInfo info;
    struct tModelGeoset *lpGeosets;
    struct tModelTexture *lpTextures;
    struct tModelSequence *lpSequences;
    struct tModelPivot *lpPivots;
    struct tModelMaterial *lpMaterials;
    struct tModelBone *lpBones;
    struct tModelGeosetAnim *lpGeosetAnims;
    struct tModelHelper *lpHelpers;
    struct tModelGlobalSequence *lpGlobalSequences;
    struct tModelSequence *lpCurrentAnimation;
    int numTextures;
    int numSequences;
    int numGlobalSequences;
    int numPivots;
};

#endif

