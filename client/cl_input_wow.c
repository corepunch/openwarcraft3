#include "cl_input_local.h"

#ifdef WOW
#define WOW_MOVE_FORWARD 1
#define WOW_MOVE_BACK 2
#define WOW_MOVE_LEFT 4
#define WOW_MOVE_RIGHT 8
#define WOW_CAMERA_MIN_PITCH 300.0f
#define WOW_CAMERA_MAX_PITCH 350.0f
#define WOW_CAMERA_MIN_DISTANCE 3.0f
#define WOW_CAMERA_MAX_DISTANCE 35.0f
#define WOW_CAMERA_TURN_SPEED 135.0f
#define WOW_MOUSE_TURN_SPEED 0.18f

static struct {
    BOOL initialized;
    BOOL right_mouse;
    BOOL left_mouse;
    BOOL middle_mouse;
    BOOL drag_active;
    BOOL move_forward;
    BOOL move_back;
    BOOL move_left;
    BOOL move_right;
    DWORD last_time;
    FLOAT yaw;
    FLOAT pitch;
    FLOAT distance;
    VECTOR3 drag_anchor;
} wow_input = {
    .pitch = 328.0f,
    .distance = 8.5f,
};

static FLOAT CL_WowClamp(FLOAT value, FLOAT min_value, FLOAT max_value) {
    return MAX(min_value, MIN(value, max_value));
}

static void CL_WowInitInputState(void) {
    if (wow_input.initialized) {
        return;
    }
    wow_input.initialized = true;
    wow_input.last_time = SDL_GetTicks();
    wow_input.pitch = 328.0f;
    wow_input.distance = 8.5f;
}

