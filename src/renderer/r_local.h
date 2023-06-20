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
#define SHADOW_TEXSIZE 1024
#define SHADOW_SCALE 1500
#define MAX_TEAMS 16
#define TEAM_MASK (MAX_TEAMS - 1)
#define PORTRAIT_SHADOW_SIZE 50
#define MAX_SKIN_BONES 8

#include "../common/common.h"
#include "../client/renderer.h"

extern refImport_t ri;

KNOWN_AS(shader_program, SHADER);
KNOWN_AS(render_buffer, BUFFER);
KNOWN_AS(vertex, VERTEX);

typedef struct vertex {
    VECTOR3 position;
    VECTOR2 texcoord;
    VECTOR2 texcoord2;
    VECTOR3 normal;
    COLOR32 color;
    BYTE skin[MAX_SKIN_BONES];
    BYTE boneWeight[MAX_SKIN_BONES];
} vertex_t;

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
    LPCTEXTURE teamGlow[MAX_TEAMS];
    LPCTEXTURE teamColor[MAX_TEAMS];
    model_t const *selectionCircle;
    LPCTEXTURE selectionCircleSmall;
    LPCTEXTURE selectionCircleMed;
    LPCTEXTURE selectionCircleLarge;
    sheetRow_t *terrainSheet;
    sheetRow_t *cliffSheet;
    DWORD depthMapFBO;
    DWORD depthMap;
};

LPCSHADER R_InitShader(LPCSTR vertex_shader, LPCSTR fragment_shader);
void R_RegisterMap(LPCSTR mapFileName);
int R_RegisterTextureFile(LPCSTR textureFileName);
LPCTEXTURE R_LoadTexture(LPCSTR textureFileName);
void R_ReleaseTexture(LPTEXTURE texture);
void R_DrawEntities(void);
void R_DrawWorld(void);
void R_DrawAlphaSurfaces(void);
void R_RenderFrame(viewDef_t const *viewDef);
LPTEXTURE R_AllocateTexture(DWORD width, DWORD height);
LPTEXTURE R_MakeSysFontTexture(void);
void R_LoadTextureMipLevel(LPCTEXTURE pTexture, DWORD level, LPCCOLOR32 pPixels, DWORD width, DWORD height);
void R_BindTexture(LPCTEXTURE texture, DWORD unit);
void R_RenderModel(renderEntity_t const *edict);
bool R_TraceModel(renderEntity_t const *edict, LPCLINE3 line);
void R_ReleaseVertexArrayObject(LPBUFFER buffer);
LPCTEXTURE R_FindTextureByID(DWORD textureID);
bool R_IsPointVisible(LPCVECTOR3 point, float fThreshold);
void R_DrawPortrait(model_t const *model, LPCRECT viewport);
void R_RenderSplat(LPCVECTOR2 position, float radius, LPCTEXTURE texture);

// r_ents.c
bool R_TraceEntity(viewDef_t const *viewdef, float x, float y, LPDWORD number);
bool R_TraceLocation(viewDef_t const *viewdef, float x, float y, LPVECTOR3 point);
void R_GetEntityMatrix(renderEntity_t const *entity, LPMATRIX4 matrix);
LINE3 R_LineForScreenPoint(viewDef_t const *viewdef, float x, float y);
DWORD R_EntitiesInRect(viewDef_t const *viewdef, LPCRECT rect, DWORD max, LPDWORD array);

// r_mdx.c
model_t *R_LoadModel(LPCSTR modelFilename);
void R_ReleaseModel(model_t *model);

size2_t R_GetWindowSize(void);

// r_buffer.c
VERTEX *R_AddQuad(VERTEX *buffer, LPCRECT screen, LPCRECT uv, COLOR32 color);
VERTEX *R_AddStrip(VERTEX *buffer, LPCRECT screen, COLOR32 color);
VERTEX *R_AddWireBox(VERTEX *buffer, LPCBOX3 box, COLOR32 color);
LPBUFFER R_MakeVertexArrayObject(LPCVERTEX vertices, DWORD size);
void R_DrawBuffer(LPCBUFFER buffer, DWORD num_vertices);

// r_draw.c
void R_PrintSysText(LPCSTR string, DWORD x, DWORD y, COLOR32 color);
void R_DrawImage(LPCTEXTURE texture, LPCRECT screen, LPCRECT uv);
void R_DrawPic(LPCTEXTURE texture, float x, float y);
void R_DrawSelectionRect(LPCRECT rect, COLOR32 color);
void R_DrawBoundingBox(LPCBOX3 box, LPCMATRIX4 matrix, COLOR32 color);

// r_font.c
LPFONT R_LoadFont(LPCSTR filename, DWORD size);
void R_DrawText(drawText_t const *drawText);

extern struct render_globals tr;

#endif
