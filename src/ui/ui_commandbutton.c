#include "ui_local.h"

static void CommandButton_Draw(uiFrameDef_t const *frame) {
    RECT const uv = { 0, 0, 1, 1 };
    uiCommandButton_t *cb = frame->typedata;
    if (!cb->Texture)
        return;
    imp.DrawImage(cb->Texture, &frame->rect, &uv);
}

static bool CommandButton_LeftMouseDown(uiFrameDef_t *frame) {
    frame->Width = COMMAND_BUTTON_SIZE * 0.95;
    frame->Height = COMMAND_BUTTON_SIZE * 0.95;
    return true;
}

static bool CommandButton_LeftMouseUp(uiFrameDef_t *frame) {
    frame->Width = COMMAND_BUTTON_SIZE;
    frame->Height = COMMAND_BUTTON_SIZE;
    uiCommandButton_t *cb = frame->typedata;
    if (cb->click) {
        cb->click(cb);
    }
    return true;
}

void CommandButton_Cancel(uiCommandButton_t const *cmd) {
    CommandBar_SetMode(CBAR_SHOW_ABILITIES);
}

void CommandButton_SelectTarget(uiCommandButton_t const *cmd) {
    CommandBar_SetMode(CBAR_SELECT_TARGET);
}

void CommandButton_SelectBuildLocation(uiCommandButton_t const *cmd) {
    PATHSTR buffer = { 0 };
    sprintf(buffer, "buildloc %s", cmd->Command);
    imp.ClienCommand(buffer);
    CommandBar_SetMode(CBAR_SELECT_BUILD_LOCATION);
}

void CommandButton_Build(uiCommandButton_t const *cmd) {
    CommandBar_SetMode(CBAR_SHOW_BUILDS);
}

void CommandButton_Stop(uiCommandButton_t const *cmd) {

}

uiFrameDef_t *UI_MakeCommandButton(void) {
    uiFrameDef_t *frame = imp.MemAlloc(sizeof(uiFrameDef_t));
    frame->typedata = imp.MemAlloc(sizeof(uiCommandButton_t));
    frame->draw = CommandButton_Draw;
    frame->mouseHandler[UI_LEFT_MOUSE_DOWN] = CommandButton_LeftMouseDown;
    frame->mouseHandler[UI_LEFT_MOUSE_UP] = CommandButton_LeftMouseUp;
    return frame;
}
