#ifndef r_local_h
#define r_local_h

#include <SDL.h>
#include <SDL_opengl.h>

// TODO: M1 doesn't link without these includes

#if __APPLE__
#include <TargetConditionals.h>
#if !TARGET_OS_IPHONE && !TARGET_IPHONE_SIMULATOR
#include <OpenGL/gl3.h>
#else
#include <OpenGLES/ES3/gl.h>
#endif
#elif __linux__
#include <GLES3/gl3.h>
#endif

#define GetError()\
{\
    for ( GLenum Error = glGetError(); ( GL_NO_ERROR != Error ); Error = glGetError() )\
    {\
        switch ( Error )\
        {\
            case GL_INVALID_ENUM:      printf( "\n%s\n\n", "GL_INVALID_ENUM"      ); assert( 0 ); break;\
            case GL_INVALID_VALUE:     printf( "\n%s\n\n", "GL_INVALID_VALUE"     ); assert( 0 ); break;\
            case GL_INVALID_OPERATION: printf( "\n%s\n\n", "GL_INVALID_OPERATION" ); assert( 0 ); break;\
            case GL_OUT_OF_MEMORY:     printf( "\n%s\n\n", "GL_OUT_OF_MEMORY"     ); assert( 0 ); break;\
            default:                                                                              break;\
        }\
    }\
}

#define R_Call(func, ...) func(__VA_ARGS__); GetError();
#define MAX_BONE_MATRICES 64

#include "../common/common.h"
#include "../client/renderer.h"

extern struct RendererImport ri;

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

struct tVertex {
    struct vector3 position;
    struct vector2 texcoord;
    struct vector2 texcoord2;
    struct color32 color;
    uint8_t skin[4];
};

struct tRenBuf {
    unsigned int vao, vbo;
};

struct tTexture {
    GLuint texid;
    int width;
    int height;
    struct tTexture *lpNext;
};

struct tModelBounds {
    float radius;
    struct vector3 min;
    struct vector3 max;
};

struct tModelGeoset {
    struct vector3 *lpVertices;
    struct vector3 *lpNormals;
    struct vector2 *lpTexcoord;
    struct tModelBounds *lpBounds;
    struct tModelBounds default_bounds;
    struct tModelGeoset *lpNext;
    struct tRenBuf *lpBuffer;
    struct tModelGeosetAnim *lpGeosetAnim;
    int *lpMatrices;
    int *lpPrimitiveTypes;
    int *lpPrimitiveCounts;
    short *lpTriangles;
    char *lpVertexGroups;
    int *lpMatrixGroups;
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
    int numMatrixGroups;
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

#define MODELTRACKINFOSIZE 16
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

typedef struct {
    struct refdef refdef;
    struct Terrain const *world;
    struct tTexture const *shadowmap;
    struct tTexture const *waterTexture;
    struct tRenBuf *renbuf;
} trGlobals_t;

unsigned int R_InitShader(void);
void R_RegisterMap(LPCSTR szMapFileName);
int R_RegisterTextureFile(LPCSTR szTextureFileName);
struct tTexture *R_LoadTexture(LPCSTR szTextureFileName);
void R_DrawEntities(void);
void R_DrawWorld(void);
void R_DrawAlphaSurfaces(void);
struct tTexture *R_AllocateTexture(uint32_t dwWidth, uint32_t dwHeight);
void R_LoadTextureMipLevel(struct tTexture *pTexture, int dwLevel, struct color32* pPixels, uint32_t dwWidth, uint32_t dwHeight);
void R_BindTexture(struct tTexture const *texture, int unit);
void RenderModel(struct render_entity const *ent);
struct tRenBuf *R_MakeVertexArrayObject(struct tVertex const *data, int size);
void R_ReleaseVertexArrayObject(struct tRenBuf *lpBuffer);
struct tTexture const* R_FindTextureByID(int texid);

// Models
struct tModel *R_LoadModel(LPCSTR szModelFilename);
void R_ReleaseModel(struct tModel *lpModel);

extern trGlobals_t tr;

#endif
