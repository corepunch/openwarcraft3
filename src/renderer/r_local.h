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
    for (GLenum Error = glGetError(); (GL_NO_ERROR != Error); Error = glGetError())\
    {\
        switch (Error)\
        {\
            case GL_INVALID_ENUM:      printf("\n%s\n\n", "GL_INVALID_ENUM"    ); assert(0); break;\
            case GL_INVALID_VALUE:     printf("\n%s\n\n", "GL_INVALID_VALUE"   ); assert(0); break;\
            case GL_INVALID_OPERATION: printf("\n%s\n\n", "GL_INVALID_OPERATION"); assert(0); break;\
            case GL_OUT_OF_MEMORY:     printf("\n%s\n\n", "GL_OUT_OF_MEMORY"   ); assert(0); break;\
            default:                                                                              break;\
        }\
    }\
}

#define R_Call(func, ...) func(__VA_ARGS__); GetError();
#define MAX_BONE_MATRICES 64

#include "../common/common.h"
#include "../client/renderer.h"

extern struct renderer_import ri;

typedef struct shader_program *LPSHADER;
typedef struct shader_program const *LPCSHADER;
typedef struct render_buffer *LPBUFFER;

struct vertex {
    VECTOR3 position;
    VECTOR2 texcoord;
    VECTOR2 texcoord2;
    struct color32 color;
    uint8_t skin[4];
    uint8_t boneWeight[4];
};

struct texture {
    GLuint texid;
    int width;
    int height;
    LPTEXTURE lpNext;
};

struct render_buffer {
    unsigned int vao, vbo;
};

struct shader_program {
    GLuint progid;
    int u_projection_matrix;
    int u_model_matrix;
    int u_texture;
    int u_shadowmap;
    int u_bones;
};

struct render_globals {
    struct viewDef viewDef;
    LPCTERRAIN world;
    LPCTEXTURE shadowmap;
    LPCTEXTURE waterTexture;
    LPCSHADER shaderStatic;
    LPCSHADER shaderSkin;
    LPBUFFER renbuf;
};

LPCSHADER R_InitShader(LPCSTR vertex_shader, LPCSTR fragment_shader);
void R_RegisterMap(LPCSTR szMapFileName);
int R_RegisterTextureFile(LPCSTR szTextureFileName);
LPTEXTURE R_LoadTexture(LPCSTR szTextureFileName);
void R_DrawEntities(void);
void R_DrawWorld(void);
void R_DrawAlphaSurfaces(void);
LPTEXTURE R_AllocateTexture(uint32_t dwWidth, uint32_t dwHeight);
void R_LoadTextureMipLevel(LPTEXTURE pTexture, DWORD dwLevel, struct color32* pPixels, uint32_t dwWidth, uint32_t dwHeight);
void R_BindTexture(LPCTEXTURE texture, int unit);
void RenderModel(struct render_entity const *lpEdict);
LPBUFFER R_MakeVertexArrayObject(struct vertex const *data, DWORD size);
void R_ReleaseVertexArrayObject(LPBUFFER lpBuffer);
LPCTEXTURE R_FindTextureByID(int texid);

// Models
LPMODEL R_LoadModel(LPCSTR szModelFilename);
void R_ReleaseModel(LPMODEL lpModel);

extern struct render_globals tr;

#endif
