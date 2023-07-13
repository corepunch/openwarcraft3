#include <stdlib.h> // atoi()

#include "client.h"
#include "renderer.h"

static struct {
    renderEntity_t entities[MAX_CLIENT_ENTITIES];
    int num_entities;
} view_state;

VECTOR3 lightAngles = {-40,0,60};

void Matrix4_fromViewAngles(LPCVECTOR3 target, LPCVECTOR3 angles, float distance, LPMATRIX4 output) {
    VECTOR3 const vieworg = Vector3_unm(target);
    Matrix4_identity(output);
    Matrix4_translate(output, &(VECTOR3){0, 0, -distance});
    Matrix4_rotate(output, angles, ROTATE_ZYX);
    Matrix4_translate(output, &vieworg);
}

void Matrix4_getLightMatrix(LPCVECTOR3 sunangles, LPCVECTOR3 target, float scale, LPMATRIX4 output) {
    MATRIX4 proj, view, tmp1, tmp2;
    Matrix4_ortho(&proj, -scale, scale, -scale, scale, -1000.0, 3000.0);
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
    re.splat = cl.pics[ent->current.splat & 0xffff];
    re.splatsize = ent->current.splat >> 16;
    re.health = BYTE2FLOAT(ent->current.stats[ENT_HEALTH]);

    view_state.entities[view_state.num_entities++] = re;

}

static void V_ClearScene(void) {
    view_state.num_entities = 0;
    cl.viewDef.num_entities = 0;
}

static void CL_AddBuilding(void) {
    if (!cl.cursorEntity)
        return;

    renderEntity_t ent;
    memset(&ent, 0, sizeof(renderEntity_t));
    
    re.TraceLocation(&cl.viewDef, mouse.origin.x, mouse.origin.y, &ent.origin);

    ent.origin.x = floor(ent.origin.x / 32) * 32;
    ent.origin.y = floor(ent.origin.y / 32) * 32;
    ent.origin.z = CM_GetHeightAtPoint(ent.origin.x, ent.origin.y);
    ent.scale = cl.cursorEntity->scale;
    ent.frame = cl.cursorEntity->frame;
    ent.oldframe = cl.cursorEntity->frame;
    ent.model = cl.models[cl.cursorEntity->model];
    
    cl.cursorEntity->origin = ent.origin;
    
    view_state.entities[view_state.num_entities++] = ent;
}

static void CL_AddEntities(void) {
    cl.viewDef.lerpfrac = (float)(cl.time - cl.frame.servertime) / FRAMETIME;

    FOR_LOOP(index, MAX_CLIENT_ENTITIES) {
        centity_t const *ce = &cl.ents[index];
        if (!ce->current.model)
            continue;
        V_AddClientEntity(ce);
    }
    
    CL_AddTEnts();
    
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
        LPCSTR filename = cl.configstrings[CS_MODELS + i];
        PATHSTR portrait = { 0 };
        LPCSTR ext = strstr(filename, ".m");
        memcpy(portrait, filename, ext - filename);
        sprintf(portrait + strlen(portrait), "_Portrait%s", ext);
        cl.models[i] = re.LoadModel(filename);
        if (FS_FileExists(portrait)) {
            cl.portraits[i] = re.LoadModel(portrait);
        }
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
    if (pathDebug)
    re.SetPathTexture(pathDebug);
#endif
    
    static DWORD lastTime = 0;
    
    cl.viewDef.viewport = (RECT) { 0, 0, 1, 1 };
    cl.viewDef.scissor = (RECT) { 0, 0.2, 1, 0.8 };
    cl.viewDef.time = cl.time;
    cl.viewDef.deltaTime = cl.time - lastTime;
    
    Matrix4_getCameraMatrix(&cl.viewDef.camera, &cl.viewDef.viewProjectionMatrix);
    Matrix4_getLightMatrix(&lightAngles, &cl.viewDef.camera.origin, VIEW_SHADOW_SIZE, &cl.viewDef.lightMatrix);

    V_ClearScene();
    CL_AddEntities();

    re.RenderFrame(&cl.viewDef);
    
//    re.DrawPic(tex1, 0, 0);
//    re.DrawPic(tex2, 512, 0);

    if (cl.selection.in_progress) {
        re.DrawSelectionRect(&cl.selection.rect, (COLOR32){0,255,0,255});
    }
    
    lastTime = cl.time;
}

void V_AddEntity(renderEntity_t *ent) {
    view_state.entities[view_state.num_entities++] = *ent;
}
