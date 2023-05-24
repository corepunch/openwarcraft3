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

typedef struct {
    entityState_t baseline;
    entityState_t current;
    entityState_t prev;
} clientEntity_t;

struct frame {
    int serverframe;
    int servertime;
    int oldclientframe;
};

struct client_state {
    LPMODEL models[MAX_MODELS];
    LPTEXTURE pics[MAX_IMAGES];
    PATHSTR configstrings[MAX_CONFIGSTRINGS];
    clientEntity_t ents[MAX_CLIENT_ENTITIES];
    viewDef_t viewDef;
    struct frame frame;
    VECTOR2 startingPosition;
    DWORD num_entities;
    DWORD sock;
    DWORD playerNumber;
    DWORD time;
};

struct client_static {
    struct netchan netchan;
};

void V_RenderView(void);
void CL_PrepRefresh(void);
void CL_ParseServerMessage(LPSIZEBUF msg);
int CL_ParseEntityBits(LPSIZEBUF msg, DWORD *bits);
void CL_SelectEntityAtScreenPoint(DWORD pixelX, DWORD pixelY);
void CON_DrawConsole(void);
void CON_printf(char *fmt, ...);

extern struct client_state cl;
extern struct Renderer *renderer;

#endif
