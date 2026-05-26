#include "client.h"

static void SCR_DrawFPS(DWORD msec) {
    static DWORD elapsed = 0;
    static DWORD frames_drawn = 0;
    static DWORD fps = 0;
    char text[32];
    size2_t window = re.GetWindowSize();
    COLOR32 color = {255,255,255,180};
    DWORD y = window.height > 24 ? window.height - 24 : 0;

    elapsed += msec;
    frames_drawn++;
    if (elapsed >= 500) {
        fps = frames_drawn * 1000 / elapsed;
        elapsed = 0;
        frames_drawn = 0;
    } else if (!fps && msec > 0) {
        fps = 1000 / msec;
    }

    if (fps) {
        snprintf(text, sizeof(text), "FPS: %u", (unsigned)fps);
    } else {
        snprintf(text, sizeof(text), "FPS: --");
    }
    re.PrintSysText(text, 10, y, color);
}

void SCR_UpdateScreen(DWORD msec) {
    re.BeginFrame();
    
    V_RenderView();

    if (ui.DrawFrame) {
        ui.DrawFrame();
    }

    CON_DrawConsole();
    SCR_DrawFPS(msec);
    
//    if (cl.pics[42]) re.DrawPic(cl.pics[42], 0, 0);
    
    re.EndFrame();
}
