#include <string.h>

#include "../client/client.h"

struct client_state cl;
struct client_static cls;
refExport_t re;
mouseEvent_t mouse;

static size2_t mock_GetWindowSize(void) {
    return MAKE(size2_t, 1024, 768);
}

void V_RenderView(void) {
}

void CON_DrawConsole(void) {
}

void CL_ParseTEnt(LPSIZEBUF msg) {
    (void)msg;
}

void test_client_stubs_init(void) {
    memset(&cl, 0, sizeof(cl));
    memset(&cls, 0, sizeof(cls));
    memset(&re, 0, sizeof(re));
    memset(&mouse, 0, sizeof(mouse));
    re.GetWindowSize = mock_GetWindowSize;
}
