#include "cl_input_local.h"
#include "ui_layout.h"

static struct {
    BOOL active;
    VECTOR3 anchor;
} camera_drag;

static BOOL smart_click_active;
static BOOL cam_left, cam_right, cam_north, cam_south;

static BOOL CL_OrderQueueModifierDown(void) {
    return (SDL_GetModState() & (KMOD_LSHIFT | KMOD_RSHIFT)) != 0;
}

static BOOL CL_TracePan(float x, float y, LPVECTOR3 point) {
    return Cvar_Integer("cl_camera_pan_plane", 0)
        ? re.TraceCameraPlane(&cl.viewDef, x, y, point) : re.TraceLocation(&cl.viewDef, x, y, point);
}

void CL_RtsResetMap(void) {
    cam_left = cam_right = cam_north = cam_south = false;
}

static void CL_BeginPan(float x, float y) {
    if (!CL_GameplayInputReady()) {
        camera_drag.active = false;
        return;
    }
    camera_drag.active = CL_TracePan(x, y, &camera_drag.anchor);
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
    if (!CL_TracePan(x, y, &point)) {
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
        SZ_Printf(&cls.netchan.message, CL_OrderQueueModifierDown()
            ? "smart %d queue" : "smart %d", entnum);
    } else if (re.TraceLocation(&cl.viewDef, x, y, &point)) {
        MSG_WriteByte(&cls.netchan.message, clc_stringcmd);
        SZ_Printf(&cls.netchan.message, CL_OrderQueueModifierDown()
            ? "smartpoint %d %d queue" : "smartpoint %d %d",
            (int)point.x, (int)point.y);
    }

    if (cl.selection.num_selected) {
        CL_RequestUnitUI(cl.selection.num_selected, cl.selection.entity_nums);
    }
}

static void IN_PanDown(void) {
    if (camera_drag.active)
        return;
    CL_BeginPan(mouse.origin.x, mouse.origin.y);
}

static void IN_PanUp(void) {
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
    CL_SendSmartCommand(mouse.origin.x, mouse.origin.y);
}

static void IN_CamLeftDown(void) { cam_left = true; }
static void IN_CamLeftUp(void) { cam_left = false; }
static void IN_CamRightDown(void) { cam_right = true; }
static void IN_CamRightUp(void) { cam_right = false; }
static void IN_CamNorthDown(void) { cam_north = true; }
static void IN_CamNorthUp(void) { cam_north = false; }
static void IN_CamSouthDown(void) { cam_south = true; }
static void IN_CamSouthUp(void) { cam_south = false; }

/* `camera edge` is client-local input state. Other camera subcommands belong
 * to the game module, so forward them through the normal server command
 * path rather than duplicating camera simulation state in the client. */
static void CL_Camera_f(void) {
    if (Cmd_Argc() >= 2 && !strcasecmp(Cmd_Argv(1), "edge")) {
        if (Cmd_Argc() != 3 || (strcmp(Cmd_Argv(2), "0") && strcmp(Cmd_Argv(2), "1"))) {
            fprintf(stderr, "usage: camera edge <0|1>\n");
            return;
        }
        Cvar_Set("cl_camera_edge_scroll", Cmd_Argv(2));
        return;
    }
    if (Cmd_Argc() >= 2 && (!strcasecmp(Cmd_Argv(1), "move") ||
                            !strcasecmp(Cmd_Argv(1), "selected"))) {
        Cmd_ForwardToServer(Cmd_ArgsFrom(0));
        return;
    }
    fprintf(stderr, "usage: camera <move <x> <y>|edge <0|1>|selected>\n");
}

void CL_RtsInit(void) {
    Cmd_AddCommand("+pan", IN_PanDown);
    Cmd_AddCommand("-pan", IN_PanUp);
    Cmd_AddCommand("+smart", IN_SmartDown);
    Cmd_AddCommand("-smart", IN_SmartUp);
    Cmd_AddCommand("+camleft", IN_CamLeftDown);
    Cmd_AddCommand("-camleft", IN_CamLeftUp);
    Cmd_AddCommand("+camright", IN_CamRightDown);
    Cmd_AddCommand("-camright", IN_CamRightUp);
    Cmd_AddCommand("+camnorth", IN_CamNorthDown);
    Cmd_AddCommand("-camnorth", IN_CamNorthUp);
    Cmd_AddCommand("+camsouth", IN_CamSouthDown);
    Cmd_AddCommand("-camsouth", IN_CamSouthUp);
    Cmd_AddCommand("camera", CL_Camera_f);
    Cvar_Get("cl_camera_edge_scroll", "0", CVAR_ARCHIVE);
    Cvar_Get("cl_camera_scroll_speed", "0", CVAR_ARCHIVE);
    Cvar_Get("cl_camera_edge_margin", "6", CVAR_ARCHIVE);
    Cvar_Get("cl_camera_pan_plane", "0", 0);
}

static BOOL CL_CanHoverHealthEntity(DWORD entnum) {
    if (!entnum || entnum >= MAX_CLIENT_ENTITIES) {
        return false;
    }
    LPCENTITYSTATE const state = &cl.ents[entnum].current;
    return state->model &&
           state->stats[ENT_HEALTH] > 0 &&
           (state->flags & EF_HOVER_HEALTH) &&
           !(state->flags & EF_NOT_SELECTABLE);
}

