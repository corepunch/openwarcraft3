#ifndef client_h
#define client_h

#include "../common/common.h"
#include "renderer.h"

#define WINDOW_WIDTH 1024
#define WINDOW_HEIGHT 768
#define MAX_CLIENT_ENTITIES 5000

struct client_entity {
    struct vector3 postion;
    float angle;
    struct vector3 scale;
    int model;
    int skin;
};

struct client_state {
    struct refdef refdef;
    struct client_entity ents[MAX_CLIENT_ENTITIES];
    struct tModel *models[MAX_MODELS];
    struct tTexture *pics[MAX_IMAGES];
    path_t configstrings[MAX_CONFIGSTRINGS];
    int num_entities;
    int sock;
};

void V_RenderView(void);
void CL_PrepRefresh(void);

#endif
