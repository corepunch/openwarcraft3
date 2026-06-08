#include "cl_input_local.h"

#ifndef WOW
static struct {
    BOOL active;
    VECTOR3 anchor;
} camera_drag;

static void CL_SetCameraPosition(VECTOR2 position) {
    cl.viewDef.camerastate[0].origin.x = position.x;
    cl.viewDef.camerastate[0].origin.y = position.y;
    cl.viewDef.camerastate[1].origin.x = position.x;
    cl.viewDef.camerastate[1].origin.y = position.y;
    cl.camera_prediction.active = true;
    cl.camera_prediction.origin = position;

    MSG_WriteByte(&cls.netchan.message, clc_camera_position);
    MSG_WriteFloat(&cls.netchan.message, position.x);
    MSG_WriteFloat(&cls.netchan.message, position.y);
}

#if defined(SC2) && defined(SC2_CAMERA_HACK)
#define SC2_CAMERA_HACK_MIN_DISTANCE  5.0f
#define SC2_CAMERA_HACK_MAX_DISTANCE  120.0f
#define SC2_CAMERA_HACK_ZOOM_STEP     2.0f
#define SC2_CAMERA_HACK_TURN_SPEED    0.18f
#define SC2_CAMERA_HACK_MIN_PITCH     10.0f
#define SC2_CAMERA_HACK_MAX_PITCH     85.0f
#define SC2_CAMERA_HACK_CELL_SIZE     1.0f

static struct {
    BOOL initialized;
    BOOL rotating;
    BOOL has_orbit_anchor;
    FLOAT distance;
    VECTOR3 angles;
    VECTOR3 orbit_anchor;
    VECTOR2 orbit_screen;
} sc2_camera_hack;

static FLOAT CL_ClampFloat(FLOAT value, FLOAT min_value, FLOAT max_value) {
    return MAX(min_value, MIN(value, max_value));
}

static BOOL CL_Sc2CameraHackShiftMiddleDown(void) {
    SDL_Keymod mod = SDL_GetModState();
    Uint32 buttons = SDL_GetMouseState(NULL, NULL);

    return (mod & (KMOD_LSHIFT | KMOD_RSHIFT)) != 0 &&
           (buttons & SDL_BUTTON(SDL_BUTTON_MIDDLE)) != 0;
}

static void CL_Sc2CameraHackInit(void) {
    viewCamera_t const *camera = &cl.viewDef.camerastate[0];

    if (sc2_camera_hack.initialized) {
        return;
    }
    sc2_camera_hack.initialized = true;
    sc2_camera_hack.distance = camera->distance > 0.0f ? camera->distance : 34.07f;
    sc2_camera_hack.angles = camera->viewangles;
    if (sc2_camera_hack.angles.x == 0.0f) {
        sc2_camera_hack.angles.x = 56.0f;
    }
    if (sc2_camera_hack.angles.y == 0.0f) {
        sc2_camera_hack.angles.y = 180.0f;
    }
}

static void CL_Sc2CameraHackApply(void) {
    CL_Sc2CameraHackInit();
    cl.playerstate.distance = sc2_camera_hack.distance;
    cl.playerstate.viewangles = sc2_camera_hack.angles;
    cl.viewDef.camerastate[0].distance = sc2_camera_hack.distance;
    cl.viewDef.camerastate[1].distance = sc2_camera_hack.distance;
    cl.viewDef.camerastate[0].viewangles = sc2_camera_hack.angles;
    cl.viewDef.camerastate[1].viewangles = sc2_camera_hack.angles;
}

static void CL_Sc2CameraHackUpdateOrbit(void) {
    VECTOR3 point;
    VECTOR2 position;

    if (!sc2_camera_hack.has_orbit_anchor) {
        return;
    }
    Matrix4_getCameraMatrix(&cl.viewDef.viewProjectionMatrix);
    if (!re.TraceLocation(&cl.viewDef, sc2_camera_hack.orbit_screen.x, sc2_camera_hack.orbit_screen.y, &point)) {
        return;
    }
    position.x = cl.viewDef.camerastate[0].origin.x + sc2_camera_hack.orbit_anchor.x - point.x;
    position.y = cl.viewDef.camerastate[0].origin.y + sc2_camera_hack.orbit_anchor.y - point.y;
    CL_SetCameraPosition(position);
    Matrix4_getCameraMatrix(&cl.viewDef.viewProjectionMatrix);
}

