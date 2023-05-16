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

struct tTexture {
    GLuint texid;
    int width;
    int height;
    struct tTexture *lpNext;
};

struct tRenBuf {
    unsigned int vao, vbo;
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
