#ifndef client_h
#define client_h

#include "../common/common.h"
#include "renderer.h"
#include "keys.h"

#define WINDOW_WIDTH 1024
#define WINDOW_HEIGHT 768
#define MAX_CLIENT_ENTITIES 5000
#define MAX_CONSOLE_MESSAGES 256
#define MAX_CONSOLE_MESSAGE_LEN 1024
#define CONSOLE_MESSAGE_TIME 5000
#define VIEW_SHADOW_SIZE 1500
#define MAX_CONFIRMATION_OBJECTS 16
#define MAX_LAYOUT_LAYERS 4
#define MAX_SELECTED_ENTITIES 64

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
    VECTOR2 origin;
} mouseEvent_t;

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
    viewDef_t viewDef;
    struct frame frame;
    VECTOR2 startingPosition;
    playerState_t playerstate;
    LPENTITYSTATE cursorEntity;
    LPCMODEL moveConfirmation;
    DWORD num_entities;
    DWORD sock;
    DWORD time;
    struct {
        RECT  rect;
        bool in_progress;
    } selection;
};

struct client_static {
    struct netchan netchan;
};

void V_RenderView(void);
void CL_PrepRefresh(void);
void CL_ParseServerMessage(LPSIZEBUF msg);
void CON_DrawConsole(void);
void CON_printf(LPCSTR fmt, ...);

// cl_view.c
//void Matrix4_fromViewAngles(LPCVECTOR3 target, LPCVECTOR3 angles, float distance, LPMATRIX4 output);
//void Matrix4_getLightMatrix(LPCVECTOR3 sunangles, LPCVECTOR3 target, float scale, LPMATRIX4 output);
//void Matrix4_getCameraMatrix(viewCamera_t const *camera, LPMATRIX4 output);
void V_AddEntity(renderEntity_t *ent);

// cl_scrn.c
LPCUIFRAME SCR_Clear(HANDLE data);
LPCRECT SCR_LayoutRect(LPCUIFRAME frame);
void SCR_UpdateScreen(void);

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
