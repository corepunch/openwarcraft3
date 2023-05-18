#ifndef r_local_h
#define r_local_h

#include <SDL.h>
#include <SDL_opengl.h>
#include <StormLib.h>

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
#define SHADOW_WIDTH 1024
#define SHADOW_HEIGHT 1024

#include "../common/common.h"
#include "../client/renderer.h"

extern struct renderer_import ri;

KNOWN_AS(shader_program, SHADER);
KNOWN_AS(render_buffer, BUFFER);
KNOWN_AS(vertex, VERTEX);

struct vertex {
    VECTOR3 position;
    VECTOR2 texcoord;
    VECTOR2 texcoord2;
    VECTOR3 normal;
    struct color32 color;
    BYTE skin[4];
    BYTE boneWeight[4];
};

struct texture {
    DWORD texid;
    DWORD width;
    DWORD height;
    LPTEXTURE lpNext;
};

struct render_buffer {
    DWORD vao;
    DWORD vbo;
};

struct shader_program {
    DWORD progid;
    DWORD uProjectionMatrix;
    DWORD uModelMatrix;
    DWORD uLightMatrix;
    DWORD uNormalMatrix;
    DWORD uTexture;
    DWORD uShadowmap;
    DWORD uBones;
    DWORD uUseDiscard;
};

struct render_globals {
    struct viewDef viewDef;
    LPCWAR3MAP world;
    LPCTEXTURE shadowmap;
    LPCTEXTURE waterTexture;
    LPCSHADER shaderStatic;
    LPCSHADER shaderSkin;
    LPCBUFFER renbuf;
    unsigned int depthMapFBO;
    unsigned int depthMap;
};

LPCSHADER R_InitShader(LPCSTR vertex_shader, LPCSTR fragment_shader);
void R_RegisterMap(LPCSTR szMapFileName);
int R_RegisterTextureFile(LPCSTR szTextureFileName);
LPTEXTURE R_LoadTexture(LPCSTR szTextureFileName);
void R_DrawEntities(void);
void R_DrawWorld(void);
void R_DrawAlphaSurfaces(void);
LPTEXTURE R_AllocateTexture(DWORD dwWidth, DWORD dwHeight);
void R_LoadTextureMipLevel(LPTEXTURE pTexture, DWORD dwLevel, LPCCOLOR32 pPixels, DWORD dwWidth, DWORD dwHeight);
void R_BindTexture(LPCTEXTURE lpTexture, DWORD dwUnit);
void RenderModel(LPCRENDERENTITY lpEdict);
void R_ReleaseVertexArrayObject(LPBUFFER lpBuffer);
LPCTEXTURE R_FindTextureByID(DWORD dwTextureID);
bool R_IsPointVisible(LPCVECTOR3 point, float fThreshold);

// VertexArrayObject
LPCBUFFER R_MakeVertexArrayObject(LPCVERTEX lpVertices, DWORD dwSize);
void R_DrawBuffer(LPCBUFFER lpBuffer, DWORD numVertices);

// Models
LPMODEL R_LoadModel(LPCSTR szModelFilename);
void R_ReleaseModel(LPMODEL lpModel);

struct size2 R_GetWindowSize(void);

extern struct render_globals tr;

#endif
