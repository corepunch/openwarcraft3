#include "client.h"
#include "renderer.h"

static struct {
    renderEntity_t entities[MAX_CLIENT_ENTITIES];
    int num_entities;
} view_state;

VECTOR3 lightAngles = {-35,0,45};

void Matrix4_fromViewAngles(LPCVECTOR3 target, LPCVECTOR3 angles, float distance, LPMATRIX4 output) {
    VECTOR3 const vieworg = Vector3_unm(target);
    Matrix4_identity(output);
    Matrix4_translate(output, &(VECTOR3){0, 0, -distance});
    Matrix4_rotate(output, angles, ROTATE_ZYX);
    Matrix4_translate(output, &vieworg);
}

void Matrix4_getLightMatrix(LPCVECTOR3 sunangles, LPCVECTOR3 target, float scale, LPMATRIX4 output) {
    MATRIX4 proj, view;
    Matrix4_ortho(&proj, -scale, scale, -scale, scale, 100.0, 3500.0);
    Matrix4_fromViewAngles(target, sunangles, 1000, &view);
    Matrix4_multiply(&proj, &view, output);
}

void Matrix4_getCameraMatrix(viewCamera_t const *camera, LPMATRIX4 output) {
    MATRIX4 proj, view;
    SIZE2 const windowSize = re.GetWindowSize();
    float const aspect = (float)windowSize.width / (float)windowSize.height;
    Matrix4_perspective(&proj, camera->fov, aspect, camera->znear, camera->zfar);
    Matrix4_fromViewAngles(&camera->target, &camera->angles, camera->distance, &view);
    Matrix4_multiply(&proj, &view, output);
}

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

    if (ent->selected) {
        re.flags = RF_SELECTED;
    }
    
    view_state.entities[view_state.num_entities++] = re;

}

static void V_ClearScene(void) {
    view_state.num_entities = 0;
    cl.viewDef.num_entities = 0;
}

static void CL_AddConfirmationObject(moveConfirmation_t const *mc) {
    renderEntity_t re;
    memset(&re, 0, sizeof(renderEntity_t));
    re.origin = mc->origin;
    re.scale = 1;
    re.frame = cl.time - mc->timespamp;
    re.oldframe = cl.time - mc->timespamp;
    re.model = cl.cursor;
    
    view_state.entities[view_state.num_entities++] = re;
}

static void CL_AddEntities(void) {
    cl.viewDef.lerpfrac = (float)(cl.time - cl.frame.servertime) / FRAMETIME;

    FOR_LOOP(index, MAX_CLIENT_ENTITIES) {
        clientEntity_t const *ce = &cl.ents[index];
        if (!ce->current.model /*|| ce->current.number != 108*/)
            continue;
        V_AddClientEntity(ce);
    }

    FOR_LOOP(index, MAX_CONFIRMATION_OBJECTS) {
        if (cl.time - cl.confs[index].timespamp > 1000)
            continue;
        CL_AddConfirmationObject(&cl.confs[index]);
    }

    cl.viewDef.num_entities = view_state.num_entities;
    cl.viewDef.entities = view_state.entities;
}

void CL_PrepRefresh(void) {
    if (!cl.configstrings[CS_MODELS+1][0])
        return; // no map loaded

    static bool map_registered = false;

    if (!map_registered) {
        re.RegisterMap(cl.configstrings[CS_MODELS+1]);
        map_registered = true;
    }

    for (int i = 2; i < MAX_MODELS && *cl.configstrings[CS_MODELS + i]; i++) {
        if (cl.models[i])
            continue;
        cl.models[i] = re.LoadModel(cl.configstrings[CS_MODELS + i]);
    }

    for (int i = 1; i < MAX_IMAGES && *cl.configstrings[CS_IMAGES + i]; i++) {
        if (cl.pics[i])
            continue;
        cl.pics[i] = re.LoadTexture(cl.configstrings[CS_IMAGES + i]);
    }
}

void V_RenderView(void) {

#ifdef DEBUG_PATHFINDING
    extern LPCOLOR32 pathDebug;
    re.SetPathTexture(pathDebug);
#endif
    
    cl.viewDef.viewport = (RECT) { 0, 0, 1, 1 };
    cl.viewDef.scissor = (RECT) { 0, 0.2, 1, 0.8 };
    cl.viewDef.time = cl.time;
    
    Matrix4_getCameraMatrix(&cl.viewDef.camera, &cl.viewDef.projectionMatrix);
    Matrix4_getLightMatrix(&lightAngles, &cl.viewDef.camera.target, VIEW_SHADOW_SIZE, &cl.viewDef.lightMatrix);

    V_ClearScene();
    CL_AddEntities();

    re.RenderFrame(&cl.viewDef);
    
//    re.DrawPic(tex1, 0, 0);
//    re.DrawPic(tex2, 512, 0);

    if (cl.selection.inProgress) {
        re.DrawSelectionRect(&cl.selection.rect, (COLOR32){0,255,0,255});
    }
}
