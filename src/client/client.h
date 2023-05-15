#ifndef client_h
#define client_h

#include "../common/common.h"
#include "renderer.h"

#define WINDOW_WIDTH 1024
#define WINDOW_HEIGHT 768
#define MAX_CLIENT_ENTITIES 5000

struct client_entity {
    struct entity_state baseline;
//    struct entity_state previous;
    struct entity_state current;
};

struct frame {
    int serverframe;
    int servertime;
    int oldclientframe;
};

struct client_state {
    struct refdef refdef;
    struct client_entity ents[MAX_CLIENT_ENTITIES];
    struct tModel *models[MAX_MODELS];
    struct tTexture *pics[MAX_IMAGES];
    struct frame frame;
    path_t configstrings[MAX_CONFIGSTRINGS];
    int num_entities;
    int sock;
};

void V_RenderView(void);
void CL_PrepRefresh(void);
void CL_ParseServerMessage(struct sizebuf *msg);
int CL_ParseEntityBits(struct sizebuf *msg, uint32_t *bits);

extern struct client_state cl;

#endif
