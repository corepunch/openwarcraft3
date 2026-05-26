#include "client.h"

#include <SDL2/SDL.h>

//struct {
//    LPCSTR command;
//    DWORD key;
//} hotkey_t;
//
//hotkey_t hotkeys = {
//    { "show_quests",  }
//};

mouseEvent_t mouse;

static struct {
    BOOL active;
    VECTOR3 anchor;
} camera_drag;

static BOOL camera_drag_left_alt;

static BOOL CL_AltModifierDown(void) {
    SDL_Keymod const mod = SDL_GetModState();
    return (mod & (KMOD_LALT | KMOD_RALT)) != 0;
}

static BOOL CL_BypassBoundMouseCommand(SDL_Event const *event) {
    if (cls.key_dest != key_game || event->button.button != SDL_BUTTON_LEFT) {
        return false;
    }
    if (event->type == SDL_MOUSEBUTTONDOWN) {
        return CL_AltModifierDown();
    }
    if (event->type == SDL_MOUSEBUTTONUP) {
        return camera_drag_left_alt;
    }
    return false;
}

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
    DWORD last_time;
    FLOAT yaw;
    FLOAT pitch;
    FLOAT distance;
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

static void CL_WowMouseMotion(SDL_MouseMotionEvent const *motion) {
    if (!wow_input.right_mouse || !motion) {
        return;
    }
    wow_input.yaw -= motion->xrel * WOW_MOUSE_TURN_SPEED;
    wow_input.pitch = CL_WowClamp(wow_input.pitch - motion->yrel * WOW_MOUSE_TURN_SPEED,
                                  WOW_CAMERA_MIN_PITCH,
                                  WOW_CAMERA_MAX_PITCH);
}

static void CL_WowMouseWheel(SDL_MouseWheelEvent const *wheel) {
    if (!wheel) {
        return;
    }
    wow_input.distance = CL_WowClamp(wow_input.distance - wheel->y * 1.0f,
                                     WOW_CAMERA_MIN_DISTANCE,
                                     WOW_CAMERA_MAX_DISTANCE);
}

static void CL_WowSendMoveCommand(void) {
    Uint8 const *keys;
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

    keys = SDL_GetKeyboardState(NULL);
    if (keys[SDL_SCANCODE_W] || (wow_input.left_mouse && wow_input.right_mouse)) {
        flags |= WOW_MOVE_FORWARD;
    }
    if (keys[SDL_SCANCODE_S]) {
        flags |= WOW_MOVE_BACK;
    }

    left = keys[SDL_SCANCODE_A] != 0;
    right = keys[SDL_SCANCODE_D] != 0;
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
              "wowmove %u %.3f %.3f %.3f",
              (unsigned)flags,
              (double)wow_input.yaw,
              (double)wow_input.pitch,
              (double)wow_input.distance);
}
#endif

static void CL_BeginCameraDrag(float x, float y) {
    camera_drag.active = re.TraceLocation(&cl.viewDef, x, y, &camera_drag.anchor);
}

