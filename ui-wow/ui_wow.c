#include "../ui/ui.h"

static uiImport_t uiimport;

static void UIWow_Init(void) {
}

static void UIWow_Shutdown(void) {
}

static void UIWow_Refresh(DWORD msec) {
    (void)msec;
}

static void UIWow_DrawFrame(void) {
}

static void UIWow_KeyEvent(int key, BOOL down, DWORD time) {
    (void)key;
    (void)down;
    (void)time;
}

static void UIWow_TextInput(LPCSTR text) {
    (void)text;
}

static void UIWow_MouseEvent(int x, int y, int button, BOOL down) {
    (void)x;
    (void)y;
    (void)button;
    (void)down;
}

static void UIWow_MenuCommand(LPCSTR route) {
    (void)route;
}

static void UIWow_UpdateUnitUI(DWORD num_units, uiUnitData_t *units) {
    (void)num_units;
    (void)units;
}

uiExport_t UI_GetAPI(uiImport_t import) {
    uiimport = import;
    (void)uiimport;

    return (uiExport_t) {
        .Init = UIWow_Init,
        .Shutdown = UIWow_Shutdown,
        .Refresh = UIWow_Refresh,
        .DrawFrame = UIWow_DrawFrame,
        .KeyEvent = UIWow_KeyEvent,
        .TextInput = UIWow_TextInput,
        .MouseEvent = UIWow_MouseEvent,
        .MenuCommand = UIWow_MenuCommand,
        .UpdateUnitUI = UIWow_UpdateUnitUI,
    };
}
