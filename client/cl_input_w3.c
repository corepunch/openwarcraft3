#include "cl_input_local.h"

#ifndef WOW
static struct {
    BOOL active;
    VECTOR3 anchor;
} camera_drag;

static BOOL smart_click_active;

static void CL_BeginPan(float x, float y) {
    camera_drag.active = re.TraceLocation(&cl.viewDef, x, y, &camera_drag.anchor);
}

static void CL_UpdatePan(float x, float y) {
    VECTOR3 point;
    VECTOR2 position;

    if (!camera_drag.active) {
        CL_BeginPan(x, y);
        return;
    }
    if (!re.TraceLocation(&cl.viewDef, x, y, &point)) {
        return;
    }

    position.x = cl.viewDef.camerastate[0].origin.x + camera_drag.anchor.x - point.x;
    position.y = cl.viewDef.camerastate[0].origin.y + camera_drag.anchor.y - point.y;
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

static void CL_EndPan(void) {
    camera_drag.active = false;
}

static void CL_SendSmartCommand(float x, float y) {
    DWORD entnum;
    VECTOR3 point;

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
    CL_BeginPan(mouse.origin.x, mouse.origin.y);
}

static void IN_PanUp(void) {
    CL_EndPan();
}

static void IN_SmartDown(void) {
    smart_click_active = true;
}

static void IN_SmartUp(void) {
    if (!smart_click_active) {
        return;
    }
    smart_click_active = false;
    CL_SendSmartCommand(mouse.origin.x, mouse.origin.y);
}

void CL_InputModeInit(void) {
    Cmd_AddCommand("+pan", IN_PanDown);
    Cmd_AddCommand("-pan", IN_PanUp);
    Cmd_AddCommand("+smart", IN_SmartDown);
    Cmd_AddCommand("-smart", IN_SmartUp);
}

void CL_InputModeSetGameplay(void) {
    cl.moveConfirmation = re.LoadModel("UI\\Feedback\\Confirmation\\Confirmation.mdx");
    cl.viewDef.camerastate[0].zfar = 5000;
    cl.viewDef.camerastate[0].znear = 100;
    cl.viewDef.camerastate[1].zfar = 5000;
    cl.viewDef.camerastate[1].znear = 100;
}

void CL_InputModeMouseMotion(SDL_MouseMotionEvent const *motion) {
    if (!motion) {
        return;
    }
    if (camera_drag.active) {
        CL_UpdatePan(motion->x, motion->y);
    }
    if (cl.selection.in_progress) {
        cl.selection.rect.w = motion->x - cl.selection.rect.x;
        cl.selection.rect.h = motion->y - cl.selection.rect.y;
    }
}

BOOL CL_InputModeMouseWheel(SDL_MouseWheelEvent const *wheel) {
    (void)wheel;
    return false;
}

void CL_InputModeFrame(void) {
}
#endif
