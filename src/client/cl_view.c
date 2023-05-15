#include "client.h"
#include "renderer.h"

extern struct Renderer *renderer;
extern struct client_state cl;

static struct {
    struct render_entity entities[MAX_CLIENT_ENTITIES];
    int num_entities;
} view_state;

static void V_AddClientEntity(struct client_entity const *ent) {
    struct render_entity *re = &view_state.entities[view_state.num_entities++];
    re->postion = ent->current.origin;
    re->angle = ent->current.angle;
    re->scale = ent->current.scale;
    re->frame = ent->current.frame;
    re->model = cl.models[ent->current.model];
    re->skin = cl.pics[ent->current.image];
}

static void V_ClearScene(void) {
    view_state.num_entities = 0;
    cl.refdef.num_entities = 0;
}

static void CL_AddEntities(void) {
    FOR_LOOP(index, MAX_CLIENT_ENTITIES) {
        struct client_entity const *ce = &cl.ents[index];
        if (!ce->current.model)
            continue;
        V_AddClientEntity(ce);
    }
    cl.refdef.num_entities = view_state.num_entities;
    cl.refdef.entities = view_state.entities;
}

void CL_PrepRefresh(void) {
    if (!cl.configstrings[CS_MODELS+1][0])
        return; // no map loaded
    
    static bool map_registered = false;

    if (!map_registered) {
        renderer->RegisterMap(cl.configstrings[CS_MODELS+1]);
        map_registered = true;
    }
    
    for (int i = 2; i < MAX_MODELS && *cl.configstrings[CS_MODELS + i]; i++) {
        if (cl.models[i])
            continue;
        cl.models[i] = renderer->LoadModel(cl.configstrings[CS_MODELS + i]);
    }
    
    for (int i = 1; i < MAX_IMAGES && *cl.configstrings[CS_IMAGES + i]; i++) {
        if (cl.pics[i])
            continue;
        cl.pics[i] = renderer->LoadTexture(cl.configstrings[CS_IMAGES + i]);
    }
}

void V_RenderView(void) {
    V_ClearScene();
    CL_AddEntities();
    
    renderer->BeginFrame();
    renderer->RenderFrame(&cl.refdef);
    renderer->EndFrame();
}

