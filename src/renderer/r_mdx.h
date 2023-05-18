#ifndef r_mdx_h
#define r_mdx_h

#include "../common/common.h"

#define MODELTRACKINFOSIZE 16

ADD_TYPEDEFS(tModelGeoset, MODELGEOSET);
ADD_TYPEDEFS(tModelKeyTrack, MODELKEYTRACK);
ADD_TYPEDEFS(tModelKeyFrame, MODELKEYFRAME);
ADD_TYPEDEFS(tModelNode, MODELNODE);
ADD_TYPEDEFS(tModelMaterialLayer, MODELLAYER);
ADD_TYPEDEFS(tModelSequence, MODELSEQUENCE);

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
    LPVECTOR3 lpVertices;
    LPVECTOR3 lpNormals;
    LPVECTOR2 lpTexcoord;
    struct tModelBounds *lpBounds;
    struct tModelBounds default_bounds;
    LPMODELGEOSET lpNext;
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
    struct tModelMaterial *lpNext;
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
    DWORD dwKeyframeCount;
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
    struct tModelGeosetAnim *lpNext;
    LPMODELKEYTRACK lpAlphas; // float
};

struct tModelNode {
    tModelObjectName name;
    DWORD objectId; // globally unique id, used as the index in the hierarchy. index into PIVT
    DWORD parentId; // parent MDLGENOBJECT's objectId or 0xFFFFFFFF if none
    DWORD flags;
    LPMODELKEYTRACK lpTranslation; // vec3
    LPMODELKEYTRACK lpRotation; // vec4
    LPMODELKEYTRACK lpScale; // vec3
    LPMODELNODE lpParent;
    struct tModelPivot *lpPivot;
    MATRIX4 localMatrix;
    MATRIX4 globalMatrix;
};

struct tModelBone {
    struct tModelNode node;
    struct tModelBone *lpNext;
    DWORD geoset_id;
    DWORD geoset_animation_id;
};

struct tModelHelper {
    struct tModelNode node;
    struct tModelHelper *lpNext;
};

struct tModelGlobalSequence {
    DWORD value;
};

struct tModel {
    struct tModelInfo info;
    LPMODELGEOSET lpGeosets;
    struct tModelTexture *lpTextures;
    LPMODELSEQUENCE lpSequences;
    struct tModelPivot *lpPivots;
    struct tModelMaterial *lpMaterials;
    struct tModelBone *lpBones;
    struct tModelGeosetAnim *lpGeosetAnims;
    struct tModelHelper *lpHelpers;
    struct tModelGlobalSequence *lpGlobalSequences;
    LPCMODELSEQUENCE lpCurrentAnimation;
    int numTextures;
    int numSequences;
    int numGlobalSequences;
    int numPivots;
};

#endif

