#include "cl_input_local.h"

#define BZ_MOVE_FORWARD 1 // bit; move command wire value for forward input
#define BZ_MOVE_BACK 2 // bit; move command wire value for backward input
#define BZ_MOVE_LEFT 4 // bit; move command wire value for left input
#define BZ_MOVE_RIGHT 8 // bit; move command wire value for right input

static struct {
    BOOL initialized;
    BOOL right_mouse;
    BOOL left_mouse;
    BOOL move_forward;
    BOOL move_back;
    BOOL move_left;
    BOOL move_right;
    DWORD last_time;
    FLOAT yaw;
    FLOAT pitch;
    /* Click-vs-drag tracking for LMB and RMB. */
    BOOL lmb_down;
    BOOL rmb_dragging;
    VECTOR2 lmb_down_pos;
    VECTOR2 rmb_down_pos;
    /* Context cursors. */
    SDL_Cursor *cursor_arrow;
    SDL_Cursor *cursor_crosshair;
    SDL_Cursor *cursor_hand;
    DWORD last_hover_entity;
} orbit;

static FLOAT CL_OrbitClamp(FLOAT value, FLOAT min_value, FLOAT max_value) {
    return MAX(min_value, MIN(value, max_value));
}

static void CL_OrbitInitInputState(void) {
    if (orbit.initialized) {
        return;
    }
    orbit.initialized = true;
    orbit.last_time = SDL_GetTicks();
    orbit.pitch = Cvar_Value("cl_camera_pitch", 0);
    if (cl.playerstate.distance <= 0.0f)
        cl.playerstate.distance = Cvar_Value("cl_camera_distance", 0);
    orbit.cursor_arrow = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW);
    orbit.cursor_crosshair = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_CROSSHAIR);
    orbit.cursor_hand = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_HAND);
    orbit.last_hover_entity = 0;
    /* Register tunable cvars with defaults. Config file values take
     * precedence because Cvar_Get only sets the default when the cvar
     * does not yet exist (config files are loaded before input init). */
    Cvar_Get("cl_camera_pitch", "0", 0);
    Cvar_Get("cl_camera_distance", "0", 0);
    Cvar_Get("cl_mouse_speed", "0.18", CVAR_ARCHIVE);
    Cvar_Get("cl_camera_min_pitch", "305.0", 0);
    Cvar_Get("cl_camera_max_pitch", "355.0", 0);
    Cvar_Get("camera_min_distance", "5.5", 0);
    Cvar_Get("camera_max_distance", "25.0", 0);
    Cvar_Get("cl_click_threshold", "10", 0);
}

static BOOL CL_OrbitMouseMovedPast(VECTOR2 const *a, VECTOR2 const *b) {
    FLOAT threshold = Cvar_Value("cl_click_threshold", 10.0f);
    FLOAT dx = a->x - b->x;
    FLOAT dy = a->y - b->y;
    return (dx * dx + dy * dy) > (threshold * threshold);
}

static void CL_OrbitSendAttack(DWORD entity_number) {
    MSG_WriteByte(&cls.netchan.message, clc_stringcmd);
    SZ_Printf(&cls.netchan.message, "attack %u", (unsigned)entity_number);
}

static void CL_OrbitSendInteract(DWORD entity_number) {
    MSG_WriteByte(&cls.netchan.message, clc_stringcmd);
    SZ_Printf(&cls.netchan.message, "interact %u", (unsigned)entity_number);
}

/* LMB up: if mouse didn't move much, treat as a click → select target under cursor. */
static void CL_OrbitLmbUp(void) {
    DWORD entnum;

    orbit.left_mouse = false;
    orbit.lmb_down = false;
    if (!CL_GameplayInputReady()) {
        return;
    }
    if (CL_OrbitMouseMovedPast(&mouse.origin, &orbit.lmb_down_pos)) {
        return; /* was a drag, not a click */
    }
    if (CL_MouseOverGameplayUI()) {
        return;
    }
    if (re.TraceEntity(&cl.viewDef, mouse.origin.x, mouse.origin.y, &entnum)) {
        CL_ApplySelection(&entnum, 1);
    } else {
        CL_ApplySelection(&entnum, 0); /* deselect */
    }
}

/* RMB up: if mouse didn't move much, treat as a click → context interact. */
static void CL_OrbitRmbUp(void) {
    DWORD entnum;

    orbit.right_mouse = false;
    orbit.rmb_dragging = false;
    SDL_SetRelativeMouseMode(SDL_FALSE);
    if (!CL_GameplayInputReady()) {
        return;
    }
    if (CL_OrbitMouseMovedPast(&mouse.origin, &orbit.rmb_down_pos)) {
        return; /* was a camera drag, not a click */
    }
    if (CL_MouseOverGameplayUI()) {
        return;
    }
    if (re.TraceEntity(&cl.viewDef, mouse.origin.x, mouse.origin.y, &entnum)) {
        CL_OrbitSendInteract(entnum);
    } else if (cl.selection.num_selected) {
        CL_OrbitSendInteract(cl.selection.entity_nums[0]);
    }
}

BOOL CL_OrbitSelectDown(void) {
    orbit.left_mouse = true;
    orbit.lmb_down = true;
    orbit.lmb_down_pos = mouse.origin;
    return true;
}

BOOL CL_OrbitSelectUp(void) {
    CL_OrbitLmbUp();
    return true;
}

/* +attack/-attack: click-to-attack; bind in config if needed. */
static void IN_AttackDown(void) {
    orbit.left_mouse = true;
    orbit.lmb_down = true;
    orbit.lmb_down_pos = mouse.origin;
}