static void CL_WowSetCameraPosition(VECTOR2 position) {
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

static void CL_WowBeginDrag(FLOAT x, FLOAT y) {
    if (!CL_GameplayInputReady()) {
        wow_input.drag_active = false;
        return;
    }
    Matrix4_getCameraMatrix(&cl.viewDef.viewProjectionMatrix);
    wow_input.drag_active = re.TraceLocation(&cl.viewDef, x, y, &wow_input.drag_anchor);
}

static void CL_WowUpdateDrag(FLOAT x, FLOAT y) {
    VECTOR3 point;
    VECTOR2 position;

    if (!CL_GameplayInputReady()) {
        wow_input.drag_active = false;
        return;
    }
    if (!wow_input.drag_active)
        return;
    Matrix4_getCameraMatrix(&cl.viewDef.viewProjectionMatrix);
    if (!re.TraceLocation(&cl.viewDef, x, y, &point))
        return;
    position.x = cl.viewDef.camerastate[0].origin.x + wow_input.drag_anchor.x - point.x;
    position.y = cl.viewDef.camerastate[0].origin.y + wow_input.drag_anchor.y - point.y;
    CL_WowSetCameraPosition(position);
    Matrix4_getCameraMatrix(&cl.viewDef.viewProjectionMatrix);
}

static void CL_WowEndDrag(void) {
    wow_input.middle_mouse = false;
    wow_input.drag_active = false;
}

static void IN_WowLeftDown(void) {
    wow_input.left_mouse = true;
}

static void IN_WowLeftUp(void) {
    wow_input.left_mouse = false;
}

static void CL_SendAttack(void) {
    if (!CL_GameplayInputReady()) {
        return;
    }
    if (wow_input.right_mouse) {
        return;
    }

    MSG_WriteByte(&cls.netchan.message, clc_stringcmd);
    SZ_Printf(&cls.netchan.message, "attack");
}

static void IN_AttackDown(void) {
    wow_input.left_mouse = true;
    CL_SendAttack();
}

static void IN_AttackUp(void) {
    wow_input.left_mouse = false;
}

static void IN_LookDown(void) {
    wow_input.right_mouse = true;
    CL_WowInitInputState();
    SDL_SetRelativeMouseMode(SDL_TRUE);
}

static void IN_LookUp(void) {
    wow_input.right_mouse = false;
    SDL_SetRelativeMouseMode(SDL_FALSE);
}

static void IN_ForwardDown(void) {
    wow_input.move_forward = true;
}

static void IN_ForwardUp(void) {
    wow_input.move_forward = false;
}

static void IN_BackDown(void) {
    wow_input.move_back = true;
}

static void IN_BackUp(void) {
    wow_input.move_back = false;
}

static void IN_MoveLeftDown(void) {
    wow_input.move_left = true;
}

static void IN_MoveLeftUp(void) {
    wow_input.move_left = false;
}

static void IN_MoveRightDown(void) {
    wow_input.move_right = true;
}

static void IN_MoveRightUp(void) {
    wow_input.move_right = false;
}

void CL_InputModeInit(void) {
    Cmd_AddCommand("+wowleft", IN_WowLeftDown);
    Cmd_AddCommand("-wowleft", IN_WowLeftUp);
    Cmd_AddCommand("+attack", IN_AttackDown);
    Cmd_AddCommand("-attack", IN_AttackUp);
    Cmd_AddCommand("+wowattack", IN_AttackDown);
    Cmd_AddCommand("-wowattack", IN_AttackUp);
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

void CL_InputModeSetGameplay(void) {
    cl.viewDef.camerastate[0].zfar = 16000;
    cl.viewDef.camerastate[0].znear = 1;
    cl.viewDef.camerastate[1].zfar = 16000;
    cl.viewDef.camerastate[1].znear = 1;
}

void CL_InputModeMouseButton(SDL_MouseButtonEvent const *button, BOOL down) {
    if (!button || button->button != SDL_BUTTON_MIDDLE) {
        return;
    }
    if (down) {
        wow_input.middle_mouse = true;
        CL_WowBeginDrag(button->x, button->y);
        return;
    }
    CL_WowEndDrag();
}

void CL_InputModeMouseMotion(SDL_MouseMotionEvent const *motion) {
    if (motion && wow_input.middle_mouse) {
        CL_WowUpdateDrag(motion->x, motion->y);
        return;
    }
    if (!wow_input.right_mouse || !motion) {
        return;
    }
    wow_input.yaw -= motion->xrel * WOW_MOUSE_TURN_SPEED;
    wow_input.pitch = CL_WowClamp(wow_input.pitch - motion->yrel * WOW_MOUSE_TURN_SPEED,
                                  WOW_CAMERA_MIN_PITCH,
                                  WOW_CAMERA_MAX_PITCH);
}

BOOL CL_InputModeMouseWheel(SDL_MouseWheelEvent const *wheel) {
    if (!wheel) {
        return false;
    }
    wow_input.distance = CL_WowClamp(wow_input.distance - wheel->y * 1.0f,
                                     WOW_CAMERA_MIN_DISTANCE,
                                     WOW_CAMERA_MAX_DISTANCE);
    return true;
}

void CL_InputModeFrame(void) {
    DWORD now;
    FLOAT dt;
    DWORD flags = 0;
    BOOL left;
    BOOL right;

    CL_WowInitInputState();
    if (cls.key_dest != key_game || cls.state != ca_active) {
        return;
    }

    now = SDL_GetTicks();
    dt = (FLOAT)(now - wow_input.last_time) / 1000.0f;
    if (dt < 0.0f || dt > 0.25f) {
        dt = 0.0f;
    }
    wow_input.last_time = now;

    if (wow_input.move_forward || (wow_input.left_mouse && wow_input.right_mouse)) {
        flags |= WOW_MOVE_FORWARD;
    }
    if (wow_input.move_back) {
        flags |= WOW_MOVE_BACK;
    }

    left = wow_input.move_left;
    right = wow_input.move_right;
    if (wow_input.right_mouse) {
        if (left) {
            flags |= WOW_MOVE_LEFT;
        }
        if (right) {
            flags |= WOW_MOVE_RIGHT;
        }
    } else {
        if (left) {
            wow_input.yaw += WOW_CAMERA_TURN_SPEED * dt;
        }
        if (right) {
            wow_input.yaw -= WOW_CAMERA_TURN_SPEED * dt;
        }
    }

    MSG_WriteByte(&cls.netchan.message, clc_stringcmd);
    SZ_Printf(&cls.netchan.message,
              "move %u %.3f %.3f %.3f",
              (unsigned)flags,
              (double)wow_input.yaw,
              (double)wow_input.pitch,
              (double)wow_input.distance);
}
#endif
