#include "client.h"
#include "renderer.h"

static struct {
    renderEntity_t entities[MAX_CLIENT_ENTITIES];
    int num_entities;
} view_state;

float LerpRotation(float a, float b, float t) {
    if (b < 0) {
        b = b + 2 * M_PI;
    }
    float apos = a + 2 * M_PI;
    float aneg = a - 2 * M_PI;
    if (fabs(a - b) < fabs(apos - b) && fabs(a - b) < fabs(aneg - b)) {
        return LerpNumber(a, b, t);
    } else if (fabs(apos - b) < fabs(aneg - b)) {
        return LerpNumber(apos, b, t);
    } else {
        return LerpNumber(aneg, b, t);
    }
}

static void V_AddClientEntity(clientEntity_t const *ent) {
    renderEntity_t re = { 0 };
    re.origin = Vector3_lerp(&ent->prev.origin, &ent->current.origin, cl.viewDef.lerpfrac);
    re.angle = LerpRotation(ent->prev.angle, ent->current.angle, cl.viewDef.lerpfrac);
    re.scale = LerpNumber(ent->prev.scale, ent->current.scale, cl.viewDef.lerpfrac);
    re.frame = ent->current.frame;
    re.oldframe = ent->prev.frame;
    re.model = cl.models[ent->current.model];
    re.skin = cl.pics[ent->current.image];
    re.team = ent->current.player;

    extern int selectedEntity;

    view_state.entities[view_state.num_entities++] = re;

    if (ent->current.number == selectedEntity) {
        int model = 3;
        re.model = cl.models[model];
        view_state.entities[view_state.num_entities++] = re;
    }
}

static void V_ClearScene(void) {
    view_state.num_entities = 0;
    cl.viewDef.num_entities = 0;
}

static void CL_AddEntities(void) {
    cl.viewDef.lerpfrac = (float)(cl.time - cl.frame.servertime) / FRAMETIME;

    FOR_LOOP(index, MAX_CLIENT_ENTITIES) {
        clientEntity_t const *ce = &cl.ents[index];
        if (!ce->current.model)
            continue;
        V_AddClientEntity(ce);
    }

    cl.viewDef.num_entities = view_state.num_entities;
    cl.viewDef.entities = view_state.entities;
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

//static struct texture *tex1 = NULL;
//static struct texture *tex2 = NULL;

void V_RenderView(void) {
//    if (!tex1) tex1 = renderer->LoadTexture("UI\\Glues\\Loading\\Backgrounds\\Campaigns\\Barrens-TopLeft.blp");
//    if (!tex2) tex2 = renderer->LoadTexture("UI\\Glues\\Loading\\Backgrounds\\Campaigns\\Barrens-TopRight.blp");

    V_ClearScene();
    CL_AddEntities();

    renderer->BeginFrame();
    renderer->RenderFrame(&cl.viewDef);
    
//    renderer->DrawPic(tex1, 0, 0);
//    renderer->DrawPic(tex2, 512, 0);

    CON_DrawConsole();
    renderer->EndFrame();
}