static void IN_AttackUp(void) {
    DWORD entnum;
    orbit.left_mouse = false;
    orbit.lmb_down = false;
    if (!CL_GameplayInputReady())
        return;
    if (CL_OrbitMouseMovedPast(&mouse.origin, &orbit.lmb_down_pos))
        return; /* was a drag, not a click */
    if (CL_MouseOverGameplayUI())
        return;
    if (re.TraceEntity(&cl.viewDef, mouse.origin.x, mouse.origin.y, &entnum))
        CL_OrbitSendAttack(entnum);
}

static void IN_LookDown(void) {
    orbit.right_mouse = true;
    orbit.rmb_down_pos = mouse.origin;
    orbit.rmb_dragging = true;
    CL_OrbitInitInputState();
    SDL_SetRelativeMouseMode(SDL_TRUE);
}

static void IN_LookUp(void) {
    CL_OrbitRmbUp();
}

static void IN_ForwardDown(void) {
    orbit.move_forward = true;
}

static void IN_ForwardUp(void) {
    orbit.move_forward = false;
}

static void IN_BackDown(void) {
    orbit.move_back = true;
}

static void IN_BackUp(void) {
    orbit.move_back = false;
}

static void IN_MoveLeftDown(void) {
    orbit.move_left = true;
}

static void IN_MoveLeftUp(void) {
    orbit.move_left = false;
}

static void IN_MoveRightDown(void) {
    orbit.move_right = true;
}

static void IN_MoveRightUp(void) {
    orbit.move_right = false;
}

void CL_OrbitInit(void) {
    Cmd_AddCommand("+attack", IN_AttackDown);
    Cmd_AddCommand("-attack", IN_AttackUp);
    Cmd_AddCommand("+look", IN_LookDown);
    Cmd_AddCommand("-look", IN_LookUp);
    Cmd_AddCommand("+forward", IN_ForwardDown);
    Cmd_AddCommand("-forward", IN_ForwardUp);
    Cmd_AddCommand("+back", IN_BackDown);
    Cmd_AddCommand("-back", IN_BackUp);
    Cmd_AddCommand("+moveleft", IN_MoveLeftDown);
    Cmd_AddCommand("-moveleft", IN_MoveLeftUp);
    Cmd_AddCommand("+moveright", IN_MoveRightDown);
    Cmd_AddCommand("-moveright", IN_MoveRightUp);
}

void CL_OrbitMouseMotion(SDL_MouseMotionEvent const *motion) {
    DWORD entnum;

    if (!motion) {
        return;
    }
    /* Camera rotation while RMB held. */
    if (orbit.right_mouse) {
        FLOAT speed = Cvar_Value("cl_mouse_speed", 0.18f);
        FLOAT min_pitch = Cvar_Value("cl_camera_min_pitch", 300.0f);
        FLOAT max_pitch = Cvar_Value("cl_camera_max_pitch", 350.0f);

        orbit.yaw -= motion->xrel * speed;
        orbit.pitch = CL_OrbitClamp(orbit.pitch - motion->yrel * speed,
                                      min_pitch, max_pitch);
    }
    /* Hover detection: trace entity under cursor every motion. */
    if (!CL_GameplayInputReady() || CL_MouseOverGameplayUI()) {
        if (cl.hover_entity != 0) {
            cl.hover_entity = 0;
            SDL_SetCursor(orbit.cursor_arrow);
        }
        return;
    }
    if (re.TraceEntity(&cl.viewDef, (float)motion->x, (float)motion->y, &entnum)) {
        cl.hover_entity = entnum;
    } else {
        cl.hover_entity = 0;
    }
    /* Update context cursor based on hovered entity. */
    if (cl.hover_entity != orbit.last_hover_entity) {
        orbit.last_hover_entity = cl.hover_entity;
        if (cl.hover_entity == 0) {
            SDL_SetCursor(orbit.cursor_arrow);
        } else {
            BOOL hostile = false;
            FOR_LOOP(i, cl.viewDef.num_entities) {
                if (cl.viewDef.entities[i].number == cl.hover_entity) {
                    hostile = (cl.viewDef.entities[i].flags & RF_HOSTILE) != 0;
                    break;
                }
            }
            SDL_SetCursor(hostile ? orbit.cursor_crosshair : orbit.cursor_hand);
        }
    }
}

void CL_OrbitFrame(void) {
    DWORD now;
    FLOAT dt;
    DWORD flags = 0;

    CL_OrbitInitInputState();
    if (cls.key_dest != key_game || cls.state != ca_active) {
        return;
    }

    now = SDL_GetTicks();
    dt = (FLOAT)(now - orbit.last_time) / 1000.0f;
    if (dt < 0.0f || dt > 0.25f) {
        dt = 0.0f;
    }
    orbit.last_time = now;

    if (orbit.move_forward || (orbit.left_mouse && orbit.right_mouse)) {
        flags |= BZ_MOVE_FORWARD;
    }
    if (orbit.move_back) {
        flags |= BZ_MOVE_BACK;
    }
    if (orbit.move_left) {
        flags |= BZ_MOVE_LEFT;
    }
    if (orbit.move_right) {
        flags |= BZ_MOVE_RIGHT;
    }

    MSG_WriteByte(&cls.netchan.message, clc_stringcmd);
    SZ_Printf(&cls.netchan.message,
              "move %u %.3f %.3f %.3f",
              (unsigned)flags,
              (double)orbit.yaw,
              (double)orbit.pitch,
              (double)cl.playerstate.distance);
}


void CL_OrbitResetMap(void) {}
