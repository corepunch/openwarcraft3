#include "client.h"
#include "renderer.h"
#include "ui.h"

refExport_t re;
uiExport_t ui;

struct client_static cls;
struct client_state cl;

LPCSTR CL_GetConfigString(DWORD index) {
    return cl.configstrings[index];
}

entityState_t const *CL_GetSelectedEntity(void) {
    FOR_LOOP(i, cl.num_entities) {
        if (cl.ents[i].selected)
            return &cl.ents[i].current;
    }
    return NULL;
}

LPCTEXTURE CL_GetTextureByIndex(DWORD index) {
    return cl.pics[index];
}

void CL_ClientCommand(LPCSTR cmd) {
    memset(cls.netchan.message.data, 0, cls.netchan.message.maxsize);
    MSG_WriteByte(&cls.netchan.message, clc_stringcmd);
    SZ_Printf(&cls.netchan.message, "%s", cmd);
}

void CL_Init(void) {
    CON_printf("OpenWarcraft3 v0.1");

    re = R_GetAPI((refImport_t) {
        .MemAlloc = MemAlloc,
        .MemFree = MemFree,
        .FileOpen = FS_OpenFile,
        .FileClose = FS_CloseFile,
        .FileExtract = FS_ExtractFile,
        .ParseSheet = FS_ParseSheet,
        .error = CON_printf,
    });
    
    ui = UI_GetAPI((uiImport_t) {
        .MemAlloc = MemAlloc,
        .MemFree = MemFree,
        .FileOpen = FS_OpenFile,
        .FileClose = FS_CloseFile,
        .ReadFileIntoString = FS_ReadFileIntoString,
        .ParserGetToken = ParserGetToken,
        .ParserError = ParserError,
        .ParseConfig = FS_ParseConfig,
        .FindConfigValue = INI_FindValue,
        .GetConfigString = CL_GetConfigString,
        .GetSelectedEntity = CL_GetSelectedEntity,
        .GetTextureByIndex = CL_GetTextureByIndex,
        .LoadTexture = re.LoadTexture,
        .LoadModel = re.LoadModel,
        .DrawPortrait = re.DrawPortrait,
        .DrawImage = re.DrawImage,
        .ClienCommand = CL_ClientCommand,
        .error = CON_printf,
    });

    re.Init(WINDOW_WIDTH, WINDOW_HEIGHT);
    ui.Init();
    
    memset(&cl, 0, sizeof(struct client_state));

    cl.cursor = re.LoadModel("UI\\Feedback\\Confirmation\\Confirmation.mdx");
    
    cl.viewDef.camera.fov = 45;
    cl.viewDef.camera.angles = (VECTOR3) { -40, 0, 0 };
    cl.viewDef.camera.distance = 1500;
    cl.viewDef.camera.zfar = 5000;
    cl.viewDef.camera.znear = 100;

    SZ_Init(&cls.netchan.message, cls.netchan.message_buf, MAX_MSGLEN);
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

void CL_SendCmd(void) {
//    extern clientMessage_t msg;
    static VECTOR3 camera_location;
    if (Vector3_distance(&cl.viewDef.camera.target, &camera_location) > EPSILON) {
        camera_location = cl.viewDef.camera.target;
        MSG_WriteByte(&cls.netchan.message, clc_move);
        MSG_WriteShort(&cls.netchan.message, camera_location.x);
        MSG_WriteShort(&cls.netchan.message, camera_location.y);
        Netchan_Transmit(NS_CLIENT, &cls.netchan);
    }
//    if (msg.cmd != CMD_NO_COMMAND && msg.num_entities > 0){
//        MSG_WriteByte(&cls.netchan.message, clc_command);
//        MSG_WriteByte(&cls.netchan.message, msg.cmd);
//        MSG_WriteShort(&cls.netchan.message, msg.num_entities);
//        FOR_LOOP(i, msg.num_entities) {
//            MSG_WriteShort(&cls.netchan.message, msg.entities[i]);
//        }
//        MSG_WriteShort(&cls.netchan.message, msg.targetentity);
//        MSG_WriteShort(&cls.netchan.message, msg.location.x);
//        MSG_WriteShort(&cls.netchan.message, msg.location.y);
//        Netchan_Transmit(NS_CLIENT, &cls.netchan);
//    }
//    msg.cmd = CMD_NO_COMMAND;
    
    Netchan_Transmit(NS_CLIENT, &cls.netchan);
}

void CL_Shutdown(void) {
    FOR_LOOP(modelIndex, MAX_MODELS) {
        if (!cl.models[modelIndex])
            continue;
        re.ReleaseModel(cl.models[modelIndex]);
    }

    ui.Shutdown();
    re.Shutdown();
}

void CL_Frame(DWORD msec) {
    cl.time += msec;

    CL_ReadPackets();
    CL_Input();
    CL_SendCmd();
    CL_PrepRefresh();
    SCR_UpdateScreen();
}
