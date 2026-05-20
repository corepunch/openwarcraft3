/*
 * ui_local.h — UI library internal types and declarations.
 *
 * This file contains the internal data structures, function prototypes, and
 * constants used within the UI library. External code should only include
 * ui.h, never this file.
 *
 * Frame template structures (FRAMEDEF, frameDef_t) are defined here.
 * These are used internally by the UI library to build frame hierarchies
 * from FDF files.
 */
#ifndef ui_local_h
#define ui_local_h

#include "ui.h"
#include "../common/shared.h"  /* For PLAYERSTATE/PLAYERTEXT enums */

/* Constants */
#define MAX_BUILD_QUEUE 7
/* MAX_SELECTED_ENTITIES is defined in common/shared.h */
#ifndef MAX_UI_CLASSES
#define MAX_UI_CLASSES 2048
#endif

#define UI_BASE_WIDTH 0.8f
#define UI_BASE_HEIGHT 0.6f
#define UI_MIN_ASPECT (4.0f / 3.0f)

/* Forward declarations */
typedef struct uiFrameDef_s frameDef_t;
typedef frameDef_t FRAMEDEF;  /* Also available as non-pointer type */
typedef frameDef_t *LPFRAMEDEF;
typedef frameDef_t const *LPCFRAMEDEF;
typedef struct uiScreen_s uiScreen_t;  /* Defined in ui_screen.h */

/* Global import callbacks (filled by UI_GetAPI) */
extern uiImport_t uiimport;

/* Frame point positioning */
typedef enum {
    FRAMEPOINT_TOPLEFT,
    FRAMEPOINT_TOP,
    FRAMEPOINT_TOPRIGHT,
    FRAMEPOINT_UNUSED1,
    FRAMEPOINT_LEFT,
    FRAMEPOINT_CENTER,
    FRAMEPOINT_RIGHT,
    FRAMEPOINT_UNUSED2,
    FRAMEPOINT_BOTTOMLEFT,
    FRAMEPOINT_BOTTOM,
    FRAMEPOINT_BOTTOMRIGHT,
    FRAMEPOINT_UNUSED3,
} UIFRAMEPOINT;

typedef enum {
    FONTFLAGS_FIXEDSIZE,
    FONTFLAGS_PASSWORDFIELD,
} UIFONTFLAGS;

typedef enum {
    FILETEXTURE,
} HIGHLIGHTTYPE;

typedef enum {
    AUTOTRACK = 1,
    HIGHLIGHTONFOCUS = 2,
    HIGHLIGHTONMOUSEOVER = 4,
} CONTROLSTYLE;

typedef enum {
    LAYOUT_HORIZONTAL,
    LAYOUT_VERTICAL,
} LAYOUTDIRECTION;

typedef struct {
    UINAME frame;
    UINAME text;
} BUTTONTEXT;

typedef struct {
    uiFramePointPos_t targetPos;
    bool used;
    LPCFRAMEDEF relativeTo;
    FLOAT offset;
} FRAMEPOINT;

typedef FRAMEPOINT const *LPCFRAMEPOINT;

typedef enum {
    UI_MOUSE_EVENT_NONE,
    UI_MOUSE_LEFT_DOWN,
    UI_MOUSE_LEFT_UP,
    UI_MOUSE_RIGHT_DOWN,
    UI_MOUSE_RIGHT_UP,
} uiMouseEvent_t;

typedef struct {
    int x;
    int y;
    int button;
    BOOL down;
    uiMouseEvent_t event;
} uiMouseState_t;

extern uiMouseState_t ui_mouse;