static void CL_UpdateCameraDrag(float x, float y) {
    VECTOR3 point;
    VECTOR2 position;

    if (!camera_drag.active) {
        CL_BeginCameraDrag(x, y);
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

static void CL_EndCameraDrag(void) {
    camera_drag.active = false;
}

static BOOL CL_MouseOverGameplayUI(void) {
    VECTOR2 const m = SCR_MouseToFdf();

    FOR_LOOP(layer_id, MAX_LAYOUT_LAYERS) {
        if (cl.layout[layer_id] == NULL) {
            continue;
        }
        LPCUIFRAME frames = SCR_Clear(cl.layout[layer_id]);
        FOR_LOOP(object_id, MAX_LAYOUT_OBJECTS) {
            LPCUIFRAME frame = frames+object_id;
            if (frame->flags.type != FT_TEXTURE) {
                continue;
            }
            RECT const screen = *SCR_LayoutRect(frame);
            if (Rect_contains(&screen, &m)) {
                return true;
            }
        }
    }
    return false;
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

void CL_Input(void) {
    SDL_Event event;
    BOOL camera_drag_pending = false;
    VECTOR2 camera_drag_mouse = { 0 };

    mouse.event = UI_EVENT_NONE;
    mouse.wheel = 0;
    while(SDL_PollEvent(&event)) {
        switch(event.type) {
            case SDL_MOUSEBUTTONDOWN:
                {
                    DWORD mousevt = K_MOUSE1 + event.button.button - 1;
                mouse.origin.x = event.button.x;
                mouse.origin.y = event.button.y;
                if (!CL_BypassBoundMouseCommand(&event)) {
                    Key_Event(mousevt, true, event.button.timestamp);
                }
                }
                break;
            case SDL_MOUSEBUTTONUP:
                {
                    DWORD mousevt = K_MOUSE1 + event.button.button - 1;
                mouse.origin.x = event.button.x;
                mouse.origin.y = event.button.y;
                if (!CL_BypassBoundMouseCommand(&event)) {
                    Key_Event(mousevt, false, event.button.timestamp);
                }
                }
                break;
            case SDL_MOUSEMOTION:
                mouse.origin.x = event.motion.x;
                mouse.origin.y = event.motion.y;
                break;
            case SDL_MOUSEWHEEL:
                {
                    int x;
                    int y;

                    SDL_GetMouseState(&x, &y);
                    mouse.origin.x = x;
                    mouse.origin.y = y;
                    mouse.wheel += event.wheel.y;
                }
                break;
        }
        
        switch(event.type) {
            case SDL_TEXTINPUT:
                if (cls.key_dest == key_menu) {
                    if (ui.TextInput) {
                        ui.TextInput(event.text.text);
                    }
                    SCR_TextInput(event.text.text);
                }
                break;
            case SDL_KEYDOWN:
                if (cls.key_dest == key_menu && SCR_EditKey(event.key.keysym.sym)) {
                    break;
                }
                Key_Event(event.key.keysym.sym, true, event.key.timestamp);
                break;
            case SDL_KEYUP:
                Key_Event(event.key.keysym.sym, false, event.key.timestamp);
//                if(event.key.keysym.sym == SDLK_ESCAPE) {
//                    return Com_Quit();
//                }
                break;
            case SDL_MOUSEBUTTONDOWN:
                mouse.origin.x = event.button.x;
                mouse.origin.y = event.button.y;
                mouse.button = event.button.button;
                if (cls.key_dest == key_menu) {
                    if (ui.MouseEvent) {
                        ui.MouseEvent(event.button.x, event.button.y, event.button.button, true);
                    }
                    if (event.button.button == SDL_BUTTON_LEFT) {
                        mouse.event = UI_LEFT_MOUSE_DOWN;
                    } else if (event.button.button == SDL_BUTTON_RIGHT) {
                        mouse.event = UI_RIGHT_MOUSE_DOWN;
                    }
                    break;
                }
                switch (event.button.button) {
                    case SDL_BUTTON_LEFT:
                        mouse.event = UI_LEFT_MOUSE_DOWN;
                        if (CL_AltModifierDown()) {
                            camera_drag_left_alt = true;
#ifndef WOW
                            CL_BeginCameraDrag(event.button.x, event.button.y);
#endif
                        }
#ifdef WOW
                        wow_input.left_mouse = true;
#endif
                        break;
                    case SDL_BUTTON_RIGHT:
                        mouse.event = UI_RIGHT_MOUSE_DOWN;
#ifdef WOW
                        wow_input.right_mouse = true;
                        CL_WowInitInputState();
                        SDL_SetRelativeMouseMode(SDL_TRUE);
#endif
                        break;
                    case SDL_BUTTON_MIDDLE:
#ifndef WOW
                        CL_BeginCameraDrag(event.button.x, event.button.y);
#endif
                        break;
                }
                break;
            case SDL_MOUSEBUTTONUP:
                mouse.origin.x = event.button.x;
                mouse.origin.y = event.button.y;
                mouse.button = 0;
                if (cls.key_dest == key_menu) {
                    if (ui.MouseEvent) {
                        ui.MouseEvent(event.button.x, event.button.y, event.button.button, false);
                    }
                    if (event.button.button == SDL_BUTTON_LEFT) {
                        mouse.event = UI_LEFT_MOUSE_UP;
                    } else if (event.button.button == SDL_BUTTON_RIGHT) {
                        mouse.event = UI_RIGHT_MOUSE_UP;
                    }
                    break;
                }
                switch (event.button.button) {
                    case SDL_BUTTON_LEFT:
                        mouse.event = UI_LEFT_MOUSE_UP;
                        if (camera_drag_left_alt) {
                            camera_drag_left_alt = false;
#ifndef WOW
                            CL_EndCameraDrag();
#endif
                            break;
                        }
#ifdef WOW
                        wow_input.left_mouse = false;
#endif
                        break;
                    case SDL_BUTTON_RIGHT:
                        mouse.event = UI_RIGHT_MOUSE_UP;
#ifdef WOW
                        wow_input.right_mouse = false;
                        SDL_SetRelativeMouseMode(SDL_FALSE);
#else
                        CL_SendSmartCommand(event.button.x, event.button.y);
#endif
                        break;
                    case SDL_BUTTON_MIDDLE:
#ifndef WOW
                        CL_EndCameraDrag();
#endif
                        break;
                }
                break;
            case SDL_MOUSEMOTION:
                mouse.origin.x = event.motion.x;
                mouse.origin.y = event.motion.y;
                if (cls.key_dest == key_menu) {
                    if (ui.MouseEvent) {
                        ui.MouseEvent(event.motion.x, event.motion.y, 0, false);
                    }
                    break;
                }
#ifdef WOW
                CL_WowMouseMotion(&event.motion);
#endif
                switch (mouse.button) {
                    case SDL_BUTTON_LEFT:
#ifndef WOW
                        if (camera_drag_left_alt) {
                            camera_drag_mouse.x = event.motion.x;
                            camera_drag_mouse.y = event.motion.y;
                            camera_drag_pending = true;
                        } else {
                            cl.selection.rect.w = event.motion.x - cl.selection.rect.x;
                            cl.selection.rect.h = event.motion.y - cl.selection.rect.y;
                        }
#endif
                        break;
                    case SDL_BUTTON_MIDDLE:
#ifndef WOW
                        camera_drag_mouse.x = event.motion.x;
                        camera_drag_mouse.y = event.motion.y;
                        camera_drag_pending = true;
#endif
                        break;
                }
                break;
            case SDL_MOUSEWHEEL:
#ifdef WOW
                if (cls.key_dest == key_game) {
                    CL_WowMouseWheel(&event.wheel);
                    break;
                }
#endif
                if (cls.key_dest == key_menu && ui.MouseEvent) {
                    int x;
                    int y;
                    int steps = event.wheel.y;

                    SDL_GetMouseState(&x, &y);
                    if (steps < 0) {
                        steps = -steps;
                    }
                    for (int i = 0; i < steps; i++) {
                        ui.MouseEvent(x, y, event.wheel.y > 0 ? 4 : 5, true);
                    }
                }
                break;
            case SDL_WINDOWEVENT:
                switch (event.window.event) {
                    case SDL_WINDOWEVENT_CLOSE:   // exit game
                        return Com_Quit();
                    default:
                        break;
                }
                break;
        }
    }
    if (camera_drag_pending) {
        CL_UpdateCameraDrag(camera_drag_mouse.x, camera_drag_mouse.y);
    }
#ifdef WOW
    CL_WowSendMoveCommand();
#endif
//    cl.viewDef.camera.origin.z = CM_GetHeightAtPoint(cl.viewDef.camera.origin.x, cl.viewDef.camera.origin.y);
}

void CL_SetMenuBindings(void) {
    cls.key_dest = key_menu;
    SDL_StartTextInput();
    Key_SetBinding(K_ESCAPE, "quit");
}

void CL_SetGameplayBindings(void) {
    cls.key_dest = key_game;
    cls.netchan.remote_address.type = NA_LOOPBACK;
    SDL_StopTextInput();
#ifndef WOW
    cl.moveConfirmation = re.LoadModel("UI\\Feedback\\Confirmation\\Confirmation.mdx");
#endif

#ifdef WOW
    cl.viewDef.camerastate[0].zfar = 16000;
    cl.viewDef.camerastate[0].znear = 1;
    cl.viewDef.camerastate[1].zfar = 16000;
    cl.viewDef.camerastate[1].znear = 1;
#else
    cl.viewDef.camerastate[0].zfar = 5000;
    cl.viewDef.camerastate[0].znear = 100;
    cl.viewDef.camerastate[1].zfar = 5000;
    cl.viewDef.camerastate[1].znear = 100;
#endif

#ifndef WOW
    Key_SetBinding(K_MOUSE1, "+select");
#endif
    Key_SetBinding('q', "cmd quests");
    Key_SetBinding(K_ESCAPE, "cmd cancel");
}

void IN_SelectDown(void) {
    cl.selection.in_progress = true;
    cl.selection.rect.x = mouse.origin.x;
    cl.selection.rect.y = mouse.origin.y;
    cl.selection.rect.w = 0;
    cl.selection.rect.h = 0;

    if (CL_MouseOverGameplayUI()) {
        cl.selection.in_progress = false;
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
            
            /* Store selected entity and request UI data (Phase 8.6) */
            cl.selection.num_selected = 1;
            cl.selection.entity_nums[0] = entnum;
            CL_RequestUnitUI(1, cl.selection.entity_nums);
        }
        if (re.TraceLocation(&cl.viewDef, r.x, r.y, &point)){
            MSG_WriteByte(&cls.netchan.message, clc_stringcmd);
            SZ_Printf(&cls.netchan.message, "point %d %d", (int)point.x, (int)point.y);
            if (cl.selection.num_selected) {
                CL_RequestUnitUI(cl.selection.num_selected, cl.selection.entity_nums);
            }
        }
    } else {
        DWORD selected[MAX_SELECTED_ENTITIES] = { 0 };
        DWORD num = re.EntitiesInRect(&cl.viewDef, &cl.selection.rect, MAX_SELECTED_ENTITIES, selected);
        char buffer[1024] = { 0 };
        if (num == 0)
            return;
        if (num > MAX_SELECTED_ENTITIES) {
            num = MAX_SELECTED_ENTITIES;
        }
        strcpy(buffer, "select");
        FOR_LOOP(i, num) {
            size_t used = strlen(buffer);
            snprintf(buffer + used, sizeof(buffer) - used, " %d", selected[i]);
        }
        MSG_WriteByte(&cls.netchan.message, clc_stringcmd);
        SZ_Printf(&cls.netchan.message, buffer);
        
        /* Store selected entities and request UI data (Phase 8.6) */
        cl.selection.num_selected = num;
        memcpy(cl.selection.entity_nums, selected, sizeof(DWORD) * num);
        CL_RequestUnitUI(num, cl.selection.entity_nums);
    }
}

void CL_ForwardToServer_f(void) {
    extern LPCSTR current_command;
    MSG_WriteByte(&cls.netchan.message, clc_stringcmd);
    SZ_Printf(&cls.netchan.message, current_command+4);
}

void CL_InitInput(void) {
    fprintf(stderr, "Input initialization.\n");
    fprintf(stderr, "%d joysticks were found.\n", SDL_NumJoysticks());
    fprintf(stderr, "Input initialized.\n\n");

    Cmd_AddCommand("+select", IN_SelectDown);
    Cmd_AddCommand("-select", IN_SelectUp);
    Cmd_AddCommand("cmd", CL_ForwardToServer_f);
}
