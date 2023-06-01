#include "ui_local.h"



bool UI_HandleMouseEvent(LPCVECTOR2 mouse, uiMouseEvent_t event, uiFrameDef_t *frame) {
    if (frame->children) {
        FOR_EACH_LIST(uiFrameDef_t, child, frame->children) {
            if (UI_HandleMouseEvent(mouse, event, child)) {
                return true;
            }
        }
        return false;
    } else if (frame->mouseHandler[event] && Rect_contains(&frame->rect, mouse)) {
        return frame->mouseHandler[event](frame);
        return true;
    } else {
        return false;
    }
}

void UI_MouseEvent(LPCVECTOR2 mouse, uiMouseEvent_t event) {
    UI_HandleMouseEvent(mouse, event, ui.simpleConsole);
}
