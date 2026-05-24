#include <stdlib.h> // atoi()

#include "client.h"
#include "renderer.h"

static struct {
    renderEntity_t entities[MAX_CLIENT_ENTITIES];
    int num_entities;
} view_state;

static bool world_loaded = false;

VECTOR3 lightAngles = {-40,0,60};

static DWORD CL_CountConfigstrings(DWORD start, DWORD max) {
    DWORD count = 0;

    for (DWORD i = 1; i < max && start + i < MAX_CONFIGSTRINGS; i++) {
        if (*cl.configstrings[start + i]) {
            count++;
        }
    }
    return count;
}

static DWORD CL_CountLoadedConfigstrings(DWORD start, DWORD max, HANDLE const *handles) {
    DWORD count = 0;

    for (DWORD i = 1; i < max && start + i < MAX_CONFIGSTRINGS; i++) {
        if (*cl.configstrings[start + i] && handles[i]) {
            count++;
        }
    }
    return count;
}

static void CL_UpdateAssetLoadingProgress(LPCSTR status, DWORD loaded, DWORD total) {
    FLOAT progress;

    if (!total) {
        total = 1;
    }
    progress = (FLOAT)loaded / (FLOAT)total;
    CL_LoadingUpdate(status, progress);
}

static void CL_SendDeferredBegin(void) {
    if (!cl.pending_begin) {
        return;
    }
    MSG_WriteByte(&cls.netchan.message, clc_stringcmd);
    MSG_WriteString(&cls.netchan.message, "begin");
    cl.pending_begin = false;
}

void Matrix4_fromViewAngles(LPCVECTOR3 target, LPCVECTOR3 angles, FLOAT distance, LPMATRIX4 output) {
    VECTOR3 const vieworg = Vector3_unm(target);
    Matrix4_identity(output);
    Matrix4_translate(output, &(VECTOR3){0, 0, -distance});
    Matrix4_rotate(output, angles, ROTATE_ZYX);
    Matrix4_translate(output, &vieworg);
}

void Matrix4_fromViewQuat(LPCVECTOR3 target, LPCQUATERNION quat, FLOAT distance, LPMATRIX4 output) {
    VECTOR3 const vieworg = Vector3_unm(target);
    Matrix4_identity(output);
    Matrix4_translate(output, &(VECTOR3){0, 0, -distance});
    Matrix4_rotateQuat(output, quat);
    Matrix4_translate(output, &vieworg);
}

void Matrix4_getLightMatrix(LPCVECTOR3 sunangles, FLOAT scale, LPMATRIX4 output) {
    MATRIX4 proj, view, tmp1, tmp2;
    viewCamera_t const *a = cl.viewDef.camerastate+1;
    viewCamera_t const *b = cl.viewDef.camerastate+0;
    VECTOR3 const target = Vector3_lerp(&a->origin, &b->origin, cl.viewDef.lerpfrac);
    Matrix4_ortho(&proj, -scale, scale, -scale, scale, -1000.0, 3000.0);
    Matrix4_identity(&tmp1);
    Matrix4_rotate(&tmp1, &(VECTOR3){0,0,45}, ROTATE_XYZ);
    Matrix4_fromViewAngles(&target, sunangles, 1000, &tmp2);
    Matrix4_multiply(&tmp1, &tmp2, &view);
    Matrix4_translate(&view, &(VECTOR3){0,-500,0});
    Matrix4_multiply(&proj, &view, output);
}

static void Matrix4_getPreviewCameraMatrix(LPCVECTOR3 target, LPMATRIX4 output) {
    MATRIX4 proj, view;
    size2_t windowSize = re.GetWindowSize();
    VECTOR3 eye = { 520.0f, -420.0f, 220.0f };
    VECTOR3 dir = Vector3_sub(target, &eye);
    FLOAT aspect = (FLOAT)windowSize.width / (FLOAT)windowSize.height;

    Matrix4_perspective(&proj, 35.0f, aspect, 10.0f, 4000.0f);
    Matrix4_lookAt(&view, &eye, &dir, &(VECTOR3){0, 0, 1});
    Matrix4_multiply(&proj, &view, output);
}

static void Matrix4_getPreviewLightMatrix(LPCVECTOR3 sunangles, LPCVECTOR3 target, float scale, LPMATRIX4 output) {
    MATRIX4 proj, view;
    Matrix4_ortho(&proj, -scale, scale, -scale, scale, -1000.0, 3000.0);
    Matrix4_fromViewAngles(target, sunangles, 1000, &view);
    Matrix4_multiply(&proj, &view, output);
}

