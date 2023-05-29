#include "client.h"
#include "renderer.h"
#include "../ui/ui.h"

#include <SDL.h>
#include <StormLib.h>

struct Renderer re;
struct client_static cls;
struct client_state cl;

void CL_Init(void) {
    CON_printf("OpenWarcraft3 v0.1");

    re = *Renderer_Init(&(struct renderer_import) {
        .MemAlloc = MemAlloc,
        .MemFree = MemFree,
        .FileOpen = FS_OpenFile,
        .FileClose = SFileCloseFile,
        .FileExtract = FS_ExtractFile,
        .ParseSheet = FS_ParseSheet,
        .error = CON_printf,
    });

    re.Init(WINDOW_WIDTH, WINDOW_HEIGHT);

    UI_Init();
    
    memset(&cl, 0, sizeof(struct client_state));

    cl.cursor = re.LoadModel("UI\\Feedback\\Confirmation\\Confirmation.mdx");
    
    cl.viewDef.camera.fov = 45;
    cl.viewDef.camera.angles = (VECTOR3) { -40, 0, 0 };
    cl.viewDef.camera.distance = 1500;
    cl.viewDef.camera.zfar = 5000;
    cl.viewDef.camera.znear = 100;

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
                cl.selection.rect.x = event.button.x;
                cl.selection.rect.y = event.button.y;
                break;
            case SDL_MOUSEBUTTONUP:
                button = 0;
                if (moved == false) {
                    CL_SelectEntityAtScreenPoint(event.button.x, event.button.y);
                } else if (cl.selection.inProgress) {
                    CL_SelectEntitiesAtScreenRect(&cl.selection.rect);
                }
                cl.selection.inProgress = false;
                break;
            case SDL_MOUSEMOTION:
                switch (button) {
                    case 3:
                        cl.selection.inProgress = true;
                        cl.selection.rect.width = event.button.x - cl.selection.rect.x;
                        cl.selection.rect.height = event.button.y - cl.selection.rect.y;
                        moved = true;
                        break;
                    case 1:
                        moved = true;
                        cl.viewDef.camera.target.x -= event.motion.xrel * 5;
                        cl.viewDef.camera.target.y += event.motion.yrel * 5;
                        break;
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
    cl.viewDef.camera.target.z = CM_GetHeightAtPoint(cl.viewDef.camera.target.x, cl.viewDef.camera.target.y);
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
    extern clientMessage_t msg;
    static VECTOR3 camera_location;
    if (Vector3_distance(&cl.viewDef.camera.target, &camera_location) > EPSILON) {
        camera_location = cl.viewDef.camera.target;
        MSG_WriteByte(&cls.netchan.message, clc_move);
        MSG_WriteShort(&cls.netchan.message, camera_location.x);
        MSG_WriteShort(&cls.netchan.message, camera_location.y);
        Netchan_Transmit(NS_CLIENT, &cls.netchan);
    }
    if (msg.cmd != CMD_NO_COMMAND && msg.num_entities > 0){
        MSG_WriteByte(&cls.netchan.message, clc_command);
        MSG_WriteByte(&cls.netchan.message, msg.cmd);
        MSG_WriteShort(&cls.netchan.message, msg.num_entities);
        FOR_LOOP(i, msg.num_entities) {
            MSG_WriteShort(&cls.netchan.message, msg.entities[i]);
        }
        MSG_WriteShort(&cls.netchan.message, msg.targetentity);
        MSG_WriteShort(&cls.netchan.message, msg.location.x);
        MSG_WriteShort(&cls.netchan.message, msg.location.y);
        Netchan_Transmit(NS_CLIENT, &cls.netchan);
    }
    msg.cmd = CMD_NO_COMMAND;
}

void CL_Shutdown(void) {
    FOR_LOOP(modelIndex, MAX_MODELS) {
        if (!cl.models[modelIndex])
            continue;
        re.ReleaseModel(cl.models[modelIndex]);
    }

    re.Shutdown();
}

void CL_Frame(DWORD msec) {
    cl.time += msec;

    CL_ReadPackets();
    CL_SendCommands();
    CL_Input();
    CL_PrepRefresh();
    V_RenderView();
}