/* Frame template definition (server-side/library-side only) */
struct uiFrameDef_s {
    LPCFRAMEDEF Parent;
    FRAMETYPE Type;
    UINAME Name;
    UINAME TextStorage;
    UINAME OnClick;
    LPCSTR Text, Tip, Ubertip;
    FLOAT Width, Height;
    COLOR32 Color;
    BLEND_MODE AlphaMode;
    BOOL DecorateFileNames;
    BOOL inuse;
    BOOL AnyPointsSet;
    BOOL hidden;
    DWORD TextLength;
    DWORD Stat;
    LPSTR DynamicText;
    DWORD DynamicTextCapacity;
    struct {
        FRAMEPOINT x[FPP_COUNT];
        FRAMEPOINT y[FPP_COUNT];
    } Points;
    struct {
        DWORD Image;
        DWORD Image2;
        BOX2 TexCoord;
    } Texture;
    struct {
        BOOL TileBackground;
        DWORD Background;
        DWORD CornerFlags;
        FLOAT CornerSize;
        FLOAT BackgroundSize;
        FLOAT BackgroundInsets[4];
        DWORD EdgeFile;
        BOOL BlendAll;
        BOOL Mirrored;
    } Backdrop;
    LPCFRAMEDEF DialogBackdrop;
    struct {
        DWORD model;
    } Portrait;
    struct {
        UIFRAMEPOINT corner;
        FLOAT x, y;
    } Anchor;
    struct {
        UIFRAMEPOINT type;
        LPCFRAMEDEF relativeTo;
        UIFRAMEPOINT target;
        FLOAT x, y;
    } SetPoint;
    struct {
        UINAME Name;
        UINAME Unknown;
        UIFONTFLAGS FontFlags;
        FLOAT Size;
        DWORD Index;
        COLOR32 Color;
        COLOR32 HighlightColor;
        COLOR32 DisabledColor;
        COLOR32 ShadowColor;
        VECTOR2 ShadowOffset;
        struct {
            VECTOR2 Offset;
            uiFontJustificationH_t Horizontal;
            uiFontJustificationV_t Vertical;
        } Justification;
    } Font;
    struct {
        HIGHLIGHTTYPE Type;
        DWORD AlphaFile;
        BLEND_MODE AlphaMode;
    } Highlight;
    struct {
        VECTOR2 PushedTextOffset;
        UINAME NormalTexture;
        UINAME PushedTexture;
        UINAME DisabledTexture;
        UINAME UseHighlight;
        BUTTONTEXT NormalText;
        BUTTONTEXT DisabledText;
        BUTTONTEXT HighlightText;
    } Button;
    struct {
        DWORD Style;
        struct {
            UINAME Normal;
            UINAME Pushed;
            UINAME Disabled;
            UINAME MouseOver;
            UINAME DisabledPushed;
            UINAME Focus;
        } Backdrop;
        UINAME ShortcutKey;
        UINAME TabFocusNext;
        BOOL TabFocusDefault;
    } Control;
    struct {
        FLOAT InitialValue;
        LAYOUTDIRECTION Layout;
        FLOAT MaxValue;
        FLOAT MinValue;
        FLOAT StepSize;
        UINAME ThumbButtonFrame;
        UINAME IncButtonFrame;
        UINAME DecButtonFrame;
    } Slider;
    struct {
        FLOAT Border;
        UINAME ScrollBar;
        UINAME FetchCommand;
    } ListBox;
    struct {
        FLOAT Border;
        struct {
            UINAME Text;
            DWORD Value;
            FLOAT Height;
        } Item;
        COLOR32 TextHighlightColor;
    } Menu;
    struct {
        FLOAT BorderSize;
        COLOR32 CursorColor;
        COLOR32 HighlightColor;
        BOOL HighlightInitial;
        DWORD MaxChars;
        BOOL Focus;
        UINAME Text;
        COLOR32 TextColor;
        UINAME TextFrame;
        VECTOR2 TextOffset;
    } Edit;
    struct {
        UINAME ArrowFrame;
        UINAME MenuFrame;
        UINAME TitleFrame;
        FLOAT ButtonInset;
    } Popup;
    struct {
        FLOAT LineHeight;
        FLOAT LineGap;
        FLOAT Inset;
        DWORD MaxLines;
        UINAME ScrollBar;
    } TextArea;
    struct {
        UINAME CheckHighlight;
        UINAME DisabledCheckHighlight;
    } CheckBox;
    struct {
        LPCFRAMEDEF FirstItem;
        LPCFRAMEDEF BuildTimer;
        FLOAT ItemOffset;
        DWORD NumQueue;
        uiBuildQueueItem_t Queue[MAX_BUILD_QUEUE];
    } BuildQueue;
    struct {
        DWORD HpBar;
        DWORD ManaBar;
        VECTOR2 Offset;
        DWORD NumColumns;
        DWORD NumItems;
        uiBuildQueueItem_t Items[MAX_SELECTED_ENTITIES];
    } Multiselect;
};

