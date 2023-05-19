#include "client.h"
#include "renderer.h"

#include <SDL.h>
#include <StormLib.h>

struct Renderer *renderer = NULL;
struct client_static cls;
struct client_state cl;

void CL_Init(void) {
    renderer = Renderer_Init(&(struct renderer_import) {
        .MemAlloc = MemAlloc,
        .MemFree = MemFree,
        .FileOpen = FS_OpenFile,
        .FileClose = SFileCloseFile,
        .FileExtract = FS_ExtractFile,
        .FindTerrainInfo = FindTerrainInfo,
        .FindCliffInfo = FindCliffInfo,
    });
    
    renderer->Init(WINDOW_WIDTH, WINDOW_HEIGHT);
    
    memset(&cl, 0, sizeof(struct client_state));
    
    cl.viewDef.fov = 45;
    cl.viewDef.vieworg = (VECTOR3) { -600, 2500, -2000 };
    cl.viewDef.viewangles = (VECTOR3) { -30, 0, 0 };
    cl.team = 1;
    
    void __netchan_init(struct netchan *netchan);
    __netchan_init(&cls.netchan);
}

void CL_Input(void) {
    static int button;
    static int moved = false;
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
                moved = false;
                break;
            case SDL_MOUSEBUTTONUP:
                button = 0;
                if (moved == false) {
                    CL_SelectEntityAtScreenPoint(event.button.x, event.button.y);
                }
                break;
            case SDL_MOUSEMOTION:
                if (button == 1) {
                    moved = true;
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
    static BYTE net_message_buffer[MAX_MSGLEN];
    static struct sizebuf net_message = {
        .data = net_message_buffer,
        .maxsize = MAX_MSGLEN,
        .cursize = 0,
        .readcount = 0,
    };
    while (NET_GetPacket(NS_CLIENT, cl.sock, &net_message)) {
        CL_ParseServerMessage(&net_message);
    }
}

void CL_SendCommands(void) {
    extern struct client_message msg;
    if (msg.cmd != CMD_NO_COMMAND){
        MSG_WriteByte(&cls.netchan.message, clc_move);
        MSG_WriteShort(&cls.netchan.message, msg.entity);
        MSG_WriteShort(&cls.netchan.message, msg.location.x);
        MSG_WriteShort(&cls.netchan.message, msg.location.y);
        Netchan_Transmit(NS_CLIENT, &cls.netchan);
        msg.cmd = CMD_NO_COMMAND;
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
    cl.time += msec;
    
    CL_ReadPackets();
    CL_SendCommands();
    CL_Input();
    CL_PrepRefresh();
    V_RenderView();
}