static BOOL CL_Sc2CameraHackBeginRotate(void) {
    if (!CL_GameplayInputReady() || !CL_Sc2CameraHackShiftMiddleDown()) {
        return false;
    }
    CL_Sc2CameraHackInit();
    sc2_camera_hack.rotating = true;
    sc2_camera_hack.orbit_screen = MAKE(VECTOR2, mouse.origin.x, mouse.origin.y);
    Matrix4_getCameraMatrix(&cl.viewDef.viewProjectionMatrix);
    sc2_camera_hack.has_orbit_anchor = re.TraceLocation(&cl.viewDef,
                                                        sc2_camera_hack.orbit_screen.x,
                                                        sc2_camera_hack.orbit_screen.y,
                                                        &sc2_camera_hack.orbit_anchor);
    return true;
}

static void CL_Sc2CameraHackEndRotate(void) {
    sc2_camera_hack.rotating = false;
    sc2_camera_hack.has_orbit_anchor = false;
}

static BOOL CL_Sc2CameraHackMouseMotion(SDL_MouseMotionEvent const *motion) {
    if (!motion || !sc2_camera_hack.rotating) {
        return false;
    }
    CL_Sc2CameraHackInit();
    sc2_camera_hack.angles.y += motion->xrel * SC2_CAMERA_HACK_TURN_SPEED;
    sc2_camera_hack.angles.x = CL_ClampFloat(sc2_camera_hack.angles.x + motion->yrel * SC2_CAMERA_HACK_TURN_SPEED,
                                             SC2_CAMERA_HACK_MIN_PITCH,
                                             SC2_CAMERA_HACK_MAX_PITCH);
    CL_Sc2CameraHackApply();
    CL_Sc2CameraHackUpdateOrbit();
    return true;
}

static BOOL CL_Sc2CameraHackMouseWheel(SDL_MouseWheelEvent const *wheel) {
    if (!wheel || !CL_GameplayInputReady()) {
        return false;
    }
    CL_Sc2CameraHackInit();
    sc2_camera_hack.distance = CL_ClampFloat(sc2_camera_hack.distance - wheel->y * SC2_CAMERA_HACK_ZOOM_STEP,
                                             SC2_CAMERA_HACK_MIN_DISTANCE,
                                             SC2_CAMERA_HACK_MAX_DISTANCE);
    CL_Sc2CameraHackApply();
    return true;
}

static void CL_Sc2CameraHackPrintTile(FLOAT screen_x, FLOAT screen_y) {
    BOX2 bounds;
    VECTOR3 point;
    DWORD width;
    DWORD height;
    DWORD tile_x;
    DWORD tile_y;
    DWORD tile_index;
    DWORD cliff_x;
    DWORD cliff_y;
    DWORD cliff_width;
    DWORD cliff_index;

    if (!CL_GameplayInputReady() || CL_MouseOverGameplayUI())
        return;
    if (!re.TraceLocation(&cl.viewDef, screen_x, screen_y, &point))
        return;
    bounds = CM_GetWorldBounds();
    if (point.x < bounds.min.x || point.y < bounds.min.y ||
        point.x >= bounds.max.x || point.y >= bounds.max.y)
        return;
    width = (DWORD)MAX(1.0f, floorf((bounds.max.x - bounds.min.x) / SC2_CAMERA_HACK_CELL_SIZE + 0.5f));
    height = (DWORD)MAX(1.0f, floorf((bounds.max.y - bounds.min.y) / SC2_CAMERA_HACK_CELL_SIZE + 0.5f));
    tile_x = (DWORD)floorf((point.x - bounds.min.x) / SC2_CAMERA_HACK_CELL_SIZE);
    tile_y = (DWORD)floorf((point.y - bounds.min.y) / SC2_CAMERA_HACK_CELL_SIZE);
    tile_x = MIN(tile_x, width - 1);
    tile_y = MIN(tile_y, height - 1);
    tile_index = tile_x + tile_y * width;
    cliff_width = MAX(1, (width + 1) / 2);
    cliff_x = tile_x / 2;
    cliff_y = tile_y / 2;
    cliff_index = cliff_x + cliff_y * cliff_width;
    fprintf(stderr,
            "SC2 tile pick: world=(%.3f %.3f %.3f) tile=(%u,%u) tile_index=%u cliff=(%u,%u) cliff_index=%u\n",
            point.x,
            point.y,
            point.z,
            (unsigned)tile_x,
            (unsigned)tile_y,
            (unsigned)tile_index,
            (unsigned)cliff_x,
            (unsigned)cliff_y,
            (unsigned)cliff_index);
}
#endif

static BOOL smart_click_active;

static void CL_BeginPan(float x, float y) {
    if (!CL_GameplayInputReady()) {
        camera_drag.active = false;
        return;
    }
    camera_drag.active = re.TraceLocation(&cl.viewDef, x, y, &camera_drag.anchor);
}

