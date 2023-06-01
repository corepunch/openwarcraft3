#ifndef ui_local_h
#define ui_local_h

#include "../client/ui.h"

#define UI_SCALE 1000
#define MAX_FRAME_POINTS 8
#define COMMANDBAR_COLUMNS 4
#define COMMANDBAR_ROWS 3
#define MAX_SELECTED_ENTITIES 16
#define NUM_COMMANDBAR_CELLS (COMMANDBAR_COLUMNS * COMMANDBAR_ROWS)
#define COMMAND_BUTTON_SIZE 0.039
#define COMMANDBAR_FRAME_WIDTH 0.1745
#define COMMANDBAR_FRAME_HEIGHT 0.1290

typedef char uiName_t[80];
typedef struct uiFrameDef_s uiFrameDef_t;
typedef bool (*uiMouseEventHandler_t)(uiFrameDef_t *frameDef);

typedef struct {
    entityState_t const *current;
    model_t const *portraitModel;
} uiPortrait_t;

typedef enum {
    FT_UNDEFINED,
    FT_BACKDROP,
    FT_BUTTON,
    FT_CHATDISPLAY,
    FT_CHECKBOX,
    FT_CONTROL,
    FT_DIALOG,
    FT_EDITBOX,
    FT_FRAME,
    FT_GLUEBUTTON,
    FT_GLUECHECKBOX,
    FT_GLUEEDITBOX,
    FT_GLUEPOPUPMENU,
    FT_GLUETEXTBUTTON,
    FT_HIGHLIGHT,
    FT_LISTBOX,
    FT_MENU,
    FT_MODEL,
    FT_POPUPMENU,
    FT_SCROLLBAR,
    FT_SIMPLEBUTTON,
    FT_SIMPLECHECKBOX,
    FT_SIMPLEFRAME,
    FT_SIMPLESTATUSBAR,
    FT_SLASHCHATBOX,
    FT_SLIDER,
    FT_SPRITE,
    FT_TEXT,
    FT_TEXTAREA,
    FT_TEXTBUTTON,
    FT_TIMERTEXT,
} uiFrameType_t;

typedef enum {
    AM_ALPHAKEY,
} uiAlphaMode_t;

typedef enum {
    AC_TOPLEFT,
    AC_TOPRIGHT,
    AC_BOTTOMLEFT,
    AC_BOTTOMRIGHT,
} uiAnchorCorner_t;

typedef enum {
    FRAMEPOINT_TOPLEFT,
    FRAMEPOINT_TOP,
    FRAMEPOINT_TOPRIGHT,
    FRAMEPOINT_LEFT,
    FRAMEPOINT_CENTER,
    FRAMEPOINT_RIGHT,
    FRAMEPOINT_BOTTOMLEFT,
    FRAMEPOINT_BOTTOM,
    FRAMEPOINT_BOTTOMRIGHT,
} uiFramePointType_t;

typedef enum {
    CBAR_SHOW_ABILITIES,
    CBAR_SELECT_TARGET,
} uiCommandBarMode_t;

typedef struct {
    uiAnchorCorner_t corner;
    float x;
    float y;
} uiAnchor_t;

typedef struct uiTextureDef_s {
    LPCTEXTURE Texture;
    uiAlphaMode_t AlphaMode;
    uiAnchor_t Anchor;
    RECT TexCoord;
} uiTextureDef_t;

typedef struct uiCommandButton_s {
    LPCTEXTURE Texture;
    void (*click)(struct uiCommandButton_s const *cmd);
} uiCommandButton_t;

typedef struct uiFramePoint_s {
    uiFramePointType_t type;
    uiFramePointType_t targetType;
    uiFrameDef_t *targetFrame;
    float x, y;
    struct uiFramePoint_s *next;
} uiFramePoint_t;

struct uiFrameDef_s {
    uiFrameDef_t *next;
    uiFrameDef_t *children;
    uiFrameType_t Type;
    uiFramePoint_t Points[MAX_FRAME_POINTS];
    uiName_t Name;
    DWORD numPoints;
    RECT rect;
    float Width;
    float Height;
    bool DecorateFileNames;
    void *typedata;
    void (*draw)(uiFrameDef_t const *frameDef);
    uiMouseEventHandler_t mouseHandler[NUM_UI_MOUSE_EVENTS];
};

typedef struct {
    uiFrameDef_t *simpleConsole;
    uiFrameDef_t *commandBar;
    configValue_t *theme;
    entityState_t const *selectedEntities[MAX_SELECTED_ENTITIES];
    uiCommandBarMode_t commandBarMode;
    LPCTEXTURE item_textures[MAX_ITEMS];
    LPCTEXTURE btnCancel;
} uiLocal_t;

// ui_parser.c
uiFrameDef_t *FDF_ParseFile(LPCSTR filename);
uiFrameDef_t *FDF_ParseBuffer(LPCSTR buffer, LPCSTR fileName);

// ui_commandbar.c
uiFrameDef_t *UI_MakeCommandBar(uiFrameDef_t *parent);
uiCommandButton_t *UI_GetCommandButton(DWORD index);
void CommandBar_SetMode(uiCommandBarMode_t mode);

// ui_commandbutton.c
void CommandButton_SelectTarget(uiCommandButton_t const *cmd);
void CommandButton_Cancel(uiCommandButton_t const *cmd);
uiFrameDef_t *UI_MakeCommandButton(void);
DWORD UI_CommandButtonPosition(BYTE item);
LPCTEXTURE UI_LoadItemTexture(BYTE item);
LPCTEXTURE UI_GetItemTexture(BYTE item);


// ui_texture.c
uiFrameDef_t *UI_MakeTextureFrame(void);

// ui_frame.c
void UI_DrawFrame(uiFrameDef_t const *frame);
void UI_UpdateFrame(uiFrameDef_t *frame);
void UI_FrameAddChild(uiFrameDef_t *frame, uiFrameDef_t *child);
RECT GetFrameRect(uiFrameDef_t const *frame);
void SetFramePoint(uiFrameDef_t *frame,
                   uiFramePointType_t framePoint,
                   uiFrameDef_t *targetFrame,
                   uiFramePointType_t targetPoint,
                   float x,
                   float y);

// ui_servercmds.c
void UI_ServerCommand(DWORD argc, LPCSTR argv[]);
LPCTEXTURE UI_GetItemTexture(BYTE item);
DWORD UI_CommandButtonPosition(BYTE item);

// ui_config.c
void UI_InitConfigFiles(void);
void UI_ShutdownConfigFiles(void);
LPCSTR UI_FindConfigValue(LPCSTR category, LPCSTR field);
LPCSTR UI_GetClassName(DWORD class_id);

// ui_input.c
void UI_MouseEvent(LPCVECTOR2 mouse, uiMouseEvent_t event);

extern uiImport_t imp;
extern uiLocal_t ui;

#endif
