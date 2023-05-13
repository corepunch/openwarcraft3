#ifndef renderer_h
#define renderer_h

#include "../common/common.h"

struct RendererImport {
    HANDLE (*FileOpen)(LPCSTR szFileName);
    bool (*FileExtract)(LPCSTR szToExtract, LPCSTR szExtracted);
    bool (*FileClose)(HANDLE hFile);
    void *(*MemAlloc)(long size);
    void (*MemFree)(void *);
};

struct render_entity {
    struct vector3 postion;
    float angle;
    struct vector3 scale;
    struct tModel *model;
    struct tTexture *skin;
};

struct refdef {
    float fov;
    struct vector3 vieworg;
    struct vector3 viewangles;
    int time;
    int num_entities;
    struct render_entity *entities;
};

struct Renderer {
    void (*Init)(DWORD dwWidth, DWORD dwHeight);
    void (*Shutdown)(void);
    void (*RegisterMap)(char const *szMapFileName);
    void (*RenderFrame)(struct refdef const *lpRefDef);
    struct tTexture *(*LoadTexture)(LPCSTR szTextureFileName);
    struct tModel *(*LoadModel)(LPCSTR szModelFilename);
    void (*BeginFrame)(void);
    void (*EndFrame)(void);
    void (*DrawPic)(struct tTexture const *lpTexture);
};

struct Renderer *Renderer_Init(struct RendererImport *pImport);

#endif