void CL_RtsMouseMotion(SDL_MouseMotionEvent const *motion) {
    DWORD entnum = 0;
    BOOL trace_hit = false;

    if (!motion) {
        return;
    }
    if (!CL_GameplayInputReady()) {
        camera_drag.active = false;
        CL_EndMinimapDrag();
        cl.selection.in_progress = false;
        cl.hover_entity = 0;
        return;
    }
    if (!CL_MouseOverGameplayUI())
        trace_hit = re.TraceEntity(&cl.viewDef, (float)motion->x, (float)motion->y, &entnum);
    if (trace_hit && CL_CanHoverHealthEntity(entnum)) {
        cl.hover_entity = entnum;
    } else {
        cl.hover_entity = 0;
    }
    if (camera_drag.active) {
        CL_UpdatePan(motion->x, motion->y);
    }
    CL_UpdateMinimapDrag(motion->x, motion->y);
    if (cl.selection.in_progress) {
        cl.selection.rect.w = motion->x - cl.selection.rect.x;
        cl.selection.rect.h = motion->y - cl.selection.rect.y;
        SCR_LayoutClampSelectionRect(&cl.selection.rect);
    }
}

BOOL CL_RtsMouseWheel(SDL_MouseWheelEvent const *wheel) {
    (void)wheel;
    return false;
}

/* Camera scrolling: +cam* binds and screen-edge push. Runs every client frame.
 * World +Y is north (up on screen), +X is east (right). */
void CL_RtsFrame(void) {
    static DWORD last_ms = 0;
    DWORD now = SDL_GetTicks();
    float dt = (last_ms && now > last_ms) ? (now - last_ms) / 1000.0f : 0.0f;
    last_ms = now;
    if (dt > 0.1f) dt = 0.1f; /* clamp after a stall */

    /* A server-authored modal owns input completely; terminate any world drag
     * that began before the modal arrived. */
    if (!CL_GameplayInputReady()) {
        camera_drag.active = false;
        CL_EndMinimapDrag();
        cl.selection.in_progress = false;
        cl.hover_entity = 0;
        return;
    }
    /* Drag-pan takes over; don't fight it. */
    if (camera_drag.active || dt <= 0.0f) {
        return;
    }

    float dx = 0.0f, dy = 0.0f;
    if (cam_left)  dx -= 1.0f;
    if (cam_right) dx += 1.0f;
    if (cam_north) dy += 1.0f;
    if (cam_south) dy -= 1.0f;

    /* Screen-edge scrolling (only while the cursor is inside the window). */
    size2_t win = re.GetWindowSize();
    float mx = mouse.origin.x, my = mouse.origin.y, margin = Cvar_Value("cl_camera_edge_margin", 6);
    if (Cvar_Value("cl_camera_edge_scroll", 1.0f) != 0.0f && win.width > 0 && win.height > 0 &&
        mx >= 0 && my >= 0 && mx < win.width && my < win.height) {
        if (mx <= margin)               dx -= 1.0f;
        if (mx >= (float)win.width - 1 - margin)  dx += 1.0f;
        if (my <= margin)               dy += 1.0f; /* top of screen = north */
        if (my >= (float)win.height - 1 - margin) dy -= 1.0f;
    }

    if (dx == 0.0f && dy == 0.0f) {
        return;
    }

    VECTOR2 position;
    float step = Cvar_Value("cl_camera_scroll_speed", 0) * dt;
    position.x = cl.viewDef.camerastate[0].origin.x + dx * step;
    position.y = cl.viewDef.camerastate[0].origin.y + dy * step;
    CL_SetCameraPosition(position);
}

#ifdef BZ_TESTS
#include "shared/test.h"
static DWORD pan_terrain, pan_plane;
static bool CL_TestTerrain(viewDef_t const *view, float x, float y, LPVECTOR3 point) {
    (void)view; (void)x; (void)y;
    pan_terrain++; *point = (VECTOR3){ 1, 2, 3 }; return true;
}
static bool CL_TestPlane(viewDef_t const *view, float x, float y, LPVECTOR3 point) {
    (void)view; (void)x; (void)y;
    pan_plane++; *point = (VECTOR3){ 4, 5, 6 }; return true;
}
TEST(client_input, pan_uses_configured_surface) {
    refExport_t saved = re;
    FLOAT old = Cvar_Value("cl_camera_pan_plane", 0);
    VECTOR3 point;
    re.TraceLocation = CL_TestTerrain;
    re.TraceCameraPlane = CL_TestPlane;
    pan_terrain = pan_plane = 0;
    Cvar_Set("cl_camera_pan_plane", "0");
    T_ASSERT(CL_TracePan(0, 0, &point));
    T_EQ(pan_terrain, 1); T_EQ(pan_plane, 0); T_FEQ(point.z, 3, 0.001f);
    Cvar_Set("cl_camera_pan_plane", "1");
    T_ASSERT(CL_TracePan(0, 0, &point));
    T_EQ(pan_terrain, 1); T_EQ(pan_plane, 1); T_FEQ(point.z, 6, 0.001f);
    Cvar_SetValue("cl_camera_pan_plane", old);
    re = saved;
}
#endif
