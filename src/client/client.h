#ifndef client_h
#define client_h

#include "../common/common.h"
#include "renderer.h"

#define WINDOW_WIDTH 1024
#define WINDOW_HEIGHT 768
#define MAX_CLIENT_ENTITIES 5000
#define MAX_CONSOLE_MESSAGES 256
#define MAX_CONSOLE_MESSAGE_LEN 1024
#define CONSOLE_MESSAGE_TIME 5000
#define VIEW_SHADOW_SIZE 1500
#define MAX_CONFIRMATION_OBJECTS 16
#define MAX_LAYOUT_LENGTH 1024

typedef struct {
    entityState_t baseline;
    entityState_t current;
    entityState_t prev;
    bool selected;
} clientEntity_t;

struct frame {
    int serverframe;
    int servertime;
    int oldclientframe;
};

typedef struct  {
    VECTOR3 origin;
    DWORD timespamp;
} moveConfirmation_t;

struct client_state {
    model_t *models[MAX_MODELS];
    LPTEXTURE pics[MAX_IMAGES];
    PATHSTR configstrings[MAX_CONFIGSTRINGS];
    clientEntity_t ents[MAX_CLIENT_ENTITIES];
    moveConfirmation_t confs[MAX_CONFIRMATION_OBJECTS];
    char layout[MAX_LAYOUT_LENGTH];        // general 2D overlay
    viewDef_t viewDef;
    struct frame frame;
    VECTOR2 startingPosition;
    model_t const *cursor;
    DWORD num_entities;
    DWORD confirmationCounter;
    DWORD sock;
    DWORD playerNumber;
    DWORD time;
    struct {
        RECT  rect;
        bool inProgress;
    } selection;
};

struct client_static {
    struct netchan netchan;
};

void V_RenderView(void);
void CL_PrepRefresh(void);
void CL_ParseServerMessage(LPSIZEBUF msg);
int CL_ParseEntityBits(LPSIZEBUF msg, DWORD *bits);
void CL_SelectEntityAtScreenPoint(DWORD pixelX, DWORD pixelY);
void CL_SelectEntitiesAtScreenRect(LPCRECT rect);
void CON_DrawConsole(void);
void CON_printf(char *fmt, ...);

// cl_view.c
void Matrix4_fromViewAngles(LPCVECTOR3 target, LPCVECTOR3 angles, float distance, LPMATRIX4 output);
void Matrix4_getLightMatrix(LPCVECTOR3 sunangles, LPCVECTOR3 target, float scale, LPMATRIX4 output);
void Matrix4_getCameraMatrix(viewCamera_t const *camera, LPMATRIX4 output);

// cl_scrn.c
void SCR_UpdateScreen(void);

extern struct client_state cl;
extern struct Renderer re;

#endif