void Matrix4_getCameraMatrix(LPMATRIX4 output) {
    if (!world_loaded) {
        Matrix4_identity(output);
        return;
    }
    MATRIX4 proj, view;
    size2_t windowSize = re.GetWindowSize();
    viewCamera_t *a = cl.viewDef.camerastate+1;
    viewCamera_t *b = cl.viewDef.camerastate+0;
    VECTOR3 origin = Vector3_lerp(&a->origin, &b->origin, cl.viewDef.lerpfrac);
    QUATERNION quat = Quaternion_slerp(&a->viewquat, &b->viewquat, cl.viewDef.lerpfrac);
    FLOAT distance = LerpNumber(a->distance, b->distance, cl.viewDef.lerpfrac);
    FLOAT fov = LerpNumber(a->fov, b->fov, cl.viewDef.lerpfrac);
    FLOAT aspect = (FLOAT)windowSize.width / (FLOAT)windowSize.height;
    FLOAT znear = LerpNumber(a->znear, b->znear, cl.viewDef.lerpfrac);
    FLOAT zfar = LerpNumber(a->zfar, b->zfar, cl.viewDef.lerpfrac);
    
    origin.z = CM_GetHeightAtPoint(origin.x, origin.y) - 128;

    Matrix4_perspective(&proj, fov, aspect, znear, zfar);
    Matrix4_fromViewQuat(&origin, &quat, distance, &view);
    Matrix4_multiply(&proj, &view, output);
}

FLOAT LerpRotation(FLOAT a, FLOAT b, FLOAT t) {
    if (b < 0) {
        b = b + 2 * M_PI;
    }
    FLOAT apos = a + 2 * M_PI;
    FLOAT aneg = a - 2 * M_PI;
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
    if (view_state.num_entities >= MAX_CLIENT_ENTITIES) {
        return;
    }
    if (ent->current.model >= MAX_MODELS) {
        return;
    }
    
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
    re.healthbar = cl.healthbar;

    view_state.entities[view_state.num_entities++] = re;
    
    if (ent->current.model2 > 0) {
        if (ent->current.model2 >= MAX_MODELS) {
            return;
        }
        if (view_state.num_entities >= MAX_CLIENT_ENTITIES) {
            return;
        }
        re.model = cl.models[ent->current.model2];
        re.skin = 0;
        re.frame = 0;
        re.oldframe = 0;
        re.scale = 1;
        re.flags |= RF_NO_SHADOW;
        if (ent->current.renderfx & RF_ATTACH_OVERHEAD) {
            re.origin.z += re.radius * 2.5;
        }
        view_state.entities[view_state.num_entities++] = re;
    }
}

static void V_ClearScene(void) {
    view_state.num_entities = 0;
    cl.viewDef.num_entities = 0;
}

