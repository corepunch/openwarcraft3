#ifndef renderer_h
#define renderer_h

#include "../common/common.h"

KNOWN_AS(drawText_s, DRAWTEXT);

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
    VECTOR3 viewangles;
    float distance;
    float fov;
    float znear;
    float zfar;
} viewCamera_t;

typedef struct {
    VECTOR3 origin;
    LPCMODEL model;
    LPCTEXTURE skin;
    LPCTEXTURE splat;
    DWORD number;
    DWORD team;
    DWORD frame;
    DWORD oldframe;
    DWORD flags;
    float angle;
    float scale;
    float radius;
    float splatsize;
    float health;
} renderEntity_t;

typedef struct {
    viewCamera_t camera;
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
    DWORD rdflags;
    FRUSTUM3 frustum;
} viewDef_t;

struct drawText_s {
    LPCFONT font;
    LPCSTR text;
    RECT rect;
    COLOR32 color;
    FLOAT textWidth;
    FLOAT lineHeight;
    BOOL wordWrap;
    uiFontJustificationH_t halign;
    uiFontJustificationV_t valign;
    LPCTEXTURE *icons;
};

typedef struct {
    void (*Init)(DWORD width, DWORD height);
    void (*Shutdown)(void);
    void (*RegisterMap)(LPCSTR mapFileName);
    void (*RenderFrame)(viewDef_t const *viewdef);
    LPTEXTURE (*LoadTexture)(LPCSTR fileName);
    LPMODEL (*LoadModel)(LPCSTR filename);
    LPFONT (*LoadFont)(LPCSTR filename, DWORD size);
    size2_t (*GetWindowSize)(void);
    size2_t (*GetTextureSize)(LPCTEXTURE texture);
    void (*ReleaseModel)(LPMODEL model);
    void (*BeginFrame)(void);
    void (*EndFrame)(void);
    void (*PrintSysText)(LPCSTR string, DWORD x, DWORD y, COLOR32 color);
    void (*DrawSelectionRect)(LPCRECT rect, COLOR32 color);
    void (*DrawPic)(LPCTEXTURE texture, float x, float y);
    void (*DrawImage)(LPCTEXTURE texture, LPCRECT screen, LPCRECT uv, COLOR32 color);
    void (*DrawImageEx)(LPCTEXTURE texture, LPCRECT screen, LPCRECT uv, COLOR32 color, BOOL rotate);
    void (*DrawPortrait)(LPCMODEL model, LPCRECT viewport);
    void (*DrawText)(LPCDRAWTEXT drawText);
    VECTOR2 (*GetTextSize)(LPCDRAWTEXT drawText);

    bool (*TraceEntity)(viewDef_t const *viewdef, float x, float y, LPDWORD number);
    bool (*TraceLocation)(viewDef_t const *viewdef, float x, float y, LPVECTOR3 point);
    DWORD (*EntitiesInRect)(viewDef_t const *viewdef, LPCRECT rect, DWORD max, LPDWORD array);

#ifdef DEBUG_PATHFINDING
    void (*SetPathTexture)(LPCCOLOR32 debugTexture);
#endif
} refExport_t;

refExport_t R_GetAPI(refImport_t imp);

#endif
