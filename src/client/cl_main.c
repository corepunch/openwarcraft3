#include "client.h"
#include "renderer.h"

#include <SDL.h>

struct Renderer *renderer = NULL;
struct client_state cl = {
    .sock = 0
};

static BYTE net_message_buffer[MAX_MSGLEN];
static struct sizebuf net_message = {
    .data = net_message_buffer,
    .maxsize = MAX_MSGLEN,
    .cursize = 0,
    .readcount = 0,
};

void CL_Init(void) {
    renderer = Renderer_Init(&(struct renderer_import) {
        .MemAlloc = MemAlloc,
        .MemFree = MemFree,
        .FileOpen = FS_OpenFile,
        .FileClose = SFileCloseFile,
        .FileExtract = FS_ExtractFile,
    });
    
    renderer->Init(WINDOW_WIDTH, WINDOW_HEIGHT);
    
    memset(&cl, 0, sizeof(struct client_state));
    
    cl.viewDef.fov = 90;
    cl.viewDef.vieworg = (VECTOR3) { -600, 1500, -1000 };
    cl.viewDef.viewangles = (VECTOR3) { -20, 0, 0 };
//    cl.viewDef.fov = 70;
//    cl.viewDef.vieworg = (VECTOR3) { -700, 2500, -400 };
//    cl.viewDef.viewangles = (VECTOR3) { -90, 0, 0 };
}

void CL_Input(void) {
    static int button;
    SDL_Event event;
    while(SDL_PollEvent(&event))
    {
        switch(event.type)
        {
            case SDL_KEYUP:
                if(event.key.keysym.sym == SDLK_ESCAPE)
                    return Com_Quit();
                break;
            case SDL_MOUSEBUTTONDOWN:
                button = event.button.button;
                break;
            case SDL_MOUSEBUTTONUP:
                button = 0;
                break;
            case SDL_MOUSEMOTION:
                if (button == 1) {
                    cl.viewDef.vieworg.x += event.motion.xrel * 5;
                    cl.viewDef.vieworg.y -= event.motion.yrel * 5;
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
}

void CL_ReadPackets(void) {
    while (NET_GetPacket(cl.sock, &net_message)) {
        CL_ParseServerMessage(&net_message);
    }
}

void CL_Shutdown(void) {
    FOR_LOOP(dwModelIndex, MAX_MODELS) {
        if (!cl.models[dwModelIndex])
            continue;
        renderer->ReleaseModel(cl.models[dwModelIndex]);
    }

    renderer->Shutdown();
}

void CL_Frame(DWORD msec) {
    CL_ReadPackets();
    CL_Input();
    CL_PrepRefresh();
    V_RenderView();
}
