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
#define MAX_TEAMS 16
#define TEAM_MASK (MAX_TEAMS - 1)

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
    LPTEXTURE next;
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
    viewDef_t viewDef;
    LPCWAR3MAP world;
    LPCTEXTURE shadowmap;
    LPCTEXTURE waterTexture;
    LPCTEXTURE sysFont;
    LPCTEXTURE blackTexture;
    LPCTEXTURE whiteTexture;
    LPCSHADER shaderStatic;
    LPCSHADER shaderSkin;
    LPCSHADER shaderUI;
    LPCBUFFER renbuf;
    LPCMODEL selectionCircle;
    LPCTEXTURE teamGlow[MAX_TEAMS];
    LPCTEXTURE teamColor[MAX_TEAMS];
    unsigned int depthMapFBO;
    unsigned int depthMap;
};

LPCSHADER R_InitShader(LPCSTR vertex_shader, LPCSTR fragment_shader);
void R_RegisterMap(LPCSTR mapFileName);
int R_RegisterTextureFile(LPCSTR textureFileName);
LPTEXTURE R_LoadTexture(LPCSTR textureFileName);
void R_DrawEntities(void);
void R_DrawWorld(void);
void R_DrawAlphaSurfaces(void);
LPTEXTURE R_AllocateTexture(DWORD width, DWORD height);
LPTEXTURE R_MakeSysFontTexture(void);
void R_LoadTextureMipLevel(LPCTEXTURE pTexture, DWORD level, LPCCOLOR32 pPixels, DWORD width, DWORD height);
void R_BindTexture(LPCTEXTURE texture, DWORD unit);
void RenderModel(renderEntity_t const *edict);
void R_ReleaseVertexArrayObject(LPBUFFER buffer);
LPCTEXTURE R_FindTextureByID(DWORD textureID);
bool R_IsPointVisible(LPCVECTOR3 point, float fThreshold);

// VertexArrayObject
LPBUFFER R_MakeVertexArrayObject(LPCVERTEX vertices, DWORD size);
void R_DrawBuffer(LPCBUFFER buffer, DWORD num_vertices);

// Models
LPMODEL R_LoadModel(LPCSTR modelFilename);
void R_ReleaseModel(LPMODEL model);

struct size2 R_GetWindowSize(void);

extern struct render_globals tr;

#endif
