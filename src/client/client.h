#ifndef client_h
#define client_h

#include "../common/common.h"
#include "renderer.h"

#define WINDOW_WIDTH 1024
#define WINDOW_HEIGHT 768
#define MAX_CLIENT_ENTITIES 5000

struct client_entity {
    ENTITYSTATE baseline;
//    ENTITYSTATE previous;
    ENTITYSTATE current;
};

struct frame {
    int serverframe;
    int servertime;
    int oldclientframe;
};

struct client_state {
    LPMODEL models[MAX_MODELS];
    LPTEXTURE pics[MAX_IMAGES];
    PATHSTR configstrings[MAX_CONFIGSTRINGS];
    struct client_entity ents[MAX_CLIENT_ENTITIES];
    struct viewDef viewDef;
    struct frame frame;
    DWORD num_entities;
    DWORD sock;
    DWORD team;
};

struct client_static {
    struct netchan netchan;
};

void V_RenderView(void);
void CL_PrepRefresh(void);
void CL_ParseServerMessage(LPSIZEBUF msg);
int CL_ParseEntityBits(LPSIZEBUF msg, DWORD *bits);
void CL_SelectEntityAtScreenPoint(DWORD dwPixelX, DWORD dwPixelY);

extern struct client_state cl;
extern struct Renderer *renderer;

#endif
