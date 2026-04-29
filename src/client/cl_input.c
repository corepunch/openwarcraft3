#include "client.h"

mouseEvent_t mouse;

static int current_button = 0;

static void pan_camera(float x, float y, float sensivity) {
    cl.viewDef.camerastate->origin.x += x * sensivity;
    cl.viewDef.camerastate->origin.y += y * sensivity;
    MSG_WriteByte(&cls.netchan.message, clc_move);
    MSG_WriteShort(&cls.netchan.message, (short)(x * sensivity));
    MSG_WriteShort(&cls.netchan.message, (short)(y * sensivity));
}

// Called by the orion-ui game window proc on mouse-button-down events.
void CL_MouseButtonDown(int button, float x, float y, unsigned int time) {
    mouse.origin.x = x;
    mouse.origin.y = y;
    mouse.button = (DWORD)button;
    current_button = button;
    unsigned char key = (button == 1) ? K_MOUSE1 : (button == 3 ? K_MOUSE2 : K_MOUSE3);
    switch (button) {
        case 1: mouse.event = UI_LEFT_MOUSE_DOWN;  break;
        case 3: mouse.event = UI_RIGHT_MOUSE_DOWN; break;
        default: break;
    }
    Key_Event(key, true, time);
}

// Called by the orion-ui game window proc on mouse-button-up events.
void CL_MouseButtonUp(int button, float x, float y, unsigned int time) {
    mouse.origin.x = x;
    mouse.origin.y = y;
    mouse.button = 0;
    current_button = 0;
    unsigned char key = (button == 1) ? K_MOUSE1 : (button == 3 ? K_MOUSE2 : K_MOUSE3);
    switch (button) {
        case 1: mouse.event = UI_LEFT_MOUSE_UP;  break;
        case 3: mouse.event = UI_RIGHT_MOUSE_UP; break;
        default: break;
    }
    Key_Event(key, false, time);
}

// Called by the orion-ui game window proc on mouse-move events.
void CL_MouseMove(float x, float y, float dx, float dy) {
    mouse.origin.x = x;
    mouse.origin.y = y;
    switch (current_button) {
        case 1:
            cl.selection.rect.w = x - cl.selection.rect.x;
            cl.selection.rect.h = y - cl.selection.rect.y;
            break;
        case 3:
            pan_camera(-dx, dy, 5);
            break;
        default:
            break;
    }
}

// Called by the orion-ui game window proc on key-down events.
void CL_KeyDown(unsigned char key, unsigned int time) {
    Key_Event(key, true, time);
}

// Called by the orion-ui game window proc on key-up events.
void CL_KeyUp(unsigned char key, unsigned int time) {
    Key_Event(key, false, time);
}

void CL_Input(void) {
    // Input is now driven by orion-ui window messages delivered to the game
    // window proc (ui_main.c).  This function only resets the per-frame
    // mouse-event flag so that game systems see a clean state each tick.
    mouse.event = UI_EVENT_NONE;
}

void IN_SelectDown(void) {
    cl.selection.in_progress = true;
    cl.selection.rect.x = mouse.origin.x;
    cl.selection.rect.y = mouse.origin.y;
    cl.selection.rect.w = 0;
    cl.selection.rect.h = 0;

    size2_t const win = re.GetWindowSize();
    VECTOR2 m = {
        mouse.origin.x * 0.8f / (float)win.width,
        mouse.origin.y * 0.6f / (float)win.height
    };
    
    FOR_LOOP(layer_id, MAX_LAYOUT_LAYERS) {
        if (cl.layout[layer_id] == NULL)
            continue;
        LPCUIFRAME frames = SCR_Clear(cl.layout[layer_id]);
        FOR_LOOP(object_id, MAX_LAYOUT_OBJECTS) {
            LPCUIFRAME frame = frames+object_id;
            // Block game selection for opaque/interactive HUD elements only.
            // FT_SCREEN is the root canvas frame (covers entire screen) and
            // must not block. FT_HIGHLIGHT/FT_SIMPLESTATUSBAR are game-world
            // overlays (selection rings, HP bars) — do not block.
            switch (frame->flags.type) {
                case FT_NONE:
                case FT_SCREEN:
                case FT_HIGHLIGHT:
                case FT_SIMPLESTATUSBAR:
                case FT_TOOLTIPTEXT:
                    continue;
                default:
                    break;
            }
            RECT const screen = *SCR_LayoutRect(frame);
            if (Rect_contains(&screen, &m)) {
                cl.selection.in_progress = false;
            }
        }
    }
}

void IN_SelectUp(void) {
    if (!cl.selection.in_progress)
        return;
    RECT const r = cl.selection.rect;
    cl.selection.in_progress = false;
    DWORD entnum;
    VECTOR3 point;
    if (fabs(r.w)+fabs(r.h) < 10) {
        if (re.TraceEntity(&cl.viewDef, r.x, r.y, &entnum)) {
            MSG_WriteByte(&cls.netchan.message, clc_stringcmd);
            SZ_Printf(&cls.netchan.message, "select %d", entnum);
        }
        if (re.TraceLocation(&cl.viewDef, r.x, r.y, &point)){
            MSG_WriteByte(&cls.netchan.message, clc_stringcmd);
            SZ_Printf(&cls.netchan.message, "point %d %d", (int)point.x, (int)point.y);
        }
    } else {
        DWORD selected[MAX_SELECTED_ENTITIES] = { 0 };
        DWORD num = re.EntitiesInRect(&cl.viewDef, &cl.selection.rect, MAX_SELECTED_ENTITIES, selected);
        char buffer[1024] = { 0 };
        if (num == 0)
            return;
        strcpy(buffer, "select");
        FOR_LOOP(i, num) {
            sprintf(buffer+strlen(buffer), " %d", selected[i]);
        }
        MSG_WriteByte(&cls.netchan.message, clc_stringcmd);
        SZ_Printf(&cls.netchan.message, buffer);
    }
}

void CL_ForwardToServer_f(void) {
    extern LPCSTR current_command;
    MSG_WriteByte(&cls.netchan.message, clc_stringcmd);
    SZ_Printf(&cls.netchan.message, current_command+4);
}

void CL_InitInput(void) {
    Cmd_AddCommand("+select", IN_SelectDown);
    Cmd_AddCommand("-select", IN_SelectUp);
    Cmd_AddCommand("cmd", CL_ForwardToServer_f);
}
