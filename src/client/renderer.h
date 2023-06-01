#ifndef renderer_h
#define renderer_h

#include "../common/common.h"

enum {
   RF_SELECTED = 1,
};

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
    VECTOR3 target;
    VECTOR3 eye;
    VECTOR3 angles;
    float distance;
    float fov;
    float znear;
    float zfar;
} viewCamera_t;

typedef struct {
    VECTOR3 origin;
    model_t const *model;
    LPTEXTURE skin;
    DWORD team;
    DWORD frame;
    DWORD oldframe;
    DWORD flags;
    float angle;
    float scale;
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
    void (*Init)(DWORD width, DWORD height);
    void (*Shutdown)(void);
    void (*RegisterMap)(LPCSTR mapFileName);
    void (*RenderFrame)(viewDef_t const *refDef);
    LPCTEXTURE (*LoadTexture)(LPCSTR textureFileName);
    model_t *(*LoadModel)(LPCSTR modelFilename);
    struct size2 (*GetWindowSize)(void);
    void (*ReleaseModel)(model_t *model);
    void (*BeginFrame)(void);
    void (*EndFrame)(void);
    void (*PrintText)(LPCSTR string, DWORD x, DWORD y, COLOR32 color);
    void (*DrawSelectionRect)(LPCRECT rect, COLOR32 color);
    void (*DrawPic)(LPCTEXTURE texture, DWORD x, DWORD y);
    void (*DrawImage)(LPCTEXTURE texture, LPCRECT screen, LPCRECT uv);
    void (*DrawPortrait)(model_t const *model, LPCRECT viewport);

#ifdef DEBUG_PATHFINDING
    void (*SetPathTexture)(LPCCOLOR32 debugTexture);
#endif
} refExport_t;

refExport_t R_GetAPI(refImport_t imp);

#endif
