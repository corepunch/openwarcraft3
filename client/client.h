#ifndef client_h
#define client_h

#include "../common/common.h"
#include "renderer.h"
#include "keys.h"

#define WINDOW_WIDTH 1024
#define WINDOW_HEIGHT 768
#define UI_BASE_WIDTH 0.8f
#define UI_BASE_HEIGHT 0.6f
#define UI_MIN_ASPECT (4.0f / 3.0f)
#define MAX_CLIENT_ENTITIES 5000
#define MAX_CONSOLE_MESSAGES 256
#define MAX_CONSOLE_MESSAGE_LEN 1024
#define CONSOLE_MESSAGE_TIME 5000
#define VIEW_SHADOW_SIZE 1500
#define MAX_CONFIRMATION_OBJECTS 16
#define MAX_LAYOUT_LAYERS 16

typedef struct {
    entityState_t baseline;
    entityState_t current;
    entityState_t prev;
    DWORD serverframe;
    bool selected;
} centity_t;

typedef enum {
    UI_EVENT_NONE,
    UI_LEFT_MOUSE_DOWN,
    UI_LEFT_MOUSE_UP,
    UI_LEFT_MOUSE_DRAGGED,
    UI_RIGHT_MOUSE_DOWN,
    UI_RIGHT_MOUSE_UP,
    UI_RIGHT_MOUSE_DRAGGED,
    NUM_UI_MOUSE_EVENTS
} mouseEventType_t;

typedef struct {
    mouseEventType_t event;
    DWORD button;
    int wheel;
    VECTOR2 origin;
} mouseEvent_t;

typedef enum {
    key_game,
    key_console,
    key_message,
    key_menu,
} keydest_t;

typedef enum {
    ca_disconnected,
    ca_connecting,
    ca_connected,
    ca_active,
} connstate_t;

struct frame {
    int serverframe;
    int servertime;
    int oldclientframe;
};

struct client_state {
    LPMODEL models[MAX_MODELS];
    LPMODEL portraits[MAX_MODELS];
    LPCTEXTURE pics[MAX_IMAGES];
    LPCFONT fonts[MAX_FONTSTYLES];
    PATHSTR configstrings[MAX_CONFIGSTRINGS];
    centity_t ents[MAX_CLIENT_ENTITIES];
    HANDLE layout[MAX_LAYOUT_LAYERS];
    
    LPCTEXTURE healthbar;
    
    viewDef_t viewDef;
    struct frame frame;
    VECTOR2 startingPosition;
    PLAYER playerstate;
    LPENTITYSTATE cursorEntity;
    LPCMODEL moveConfirmation;
    DWORD num_entities;
    DWORD time;
    struct {
        RECT rect;
        bool in_progress;
    } selection;
};

struct client_static {
    struct netchan netchan;
    keydest_t key_dest;
    connstate_t state;
};

// cl_main.c
void CL_Connect(LPCSTR host, unsigned short port);
void CL_SetMenuBindings(void);
void CL_SetGameplayBindings(void);

void V_RenderView(void);
void V_Shutdown(void);
void CL_PrepRefresh(void);
void CL_ParseServerMessage(LPSIZEBUF msg);

typedef struct listFetchState_s {
    BOOL inuse;
    BOOL started;
    DWORD requestId;
    DWORD frameNumber;
    HANDLE layout;
    UINAME command;
    char text[MAX_LIST_FETCH_TEXT];
    SHORT selectedIndex;
    DWORD scrollOffset;
    BOOL loading;
    DWORD numRows;
} listFetchState_t;

void CL_ParseListFetch(LPSIZEBUF msg);
void CL_ListBoxApplyFetch(HANDLE layout,
                          LPCUIFRAME frame,
                          uiListBox_t const *listbox,
                          LPCSTR *text,
                          BOOL *loading,
                          SHORT *selectedIndex,
                          DWORD *scrollOffset,
                          DWORD *numRows);
void CL_ListBoxScroll(HANDLE layout,
                      LPCUIFRAME frame,
                      uiListBox_t const *listbox,
                      int delta,
                      DWORD visibleRows);
void CL_ListBoxSelect(HANDLE layout,
                      LPCUIFRAME frame,
                      uiListBox_t const *listbox,
                      DWORD rowIndex);
void CON_DrawConsole(void);
void CON_printf(LPCSTR fmt, ...);

// cl_view.c
//void Matrix4_fromViewAngles(LPCVECTOR3 target, LPCVECTOR3 angles, float distance, LPMATRIX4 output);
//void Matrix4_getLightMatrix(LPCVECTOR3 sunangles, LPCVECTOR3 target, float scale, LPMATRIX4 output);
//void Matrix4_getCameraMatrix(viewCamera_t const *camera, LPMATRIX4 output);
void V_AddEntity(renderEntity_t *ent);

// cl_scrn.c
LPCUIFRAME SCR_Clear(HANDLE data);
DWORD SCR_NumFrames(void);
LPUIFRAME SCR_Frame(DWORD number);
LPCRECT SCR_LayoutRect(LPCUIFRAME frame);
VECTOR2 SCR_MouseToFdf(void);
VECTOR2 SCR_GetAxisBounds(LPCRECT rect, bool is_x_axis);
FLOAT SCR_NormalizeAnchorOffset(uiFramePoint_t const *p, bool is_x_axis);
VECTOR2 SCR_SolveAxisPosition(LPCUIFRAME frame,
                              uiFramePoints_t const points,
                              FLOAT width,
                              bool is_x_axis);
LPCSTR SCR_GetStringValue(LPCUIFRAME frame);
DRAWTEXT SCR_GetDrawText(LPCUIFRAME frame,
                         FLOAT avl_width,
                         LPCSTR text,
                         uiLabel_t const *label);
void SCR_UpdateScreen(void);
void SCR_DrawOverlays(void);
void SCR_TextInput(LPCSTR text);
BOOL SCR_EditKey(int key);

// cl_input.c
void CL_Input(void);
void CL_InitInput(void);

// cl_tent.c
void CL_ParseTEnt(LPSIZEBUF msg);
void CL_AddTEnts(void);
void CL_ClearTEnts(void);

extern struct client_state cl;
extern struct client_static cls;
extern refExport_t re;
extern mouseEvent_t mouse;

#endif