/* Global parsed FDF frame table. */
extern FRAMEDEF frames[MAX_UI_CLASSES];

/* Stub types for server-side code compatibility (not used in client UI library) */
typedef struct {} uiTrigger_t;
typedef void *LPEDICT;

/* Convenience macros for frame lookup */
#define UI_FRAME(NAME) LPFRAMEDEF NAME = UI_FindFrame(#NAME);
#define UI_CHILD_FRAME(NAME, PARENT) LPFRAMEDEF NAME = UI_FindChildFrame(PARENT, #NAME);

/* Internal function prototypes */

/* ui_main.c */
void UI_InitLocal(void);
void UI_ShutdownLocal(void);
void UI_RefreshLocal(DWORD msec);
void UI_DrawFrameLocal(void);

/* ui_fdf.c — FDF parsing (moved from game/ui/ui_fdf.c) */
void UI_ParseFDF(LPCSTR filename);
void UI_ParseFDF_Buffer(LPCSTR filename, LPSTR buffer);
void UI_ClearTemplates(void);
void UI_InitFrame(LPFRAMEDEF, FRAMETYPE);
void UI_SetAllPoints(LPFRAMEDEF);
void UI_SetParent(LPFRAMEDEF, LPCFRAMEDEF);
void UI_SetText(LPFRAMEDEF, LPCSTR, ...);
void UI_SetOnClick(LPFRAMEDEF, LPCSTR, ...);
void UI_SetTextPointer(LPFRAMEDEF, LPCSTR);
void UI_SetSize(LPFRAMEDEF, FLOAT, FLOAT);
void UI_SetPoint(LPFRAMEDEF, UIFRAMEPOINT, LPCFRAMEDEF, UIFRAMEPOINT, FLOAT, FLOAT);
void UI_SetTexture(LPFRAMEDEF, LPCSTR, BOOL);
void UI_SetTexture2(LPFRAMEDEF, LPCSTR, BOOL);
void UI_SetHidden(LPFRAMEDEF, BOOL);
void UI_InheritFrom(LPFRAMEDEF, LPCSTR);
void UI_LoadTheme(LPCSTR fileName);
void UI_ClearTheme(void);
void UI_MenuCommandLocal(LPCSTR command);
VECTOR2 UI_MouseToFdf(void);
BOOL UI_MouseContains(LPCRECT rect);
void UI_ClearMouseTransient(void);
DWORD UI_FindFrameNumber(LPCSTR);
DWORD UI_CollectFrameTree(LPCFRAMEDEF root, LPCFRAMEDEF *out, DWORD max);
DWORD UI_LoadTexture(LPCSTR, BOOL);
LPCTEXTURE UI_GetTexture(DWORD index);
LPCMODEL UI_GetModel(DWORD index);
DWORD UI_LoadModel(LPCSTR file, BOOL decorate);
LPCSTR UI_GetString(LPCSTR);
LPFRAMEDEF UI_Spawn(FRAMETYPE, LPFRAMEDEF);
LPFRAMEDEF UI_FindFrame(LPCSTR);
LPFRAMEDEF UI_FindFrameNear(LPCFRAMEDEF, LPCSTR);
LPFRAMEDEF UI_FindChildFrame(LPFRAMEDEF, LPCSTR);
DWORD UI_CollectFrameTree(LPCFRAMEDEF root, LPCFRAMEDEF *out, DWORD max);
LPCSTR Theme_String(LPCSTR, LPCSTR);
FLOAT Theme_Float(LPCSTR, LPCSTR);

/* ui_frame.c — Frame tree manipulation (to be created) */
// Additional frame management functions will be declared here

/* ui_render.c — Frame rendering */
void UI_DrawFrame(LPCFRAMEDEF frame);

/* ui_router.c — Menu routing */
void UI_Route(LPCSTR route);
void UI_Push(LPCSTR path);
void UI_Pop(void);
uiScreen_t *UI_GetCurrentScreen(void);

/* Screen stack for navigation history - moved to ui_screen.h */

#endif
