#include "client.h"
#include "renderer.h"

refExport_t re;

struct client_static cls;
struct client_state cl;

void Cmd_ForwardToServer(LPCSTR text) {
    if (/*cls.state <= ca_connected || */*text == '-' || *text == '+') {
        fprintf(stderr, "Unknown command \"%s\"\n", text);
        return;
    }
    MSG_WriteByte(&cls.netchan.message, clc_stringcmd);
    SZ_Printf(&cls.netchan.message, "%s", text);
}

LPCSTR CL_GetConfigString(DWORD index) {
    return cl.configstrings[index];
}

LPCENTITYSTATE CL_GetEntityByIndex(DWORD number) {
    if (number < cl.num_entities) {
        return &cl.ents[number].current;
    } else {
        return NULL;
    }
}

LPCTEXTURE CL_GetTextureByIndex(DWORD index) {
    return cl.pics[index];
}

void CL_ClientCommand(LPCSTR cmd) {
    memset(cls.netchan.message.data, 0, cls.netchan.message.maxsize);
    MSG_WriteByte(&cls.netchan.message, clc_stringcmd);
    SZ_Printf(&cls.netchan.message, "%s", cmd);
}

void CL_ClearState(void) {
    CL_ClearTEnts ();

    memset(&cl, 0, sizeof(struct client_state));

    SZ_Clear (&cls.netchan.message);
}

void CL_Init(void) {
    CON_printf("OpenWarcraft3 v0.1");

    re = R_GetAPI((refImport_t) {
        .MemAlloc = MemAlloc,
        .MemFree = MemFree,
        .FileOpen = FS_OpenFile,
        .FileClose = FS_CloseFile,
        .FileExtract = FS_ExtractFile,
        .ReadSheet = FS_ParseSLK,
        .FindSheetCell = FS_FindSheetCell,
        .error = CON_printf,
    });
    
    re.Init(WINDOW_WIDTH, WINDOW_HEIGHT);

    SZ_Init(&cls.netchan.message, cls.netchan.message_buf, MAX_MSGLEN);
    
    CL_ClearState();

    cl.moveConfirmation = re.LoadModel("UI\\Feedback\\Confirmation\\Confirmation.mdx");
    
    cl.viewDef.camerastate[0].zfar = 5000;
    cl.viewDef.camerastate[0].znear = 100;
    cl.viewDef.camerastate[1].zfar = 5000;
    cl.viewDef.camerastate[1].znear = 100;

    Key_SetBinding(K_MOUSE1, "+select");
    Key_SetBinding('q', "cmd quests");
    Key_SetBinding(K_ESCAPE, "cmd cancel");
    
    CL_InitInput();
}

void CL_ConnectionlessPacket(void) {
    MSG_WriteByte(&cls.netchan.message, clc_stringcmd);
    MSG_WriteString(&cls.netchan.message, "new");
}

void CL_ReadPackets(void) {
    static BYTE net_message_buffer[MAX_MSGLEN];
    static sizeBuf_t net_message = {
        .data = net_message_buffer,
        .maxsize = MAX_MSGLEN,
        .cursize = 0,
        .readcount = 0,
    };
    while (NET_GetPacket(NS_CLIENT, cl.sock, &net_message)) {
        if (*(int *)net_message.data == -1) {
            CL_ConnectionlessPacket();
            continue;
        }
        CL_ParseServerMessage(&net_message);
    }
}

void CL_SendCmd(void) {
    Netchan_Transmit(NS_CLIENT, &cls.netchan);
}

void CL_Shutdown(void) {
    FOR_LOOP(modelIndex, MAX_MODELS) {
        SAFE_DELETE(cl.models[modelIndex], re.ReleaseModel);
        SAFE_DELETE(cl.portraits[modelIndex], re.ReleaseModel);
    }
    re.Shutdown();
}

void CL_SendCommand(void) {
    CL_SendCmd();

    Cbuf_Execute();
}

void CL_Frame(DWORD msec) {
    cl.time += msec;

    CL_ReadPackets();

    CL_Input();

    CL_SendCommand();

    CL_PrepRefresh();

    SCR_UpdateScreen();
}
