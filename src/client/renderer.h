#ifndef renderer_h
#define renderer_h

#include "../common/common.h"

struct renderer_import {
    HANDLE (*FileOpen)(LPCSTR szFileName);
    bool (*FileExtract)(LPCSTR szToExtract, LPCSTR szExtracted);
    bool (*FileClose)(HANDLE hFile);
    void *(*MemAlloc)(long size);
    void (*MemFree)(void *);
};

struct render_entity {
    struct vector3 origin;
    float angle;
    float scale;
    LPCMODEL model;
    LPTEXTURE skin;
    int frame;
};

struct refdef {
    float fov;
    struct vector3 vieworg;
    struct vector3 viewangles;
    int time;
    int num_entities;
    struct render_entity *entities;
    struct matrix4 projection_matrix;
};

struct Renderer {
    void (*Init)(DWORD dwWidth, DWORD dwHeight);
    void (*Shutdown)(void);
    void (*RegisterMap)(char const *szMapFileName);
    void (*RenderFrame)(struct refdef const *lpRefDef);
    LPTEXTURE (*LoadTexture)(LPCSTR szTextureFileName);
    LPMODEL (*LoadModel)(LPCSTR szModelFilename);
    void (*ReleaseModel)(LPMODEL lpModel);
    void (*BeginFrame)(void);
    void (*EndFrame)(void);
    void (*DrawPic)(LPCTEXTURE lpTexture);
};

struct Renderer *Renderer_Init(struct renderer_import *pImport);

#endif
