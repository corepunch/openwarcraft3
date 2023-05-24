#ifndef renderer_h
#define renderer_h

#include "../common/common.h"

enum {
   RF_SELECTED = 1,
};

struct renderer_import {
    HANDLE (*FileOpen)(LPCSTR fileName);
    bool (*FileExtract)(LPCSTR toExtract, LPCSTR extracted);
    bool (*FileClose)(HANDLE file);
    HANDLE (*MemAlloc)(long size);
    void (*MemFree)(HANDLE);
    HANDLE (*ParseSheet)(LPCSTR sheetFilename, LPCSHEETLAYOUT layout, DWORD elementSize);
    void (*error) (char *fmt, ...);
};

typedef struct {
    VECTOR3 origin;
    float angle;
    float scale;
    LPCMODEL model;
    LPTEXTURE skin;
    DWORD team;
    DWORD frame;
    DWORD oldframe;
    DWORD flags;
} renderEntity_t;

typedef struct {
    float fov;
    VECTOR3 vieworg;
    VECTOR3 viewangles;
    float lerpfrac;
    DWORD time;
    DWORD num_entities;
    renderEntity_t *entities;
    MATRIX4 projection_matrix;
    MATRIX4 light_matrix;
} viewDef_t;

struct Renderer {
    void (*Init)(DWORD width, DWORD height);
    void (*Shutdown)(void);
    void (*RegisterMap)(LPCSTR mapFileName);
    void (*RenderFrame)(viewDef_t const *refDef);
    LPTEXTURE (*LoadTexture)(LPCSTR textureFileName);
    LPMODEL (*LoadModel)(LPCSTR modelFilename);
    struct size2 (*GetWindowSize)(void);
    void (*ReleaseModel)(LPMODEL model);
    void (*BeginFrame)(void);
    void (*EndFrame)(void);
    void (*PrintText)(LPCSTR string, DWORD x, DWORD y, COLOR32 color);
    void (*DrawSelectionRect)(struct rect const *rect, COLOR32 color);
    void (*DrawPic)(LPCTEXTURE texture, DWORD x, DWORD y);

#ifdef DEBUG_PATHFINDING
    void (*SetPathTexture)(LPCCOLOR32 debugTexture);
#endif
};

struct Renderer *Renderer_Init(struct renderer_import *pImport);

#endif