static void CL_UpdatePan(float x, float y) {
    VECTOR3 point;
    VECTOR2 position;

    if (!CL_GameplayInputReady()) {
        camera_drag.active = false;
        return;
    }
    if (!camera_drag.active) {
        CL_BeginPan(x, y);
        return;
    }
    if (!re.TraceLocation(&cl.viewDef, x, y, &point)) {
        return;
    }

    position.x = cl.viewDef.camerastate[0].origin.x + camera_drag.anchor.x - point.x;
    position.y = cl.viewDef.camerastate[0].origin.y + camera_drag.anchor.y - point.y;
    CL_SetCameraPosition(position);
}

static void CL_EndPan(void) {
    camera_drag.active = false;
}

static void CL_SendSmartCommand(float x, float y) {
    DWORD entnum;
    VECTOR3 point;

    if (!CL_GameplayInputReady()) {
        return;
    }
    if (CL_MouseOverGameplayUI()) {
        return;
    }
    if (re.TraceEntity(&cl.viewDef, x, y, &entnum)) {
        MSG_WriteByte(&cls.netchan.message, clc_stringcmd);
        SZ_Printf(&cls.netchan.message, "smart %d", entnum);
    } else if (re.TraceLocation(&cl.viewDef, x, y, &point)) {
        MSG_WriteByte(&cls.netchan.message, clc_stringcmd);
        SZ_Printf(&cls.netchan.message, "smartpoint %d %d", (int)point.x, (int)point.y);
    }

    if (cl.selection.num_selected) {
        CL_RequestUnitUI(cl.selection.num_selected, cl.selection.entity_nums);
    }
}

static void IN_PanDown(void) {
#if defined(SC2) && defined(SC2_CAMERA_HACK)
    if (CL_Sc2CameraHackBeginRotate()) {
        CL_EndPan();
        return;
    }
#endif
    CL_BeginPan(mouse.origin.x, mouse.origin.y);
}

static void IN_PanUp(void) {
#if defined(SC2) && defined(SC2_CAMERA_HACK)
    CL_Sc2CameraHackEndRotate();
#endif
    CL_EndPan();
}

static void IN_SmartDown(void) {
    if (!CL_GameplayInputReady()) {
        smart_click_active = false;
        return;
    }
    smart_click_active = true;
}

static void IN_SmartUp(void) {
    if (!CL_GameplayInputReady()) {
        smart_click_active = false;
        return;
    }
    if (!smart_click_active) {
        return;
    }
    smart_click_active = false;
#if defined(SC2) && defined(SC2_CAMERA_HACK)
    CL_Sc2CameraHackPrintTile(mouse.origin.x, mouse.origin.y);
#endif
    CL_SendSmartCommand(mouse.origin.x, mouse.origin.y);
}

void CL_InputModeInit(void) {
    Cmd_AddCommand("+pan", IN_PanDown);
    Cmd_AddCommand("-pan", IN_PanUp);
    Cmd_AddCommand("+smart", IN_SmartDown);
    Cmd_AddCommand("-smart", IN_SmartUp);
}

void CL_InputModeSetGameplay(void) {
#ifdef SC2
    cl.viewDef.camerastate[0].zfar = 400;
    cl.viewDef.camerastate[0].znear = 0.1f;
    cl.viewDef.camerastate[1].zfar = 400;
    cl.viewDef.camerastate[1].znear = 0.1f;
#else
    if (!cl.moveConfirmation)
        cl.moveConfirmation = re.LoadModel("UI\\Feedback\\Confirmation\\Confirmation.mdx");
    cl.viewDef.camerastate[0].zfar = 5000;
    cl.viewDef.camerastate[0].znear = 100;
    cl.viewDef.camerastate[1].zfar = 5000;
    cl.viewDef.camerastate[1].znear = 100;
#endif
}

void CL_InputModeMouseMotion(SDL_MouseMotionEvent const *motion) {
    if (!motion) {
        return;
    }
#if defined(SC2) && defined(SC2_CAMERA_HACK)
    if (CL_Sc2CameraHackMouseMotion(motion)) {
        return;
    }
#endif
    if (camera_drag.active) {
        CL_UpdatePan(motion->x, motion->y);
    }
    if (cl.selection.in_progress) {
        cl.selection.rect.w = motion->x - cl.selection.rect.x;
        cl.selection.rect.h = motion->y - cl.selection.rect.y;
    }
}

BOOL CL_InputModeMouseWheel(SDL_MouseWheelEvent const *wheel) {
#if defined(SC2) && defined(SC2_CAMERA_HACK)
    if (CL_Sc2CameraHackMouseWheel(wheel)) {
        return true;
    }
#endif
    (void)wheel;
    return false;
}

void CL_InputModeFrame(void) {
#if defined(SC2) && defined(SC2_CAMERA_HACK)
    if (CL_GameplayInputReady()) {
        CL_Sc2CameraHackApply();
    }
#endif
}
#endif
