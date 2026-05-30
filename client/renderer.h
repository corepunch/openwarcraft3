#ifndef renderer_h
#define renderer_h

#include "../common/renderer_types.h"

KNOWN_AS(modelInfo_s, MODELINFO);

#define MODELINFO_MAX_TEXTURES 256

typedef struct {
    // Quake 3-style file API: renderer is archive-agnostic
    int (*FS_ReadFile)(LPCSTR name, void **buf);  // Returns file size, allocates buf
    void (*FS_FreeFile)(void *buf);
    bool (*FileExtract)(LPCSTR toExtract, LPCSTR extracted);
    
    HANDLE (*MemAlloc)(long size);
    void (*MemFree)(HANDLE);
    sheetRow_t *(*ReadSheet)(LPCSTR sheetFilename);
    LPCSTR (*FindSheetCell)(sheetRow_t *sheet, LPCSTR row, LPCSTR column);
    void (*error)(LPCSTR fmt, ...);
} refImport_t;

typedef struct {
    VECTOR3 target;
    VECTOR3 angles;
} viewLight_t;

typedef struct {
    VECTOR3 origin;
    VECTOR3 eye;
    QUATERNION viewquat;
    VECTOR3 viewangles;
    float distance;
    float fov;
    float znear;
    float zfar;
} viewCamera_t;

typedef struct {
    VECTOR3 origin;
    LPCMODEL model;
    LPCMODEL attached_model;
    LPCTEXTURE skin;
    LPCTEXTURE splat;
    LPCTEXTURE shadow;
    DWORD number;
    DWORD team;
#ifdef WOW
    DWORD appearance;
    DWORD equipment;
#endif
    DWORD frame;
    DWORD oldframe;
    DWORD flags;
    float angle;
    VECTOR3 rotation;
    float scale;
    float radius;
    float splatsize;
    float shadow_x;
    float shadow_y;
    float shadow_w;
    float shadow_h;
} renderEntity_t;

typedef struct {
    viewCamera_t camerastate[2];
    RECT viewport;
    RECT scissor;
    DWORD time;
    DWORD deltaTime;
    float lerpfrac;
    DWORD num_entities;
    renderEntity_t *entities;
    MATRIX4 viewProjectionMatrix;
    MATRIX4 lightMatrix;
    MATRIX4 textureMatrix;
    DWORD player;
    DWORD rdflags;
    FRUSTUM3 frustum;
} viewDef_t;

struct modelInfo_s {
    DWORD textureCount;
    LPCSTR texturePaths[MODELINFO_MAX_TEXTURES];
    RECT textureUVRect;
    BOOL hasTextureUVRect;
};

typedef struct {
    void (*Init)(DWORD width, DWORD height);
    void (*Shutdown)(void);
    void (*RegisterMap)(LPCSTR mapFileName);
    void (*RenderFrame)(viewDef_t const *viewdef);
    void (*SetFogOfWarData)(DWORD width, DWORD height, BYTE const *data);
    LPTEXTURE (*LoadTexture)(LPCSTR fileName);
    LPMODEL (*LoadModel)(LPCSTR filename);
    LPFONT (*LoadFont)(LPCSTR filename, DWORD size);
    size2_t (*GetWindowSize)(void);
    size2_t (*GetTextureSize)(LPCTEXTURE texture);
    void (*ReleaseTexture)(LPTEXTURE texture);
    void (*ReleaseModel)(LPMODEL model);
    void (*BeginFrame)(void);
    void (*EndFrame)(void);
    void (*PrintSysText)(LPCSTR string, DWORD x, DWORD y, COLOR32 color);
    void (*DrawSelectionRect)(LPCRECT rect, COLOR32 color);
    void (*DrawPic)(LPCTEXTURE texture, float x, float y);
    void (*DrawImage)(LPCTEXTURE texture, LPCRECT screen, LPCRECT uv, COLOR32 color);
    void (*DrawImageEx)(LPCDRAWIMAGE drawImage);
    void (*DrawMinimap)(LPCRECT screen);
    void (*DrawLoadingIndicator)(LPCRECT rect, DWORD time, COLOR32 color);
    void (*DrawPortrait)(LPCMODEL model, LPCRECT viewport, LPCSTR anim);
    void (*DrawSprite)(LPCMODEL model, LPCSTR anim, float x, float y);
    void (*DrawText)(LPCDRAWTEXT drawText);
    VECTOR2 (*GetTextSize)(LPCDRAWTEXT drawText);
    bool (*GetModelInfo)(LPMODEL model, LPMODELINFO info);

    void (*DrawBoundingBox)(LPCBOX3 box, LPCMATRIX4 modelMatrix, LPCMATRIX4 vpMatrix, COLOR32 color);
    FLOAT (*GetHeightAtPoint)(float x, float y);
    bool (*TraceEntity)(viewDef_t const *viewdef, float x, float y, LPDWORD number);
    bool (*TraceLocation)(viewDef_t const *viewdef, float x, float y, LPVECTOR3 point);
    DWORD (*EntitiesInRect)(viewDef_t const *viewdef, LPCRECT rect, DWORD max, LPDWORD array);

#ifdef DEBUG_PATHFINDING
    void (*SetPathTexture)(LPCCOLOR32 debugTexture);
#endif
} refExport_t;

typedef refExport_t *LPRENDERER;
typedef refExport_t const *LPCRENDERER;

refExport_t R_GetAPI(refImport_t imp);
refExport_t R_StdoutGetAPI(refImport_t imp);

#endif
