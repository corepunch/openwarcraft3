#include <string.h>

#include "../client/client.h"

struct client_state cl;
struct client_static cls;
refExport_t re;
uiExport_t ui;
mouseEvent_t mouse;

static size2_t mock_GetWindowSize(void) {
    return MAKE(size2_t, 1024, 768);
}

static void mock_DrawLoadingIndicator(LPCRECT rect, DWORD time, COLOR32 color) {
    (void)rect;
    (void)time;
    (void)color;
}

void V_RenderView(void) {
}

void CON_DrawConsole(void) {
}

int Cvar_Integer(LPCSTR name, int fallback) {
    (void)name;
    return fallback;
}

void CL_ParseTEnt(LPSIZEBUF msg) {
    (void)msg;
}

void CL_ParseUnitUI(LPSIZEBUF msg) {
    (void)msg;
}

void test_client_stubs_init(void) {
    memset(&cl, 0, sizeof(cl));
    memset(&cls, 0, sizeof(cls));
    memset(&re, 0, sizeof(re));
    memset(&ui, 0, sizeof(ui));
    memset(&mouse, 0, sizeof(mouse));
    re.GetWindowSize = mock_GetWindowSize;
    re.DrawLoadingIndicator = mock_DrawLoadingIndicator;
}
