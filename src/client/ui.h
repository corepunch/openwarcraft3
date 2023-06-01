#ifndef ui_h
#define ui_h

#include "../common/common.h"

typedef enum {
    UI_REFRESH_CONSOLE,
} uiEvent_t;

typedef enum {
    UI_LEFT_MOUSE_DOWN,
    UI_LEFT_MOUSE_UP,
    UI_LEFT_MOUSE_DRAGGED,
    UI_RIGHT_MOUSE_DOWN,
    UI_RIGHT_MOUSE_UP,
    UI_RIGHT_MOUSE_DRAGGED,
    NUM_UI_MOUSE_EVENTS
} uiMouseEvent_t;

typedef struct {
    HANDLE (*FileOpen)(LPCSTR fileName);
    void (*FileClose)(HANDLE file);
    LPSTR (*ReadFileIntoString)(LPCSTR fileName);
    HANDLE (*MemAlloc)(long size);
    void (*MemFree)(HANDLE);
    void (*error)(LPCSTR fmt, ...);
    LPCSTR (*GetConfigString)(DWORD index);
    entityState_t const *(*GetSelectedEntity)(void);
    LPCTEXTURE (*LoadTexture)(LPCSTR textureFileName);
    LPCTEXTURE (*GetTextureByIndex)(DWORD index);
    model_t *(*LoadModel)(LPCSTR modelFilename);
    void (*DrawImage)(LPCTEXTURE texture, LPCRECT screen, LPCRECT uv);
    void (*DrawPortrait)(model_t const *model, LPCRECT viewport);
    
    LPSTR (*ParserGetToken)(parser_t *p);
    void (*ParserError)(parser_t *p);
    configValue_t *(*ParseConfig)(LPCSTR fileName);
    LPCSTR (*FindConfigValue)(configValue_t *ini, LPCSTR sectionName, LPCSTR valueName);
} uiImport_t;

typedef struct {
    void (*Init)(void);
    void (*Shutdown)(void);
    void (*Draw)(void);
    void (*ServerCommand)(DWORD argc, LPCSTR argv[]);
    void (*HandleEvent)(entityState_t const *ent, uiEvent_t event);
    void (*MouseEvent)(LPCVECTOR2 mouse, uiMouseEvent_t event);
} uiExport_t;

uiExport_t UI_GetAPI(uiImport_t imp);

#endif /* ui_h */
