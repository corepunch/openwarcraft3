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
#define NUM_SELECTION_CIRCLES 3
#define NUM_RECT_VERTICES 6

#include "../common/common.h"
#include "../client/renderer.h"

extern refImport_t ri;

KNOWN_AS(shader_program, SHADER);
KNOWN_AS(render_buffer, BUFFER);
KNOWN_AS(render_target, RENDERTARGET);
KNOWN_AS(vertex, VERTEX);

typedef struct vertex {
    VECTOR3 position;
    VECTOR2 texcoord;
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
    DWORD uViewProjectionMatrix;
    DWORD uModelMatrix;
    DWORD uLightMatrix;
    DWORD uNormalMatrix;
    DWORD uTextureMatrix;
    DWORD uTexture;
    DWORD uShadowmap;
    DWORD uFogOfWar;
    DWORD uBones;
    DWORD uUseDiscard;
    DWORD uEyePosition;
    DWORD uActiveGlow;
};

struct render_target {
    DWORD buffer;
    DWORD texture;
};

typedef enum {
    TRACK_NO_INTERP = 0x0,
    TRACK_LINEAR = 0x1,
    TRACK_HERMITE = 0x2,
    TRACK_BEZIER = 0x3,
    NUM_TRACK_TYPES = 0x4,
} MODELKEYTRACKTYPE;

typedef enum {
    TDATA_INT1,
    TDATA_FLOAT1,
    TDATA_FLOAT3,
    TDATA_FLOAT4,
} MODELKEYTRACKDATATYPE;

enum {
    TEX_SHADOWMAP,
    TEX_WATER,
    TEX_FONT,
    TEX_WHITE,
    TEX_BLACK,
    TEX_TEAM_GLOW,
    TEX_TEAM_COLOR = TEX_TEAM_GLOW + MAX_TEAMS,
    TEX_SELECTION_CIRCLE = TEX_TEAM_COLOR + MAX_TEAMS,
    TEX_COUNT = TEX_SELECTION_CIRCLE + NUM_SELECTION_CIRCLES,
};

enum {
    RT_DEPTHMAP,
    RT_COUNT,
};

enum {
    RBUF_TEMP1,
    RBUF_COUNT
};

enum {
    MODEL_SELECTION,
    MODEL_COUNT,
};

enum {
    SHEET_TERRAIN,
    SHEET_CLIFF,
    SHEET_COUNT,
};

struct render_globals {
    viewDef_t viewDef;
    LPCWAR3MAP world;
    LPTEXTURE texture[TEX_COUNT];
    LPSHADER shader[SHADER_COUNT];
    LPBUFFER buffer[RBUF_COUNT];
    LPMODEL model[MODEL_COUNT];
    LPRENDERTARGET rt[RT_COUNT];
    sheetRow_t *sheet[SHEET_COUNT];
    size2_t drawableSize;
};

void R_RegisterMap(LPCSTR mapFileName);
int R_RegisterTextureFile(LPCSTR textureFileName);
LPTEXTURE R_LoadTexture(LPCSTR textureFileName);
void R_ReleaseTexture(LPTEXTURE texture);
void R_DrawWorld(void);
void R_DrawAlphaSurfaces(void);
void R_RenderFrame(viewDef_t const *viewDef);
LPTEXTURE R_AllocateTexture(DWORD width, DWORD height);
LPTEXTURE R_MakeSysFontTexture(void);
void R_LoadTextureMipLevel(LPCTEXTURE pTexture, DWORD level, LPCCOLOR32 pPixels, DWORD width, DWORD height);
void R_BindTexture(LPCTEXTURE texture, DWORD unit);
void R_RenderModel(renderEntity_t const *edict);
bool MDLX_TraceModel(renderEntity_t const *edict, LPCLINE3 line);
void R_ReleaseVertexArrayObject(LPBUFFER buffer);
LPCTEXTURE R_FindTextureByID(DWORD textureID);
void R_DrawPortrait(LPCMODEL model, LPCRECT viewport);
void R_RenderSplat(LPCVECTOR2 position, float radius, LPCTEXTURE texture, LPCSHADER shader, COLOR32 color);

// r_shader.c
LPSHADER R_InitShader(LPCSTR vs_default, LPCSTR fs_default);
void R_ReleaseShader(LPSHADER shader);

// r_main.c
void R_RenderShadowMap(void);
void R_RenderView(void);

// r_ents.c
bool R_TraceEntity(viewDef_t const *viewdef, float x, float y, LPDWORD number);
bool R_TraceLocation(viewDef_t const *viewdef, float x, float y, LPVECTOR3 point);
void R_GetEntityMatrix(renderEntity_t const *entity, LPMATRIX4 matrix);
LINE3 R_LineForScreenPoint(viewDef_t const *viewdef, float x, float y);
DWORD R_EntitiesInRect(viewDef_t const *viewdef, LPCRECT rect, DWORD max, LPDWORD array);
void R_DrawEntities(void);
void R_RenderOverlays(void);
FLOAT R_GetHeightAtPoint(FLOAT x, FLOAT y);

// r_mdx.c
LPMODEL R_LoadModel(LPCSTR modelFilename);
void R_ReleaseModel(LPMODEL model);

size2_t R_GetWindowSize(void);

// r_buffer.c
VERTEX *R_AddQuad(VERTEX *buffer, LPCRECT screen, LPCRECT uv, COLOR32 color, float z);
VERTEX *R_AddStrip(VERTEX *buffer, LPCRECT screen, COLOR32 color);
VERTEX *R_AddWireBox(VERTEX *buffer, LPCBOX3 box, COLOR32 color);
LPBUFFER R_MakeVertexArrayObject(LPCVERTEX vertices, DWORD size);
void R_DrawBuffer(LPCBUFFER buffer, DWORD num_vertices);

// r_draw.c
void R_PrintSysText(LPCSTR string, DWORD x, DWORD y, COLOR32 color);
void R_DrawImage(LPCTEXTURE texture, LPCRECT screen, LPCRECT uv, COLOR32 color);
void R_DrawImageEx(LPCDRAWIMAGE drawImage);
void R_DrawPic(LPCTEXTURE texture, float x, float y);
void R_DrawSelectionRect(LPCRECT rect, COLOR32 color);
void R_DrawBoundingBox(LPCBOX3 box, LPCMATRIX4 matrix, COLOR32 color);
void R_DrawWireRect(LPCRECT rect, COLOR32 color);

// r_font.c
LPFONT R_LoadFont(LPCSTR filename, DWORD size);
VECTOR2 R_GetTextSize(LPCDRAWTEXT drawText);
void R_DrawText(LPCDRAWTEXT drawText);

// r_image.c
LPRENDERTARGET R_AllocateRenderTexture(GLsizei width, GLsizei height, GLenum format, GLenum type, GLenum attachment);
void R_ReleaseRenderTexture(LPRENDERTARGET rt);

// r_fogofwar.c
void R_InitFogOfWar(DWORD width, DWORD height);
void R_ShutdownFogOfWar(void);
void R_RenderFogOfWar(void);
DWORD R_GetFogOfWarTexture(void);

// r_particles.c
void R_InitParticles(void);
void R_ShutdownParticles(void);
void R_DrawParticles(void);
cparticle_t *R_SpawnParticle(void);

// r_war3map.c
VECTOR2 GetWar3MapSize(LPCWAR3MAP war3Map);

// loaders
mdxModel_t *R_LoadModelMDLX(void *buffer, DWORD size);
m3Model_t *R_LoadModelM3(void *buffer, DWORD size);

extern struct render_globals tr;

#endif
