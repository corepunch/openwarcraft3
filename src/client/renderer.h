#ifndef renderer_h
#define renderer_h

#include "../common/common.h"

KNOWN_AS(viewDef, VIEWDEF);
KNOWN_AS(render_entity, RENDERENTITY);

struct renderer_import {
    HANDLE (*FileOpen)(LPCSTR szFileName);
    bool (*FileExtract)(LPCSTR szToExtract, LPCSTR szExtracted);
    bool (*FileClose)(HANDLE hFile);
    HANDLE (*MemAlloc)(long size);
    void (*MemFree)(HANDLE);
    HANDLE (*ParseSheet)(LPCSTR szSheetFilename, LPCSHEETLAYOUT lpLayout, DWORD dwElementSize);
};

struct render_entity {
    VECTOR3 origin;
    float angle;
    float scale;
    LPCMODEL model;
    LPTEXTURE skin;
    DWORD frame;
    DWORD oldframe;
};

struct viewDef {
    float fov;
    VECTOR3 vieworg;
    VECTOR3 viewangles;
    float lerpfrac;
    DWORD time;
    DWORD num_entities;
    LPRENDERENTITY entities;
    MATRIX4 projection_matrix;
    MATRIX4 light_matrix;
};

struct Renderer {
    void (*Init)(DWORD dwWidth, DWORD dwHeight);
    void (*Shutdown)(void);
    void (*RegisterMap)(LPCSTR szMapFileName);
    void (*RenderFrame)(LPCVIEWDEF lpRefDef);
    LPTEXTURE (*LoadTexture)(LPCSTR szTextureFileName);
    LPMODEL (*LoadModel)(LPCSTR szModelFilename);
    struct size2 (*GetWindowSize)(void);
    void (*ReleaseModel)(LPMODEL lpModel);
    void (*BeginFrame)(void);
    void (*EndFrame)(void);
    void (*DrawPic)(LPCTEXTURE lpTexture);
};

struct Renderer *Renderer_Init(struct renderer_import *pImport);

#endif
