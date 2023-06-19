#ifndef renderer_h
#define renderer_h

#include "../common/common.h"

enum {
    RDF_NOWORLDMODEL = 1,
    RDF_NOFRUSTUMCULL = 2,
};

typedef struct {
    HANDLE (*FileOpen)(LPCSTR fileName);
    bool (*FileExtract)(LPCSTR toExtract, LPCSTR extracted);
    void (*FileClose)(HANDLE file);
    HANDLE (*MemAlloc)(long size);
    void (*MemFree)(HANDLE);
    HANDLE (*ParseSheet)(LPCSTR sheetFilename, LPCSHEETLAYOUT layout, DWORD elementSize);
    void (*error)(LPCSTR fmt, ...);
} refImport_t;

typedef struct {
    VECTOR3 target;
    VECTOR3 angles;
} viewLight_t;

typedef struct {
    VECTOR3 origin;
    VECTOR3 eye;
    VECTOR3 viewangles;
    float distance;
    float fov;
    float znear;
    float zfar;
} viewCamera_t;

typedef struct {
    VECTOR3 origin;
    model_t const *model;
    LPCTEXTURE skin;
    DWORD number;
    DWORD team;
    DWORD frame;
    DWORD oldframe;
    DWORD flags;
    float angle;
    float scale;
    float radius;
} renderEntity_t;

typedef struct {
    viewCamera_t camera;
    RECT viewport;
    RECT scissor;
    DWORD time;
    float lerpfrac;
    DWORD num_entities;
    renderEntity_t *entities;
    MATRIX4 projectionMatrix;
    MATRIX4 lightMatrix;
    DWORD rdflags;
} viewDef_t;

typedef struct {
    LPCFONT font;
    LPCSTR text;
    RECT rect;
    color32_t color;
    uiFontJustificationH_t halign;
    uiFontJustificationV_t valign;
} drawText_t;

typedef struct {
    void (*Init)(DWORD width, DWORD height);
    void (*Shutdown)(void);
    void (*RegisterMap)(LPCSTR mapFileName);
    void (*RenderFrame)(viewDef_t const *viewdef);
    LPCTEXTURE (*LoadTexture)(LPCSTR fileName);
    model_t *(*LoadModel)(LPCSTR filename);
    LPFONT (*LoadFont)(LPCSTR filename, DWORD size);
    size2_t (*GetWindowSize)(void);
    size2_t (*GetTextureSize)(LPCTEXTURE texture);
    void (*ReleaseModel)(model_t *model);
    void (*BeginFrame)(void);
    void (*EndFrame)(void);
    void (*PrintSysText)(LPCSTR string, DWORD x, DWORD y, COLOR32 color);
    void (*DrawSelectionRect)(LPCRECT rect, COLOR32 color);
    void (*DrawPic)(LPCTEXTURE texture, float x, float y);
    void (*DrawImage)(LPCTEXTURE texture, LPCRECT screen, LPCRECT uv);
    void (*DrawPortrait)(model_t const *model, LPCRECT viewport);
    void (*DrawText)(drawText_t const *drawText);
    renderEntity_t *(*Trace)(viewDef_t const *viewdef, float x, float y);

#ifdef DEBUG_PATHFINDING
    void (*SetPathTexture)(LPCCOLOR32 debugTexture);
#endif
} refExport_t;

refExport_t R_GetAPI(refImport_t imp);

#endif