static void CL_AddBuilding(void) {
    if (!cl.cursorEntity)
        return;
    if (view_state.num_entities >= MAX_CLIENT_ENTITIES)
        return;
    if (cl.cursorEntity->model >= MAX_MODELS)
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
    static bool map_registered = false;
    static bool map_load_announced = false;
    static bool loading_complete_displayed = false;
    static DWORD loading_settle_frames = 0;
    static PATHSTR registered_map;
    DWORD total_assets;
    DWORD loaded_assets;

    if (!*cl.configstrings[CS_WORLD]) {
        CL_LoadingUpdate("Awaiting configstrings", 0.0f);
        return;
    }

    if (strcmp(registered_map, cl.configstrings[CS_WORLD])) {
        map_registered = false;
        map_load_announced = false;
        loading_complete_displayed = false;
        loading_settle_frames = 0;
        world_loaded = false;
        snprintf(registered_map, sizeof(registered_map), "%s", cl.configstrings[CS_WORLD]);
    }

    total_assets = 1 +
                   (*cl.configstrings[CS_HEALTHBAR] ? 1 : 0) +
                   CL_CountConfigstrings(CS_MODELS, MAX_MODELS) +
                   CL_CountConfigstrings(CS_IMAGES, MAX_IMAGES) +
                   CL_CountConfigstrings(CS_FONTS, MAX_FONTSTYLES);
    loaded_assets = (map_registered ? 1 : 0) +
                    (cl.healthbar ? 1 : 0) +
                    CL_CountLoadedConfigstrings(CS_MODELS, MAX_MODELS, (HANDLE const *)cl.models) +
                    CL_CountLoadedConfigstrings(CS_IMAGES, MAX_IMAGES, (HANDLE const *)cl.pics) +
                    CL_CountLoadedConfigstrings(CS_FONTS, MAX_FONTSTYLES, (HANDLE const *)cl.fonts);

    if (loaded_assets < total_assets) {
        loading_complete_displayed = false;
        loading_settle_frames = 0;
    }
    
    if (!map_registered) {
        if (!map_load_announced) {
            CL_UpdateAssetLoadingProgress("Loading world", loaded_assets, total_assets);
            map_load_announced = true;
            return;
        }
        re.RegisterMap(cl.configstrings[CS_WORLD]);
        map_registered = true;
        world_loaded = true;
        CL_UpdateAssetLoadingProgress("Loading world", loaded_assets + 1, total_assets);
        return;
    }
    
    if (*cl.configstrings[CS_HEALTHBAR] && !cl.healthbar) {
        cl.healthbar = re.LoadTexture(cl.configstrings[CS_HEALTHBAR]);
        CL_UpdateAssetLoadingProgress("Loading health bars", loaded_assets + 1, total_assets);
        return;
    }

    for (DWORD i = 1; i < MAX_MODELS; i++) {
        if (!*cl.configstrings[CS_MODELS + i])
            continue;
        if (cl.models[i])
            continue;
        LPCSTR filename = cl.configstrings[CS_MODELS + i];
        PATHSTR portrait = { 0 };
        LPCSTR ext = strstr(filename, ".m");
        if (ext) {
            memcpy(portrait, filename, ext - filename);
            sprintf(portrait + strlen(portrait), "_Portrait%s", ext);
        }
        cl.models[i] = re.LoadModel(filename);
        if (portrait[0] && FS_FileExists(portrait)) {
            cl.portraits[i] = re.LoadModel(portrait);
        }
        CL_UpdateAssetLoadingProgress("Loading models", loaded_assets + 1, total_assets);
        return;
    }
    
    for (DWORD i = 1; i < MAX_IMAGES; i++) {
        if (!*cl.configstrings[CS_IMAGES + i])
            continue;
        if (cl.pics[i])
            continue;
        cl.pics[i] = re.LoadTexture(cl.configstrings[CS_IMAGES + i]);
        CL_UpdateAssetLoadingProgress("Loading textures", loaded_assets + 1, total_assets);
        return;
    }
    
    for (DWORD i = 1; i < MAX_FONTSTYLES; i++) {
        if (!*cl.configstrings[CS_FONTS + i])
            continue;
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
        CL_UpdateAssetLoadingProgress("Loading fonts", loaded_assets + 1, total_assets);
        return;
    }

    if (world_loaded && cls.state == ca_active &&
        cl.playerstate.client_ui_state == CLIENT_UI_LOADING) {
        if (cl.pending_begin) {
            CL_LoadingUpdate("Starting game", 1.0f);
            CL_SendDeferredBegin();
            loading_complete_displayed = false;
            loading_settle_frames = 0;
            return;
        }
        if (!loading_complete_displayed) {
            CL_LoadingUpdate("Finishing", 1.0f);
            loading_complete_displayed = true;
            return;
        }
        if (loading_settle_frames < 3) {
            loading_settle_frames++;
            return;
        }
        CL_LoadingUpdate("Entering game", 1.0f);
        cl.playerstate.client_ui_state = CLIENT_UI_GAME;
    }
}

void V_RenderView(void) {

#ifdef DEBUG_PATHFINDING
    extern LPCOLOR32 pathDebug;
    if (pathDebug)
    re.SetPathTexture(pathDebug);
#endif
    
    static DWORD lastTime = 0;
    if (!world_loaded) {
        VECTOR3 target = { 0, 0, 90 };

        cl.viewDef.viewport = (RECT) { 0, 0, 1, 1 };
        cl.viewDef.scissor = (RECT) { 0, 0, 1, 1 };
        cl.viewDef.time = cl.time;
        cl.viewDef.deltaTime = cl.time - lastTime;
        cl.viewDef.rdflags = RDF_NOWORLDMODEL | RDF_NOFRUSTUMCULL;

        V_ClearScene();
        Matrix4_getPreviewCameraMatrix(&target, &cl.viewDef.viewProjectionMatrix);
        Matrix4_getPreviewLightMatrix(&lightAngles, &target, VIEW_SHADOW_SIZE, &cl.viewDef.lightMatrix);
        Matrix4_identity(&cl.viewDef.textureMatrix);

        re.RenderFrame(&cl.viewDef);
        lastTime = cl.time;
        return;
    }

    cl.viewDef.lerpfrac = (FLOAT)(cl.time - cl.frame.servertime) / FRAMETIME;
    cl.viewDef.viewport = (RECT) { 0, 0, 1, 1 };
    cl.viewDef.scissor = (RECT) { 0, 0.22, 1, 0.76 };
    cl.viewDef.time = cl.time;
    cl.viewDef.deltaTime = cl.time - lastTime;
    cl.viewDef.rdflags = cl.playerstate.rdflags;
    
    Matrix4_getCameraMatrix(&cl.viewDef.viewProjectionMatrix);
    Matrix4_getLightMatrix(&lightAngles, VIEW_SHADOW_SIZE, &cl.viewDef.lightMatrix);

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
    if (view_state.num_entities >= MAX_CLIENT_ENTITIES) {
        return;
    }
    view_state.entities[view_state.num_entities++] = *ent;
}

void V_Shutdown(void) {
}
