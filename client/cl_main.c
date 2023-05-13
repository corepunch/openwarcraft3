#include "client.h"
#include "renderer.h"

#include <SDL2/SDL.h>

struct Renderer *renderer = NULL;
struct client_state cl = {
    .sock = 0
};

void CL_Init(void) {
    renderer = Renderer_Init(&(struct RendererImport) {
        .MemAlloc = MemAlloc,
        .MemFree = MemFree,
        .FileOpen = FS_OpenFile,
        .FileClose = SFileCloseFile,
        .FileExtract = FS_ExtractFile,
    });
    
    renderer->Init(WINDOW_WIDTH, WINDOW_HEIGHT);
    
    memset(&cl, 0, sizeof(struct client_state));
    
    cl.refdef.fov = 90;
    cl.refdef.vieworg = (struct vector3) { -600, 1500, -1000 };
    cl.refdef.viewangles = (struct vector3) { -20, 0, 0 };
//    cl.refdef.fov = 60;
//    cl.refdef.vieworg = (struct vector3) { -700, 2500, -400 };
//    cl.refdef.viewangles = (struct vector3) { -90, 0, 0 };
}

void CL_Input(void) {
    static int button;
    SDL_Event event;
    while( SDL_PollEvent( &event ) )
    {
        switch( event.type )
        {
            case SDL_KEYUP:
                if( event.key.keysym.sym == SDLK_ESCAPE )
                    return exit(0);
                break;
            case SDL_MOUSEBUTTONDOWN:
                button = event.button.button;
                break;
            case SDL_MOUSEBUTTONUP:
                button = 0;
                break;
            case SDL_MOUSEMOTION:
                if (button == 1) {
                    cl.refdef.vieworg.x += event.motion.xrel * 5;
                    cl.refdef.vieworg.y -= event.motion.yrel * 5;
                }
                break;
            case SDL_WINDOWEVENT:
                switch (event.window.event) {
                    case SDL_WINDOWEVENT_CLOSE:   // exit game
                        return exit(0);
                    default:
                        break;
                }
                break;
        }
    }
}

void CL_ReadPacketEntities(void) {
    uint16_t num_ents = 0;
    NET_Read(cl.sock, &num_ents, 2);
    FOR_LOOP(i, num_ents) {
        struct client_entity *ent = &cl.ents[i];
        NET_Read(cl.sock, &ent->postion, 12);
        NET_Read(cl.sock, &ent->angle, 4);
        NET_Read(cl.sock, &ent->scale, 12);
        NET_Read(cl.sock, &ent->model, 4);
        NET_Read(cl.sock, &ent->skin, 4);
    }
    cl.num_entities = num_ents;
}

void CL_ParseConfigString(void) {
    uint16_t i =0;
    int len;
    NET_Read(cl.sock, &i, 2);
    NET_Read(cl.sock, &len, 4);
    NET_Read(cl.sock, cl.configstrings[i], len);
}

void CL_ReadPackets(void) {
    uint8_t pack_id = 0;
    while (NET_Read(cl.sock, &pack_id, 1)) {
        switch (pack_id) {
            case svc_packetentities:
                CL_ReadPacketEntities();
                break;
            case svc_configstring:
                CL_ParseConfigString();
                break;
            default:
                break;
        }
    }
}

void CL_Shutdown(void) {
    renderer->Shutdown();
}

void CL_Frame(int msec) {
    CL_ReadPackets();
    CL_Input();
    CL_PrepRefresh();
    V_RenderView();
}
