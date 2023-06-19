#include <stdlib.h> // atoi()

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
    MATRIX4 proj, view, tmp1, tmp2;
    Matrix4_ortho(&proj, -scale, scale, -scale, scale, 100.0, 3500.0);
    Matrix4_identity(&tmp1);
    Matrix4_rotate(&tmp1, &(VECTOR3){0,0,45}, ROTATE_XYZ);
    Matrix4_fromViewAngles(target, sunangles, 1000, &tmp2);
    Matrix4_multiply(&tmp1, &tmp2, &view);
    Matrix4_translate(&view, &(VECTOR3){0,-500,0});
    Matrix4_multiply(&proj, &view, output);
}

void Matrix4_getCameraMatrix(viewCamera_t const *camera, LPMATRIX4 output) {
    MATRIX4 proj, view;
    size2_t const windowSize = re.GetWindowSize();
    float const aspect = (float)windowSize.width / (float)windowSize.height;
    Matrix4_perspective(&proj, camera->fov, aspect, camera->znear, camera->zfar);
    Matrix4_fromViewAngles(&camera->origin, &camera->viewangles, camera->distance, &view);
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

static void V_AddClientEntity(centity_t const *ent) {
    renderEntity_t re = { 0 };
    re.origin = Vector3_lerp(&ent->prev.origin, &ent->current.origin, cl.viewDef.lerpfrac);
    re.angle = LerpRotation(ent->prev.angle, ent->current.angle, cl.viewDef.lerpfrac);
    re.scale = LerpNumber(ent->prev.scale, ent->current.scale, cl.viewDef.lerpfrac);
    re.frame = ent->current.frame;
    re.oldframe = ent->prev.frame;
    re.model = cl.models[ent->current.model];
    re.skin = cl.pics[ent->current.image];
    re.team = ent->current.player;
    re.flags = ent->current.renderfx;
    re.radius = ent->current.radius;
    re.number = ent->current.number;

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
    re.model = cl.moveConfirmation;
    
    view_state.entities[view_state.num_entities++] = re;
}

static LINE3 CL_GetMouseLine(LPCVECTOR2 mouse) {
    LINE3 line;
    MATRIX4 cameramat;
    MATRIX4 invcammat;
    size2_t const windowSize = re.GetWindowSize();
    float const x = (mouse->x / (float)windowSize.width - 0.5) * 2;
    float const y = (0.5 - mouse->y / (float)windowSize.height) * 2;
    Matrix4_getCameraMatrix(&cl.viewDef.camera, &cameramat);
    Matrix4_inverse(&cameramat, &invcammat);
    line.a = Matrix4_multiply_vector3(&invcammat, &(VECTOR3) { x, y, 0 });
    line.b = Matrix4_multiply_vector3(&invcammat, &(VECTOR3) { x, y, 1 });
    return line;
}

static void CL_AddBuilding(void) {
    if (!cl.cursorEntity)
        return;

    renderEntity_t re;
    memset(&re, 0, sizeof(renderEntity_t));
    
    LINE3 const line = CL_GetMouseLine(&mouse.origin);
    CM_IntersectLineWithHeightmap(&line, &re.origin);

    re.origin.x = floor(re.origin.x / 32) * 32;
    re.origin.y = floor(re.origin.y / 32) * 32;
    re.origin.z = CM_GetHeightAtPoint(re.origin.x, re.origin.y);
    re.scale = cl.cursorEntity->scale;
    re.frame = cl.cursorEntity->frame;
    re.oldframe = cl.cursorEntity->frame;
    re.model = cl.models[cl.cursorEntity->model];
    
    cl.cursorEntity->origin = re.origin;
    
    view_state.entities[view_state.num_entities++] = re;
}

static void CL_AddEntities(void) {
    cl.viewDef.lerpfrac = (float)(cl.time - cl.frame.servertime) / FRAMETIME;

    FOR_LOOP(index, MAX_CLIENT_ENTITIES) {
        centity_t const *ce = &cl.ents[index];
        if (!ce->current.model)
            continue;
        V_AddClientEntity(ce);
    }
    
    FOR_LOOP(index, MAX_CONFIRMATION_OBJECTS) {
        if (cl.time - cl.confs[index].timespamp > 1000)
            continue;
        CL_AddConfirmationObject(&cl.confs[index]);
    }
    
    CL_AddBuilding();

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
    
    for (int i = 1; i < MAX_FONTSTYLES && *cl.configstrings[CS_FONTS + i]; i++) {
        if (cl.fonts[i])
            continue;
        LPCSTR fontspec = cl.configstrings[CS_FONTS + i];
        LPCSTR split = strstr(fontspec, ",");
        if (split) {
            PATHSTR filename = { 0 };
            memcpy(filename, fontspec, split - fontspec);
            DWORD fontsize = atoi(split+1);
            cl.fonts[i] = re.LoadFont(filename, fontsize);
        } else {
            cl.fonts[i] = re.LoadFont(cl.configstrings[CS_FONTS + i], 16);
        }
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
    Matrix4_getLightMatrix(&lightAngles, &cl.viewDef.camera.origin, VIEW_SHADOW_SIZE, &cl.viewDef.lightMatrix);

    V_ClearScene();
    CL_AddEntities();

    re.RenderFrame(&cl.viewDef);
    
//    re.DrawPic(tex1, 0, 0);
//    re.DrawPic(tex2, 512, 0);

    if (cl.selection.in_progress) {
        re.DrawSelectionRect(&cl.selection.rect, (COLOR32){0,255,0,255});
    }
}
