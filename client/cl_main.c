/*
 * cl_main.c — Main client loop and initialization.
 *
 * The client is responsible for three things:
 *   1. Capturing user input and forwarding commands to the server.
 *   2. Receiving game state snapshots from the server and applying them.
 *   3. Preparing and rendering the scene each frame.
 *
 * CL_Frame() is the entry point called from the platform event loop.
 * CL_Init() sets up the renderer and input bindings at startup.
 */
#include "client.h"
#include "renderer.h"
#include <arpa/inet.h>

refExport_t re;

struct client_static cls;
struct client_state cl;

void Cmd_ForwardToServer(LPCSTR text) {
    if (cls.state <= ca_connected || *text == '-' || *text == '+') {
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

    fprintf(stderr, "CL_Init: get renderer API\n");
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
    
    fprintf(stderr, "CL_Init: renderer init\n");
    re.Init(WINDOW_WIDTH, WINDOW_HEIGHT);

    fprintf(stderr, "CL_Init: netchan init\n");
    SZ_Init(&cls.netchan.message, cls.netchan.message_buf, MAX_MSGLEN);
    
    fprintf(stderr, "CL_Init: clear state\n");
    CL_ClearState();

    Cmd_AddCommand("quit", Com_Quit);

    CL_InitInput();

    if (cls.key_dest == key_menu) {
        fprintf(stderr, "CL_Init: menu bindings\n");
        CL_SetMenuBindings();
        cls.state = ca_connecting;
    } else {
        fprintf(stderr, "CL_Init: gameplay setup\n");
        CL_SetGameplayBindings();
    }
    fprintf(stderr, "CL_Init: complete\n");
}

void CL_ConnectionlessPacket(LPSIZEBUF msg) {
    char command[256] = { 0 };
    DWORD length;

    if (msg->cursize <= 4) {
        return;
    }
    length = msg->cursize - 4;
    if (length >= sizeof(command)) {
        length = sizeof(command) - 1;
    }
    memcpy(command, msg->data + 4, length);
    if (strcmp(command, "client_connect")) {
        return;
    }

    MSG_WriteByte(&cls.netchan.message, clc_stringcmd);
    MSG_WriteString(&cls.netchan.message, "new");
    cls.state = ca_connected;
}

/* Read all available server packets from the network buffer and dispatch each
 * message type to the appropriate CL_Parse* handler in cl_parse.c. */
void CL_ReadPackets(void) {
    static BYTE net_message_buffer[MAX_MSGLEN];
    static sizeBuf_t net_message = {
        .data = net_message_buffer,
        .maxsize = MAX_MSGLEN,
        .cursize = 0,
        .readcount = 0,
    };
    netadr_t from;
    int r;
    while ((r = NET_GetPacket(NS_CLIENT, &from, &net_message)) != 0) {
        // Guard: need at least 4 bytes for the OOB marker.
        // Read the header via memcpy to avoid strict-aliasing UB.
        if (r >= 4) {
            int hdr;
            memcpy(&hdr, net_message.data, sizeof(hdr));
            if (hdr == -1) {
                CL_ConnectionlessPacket(&net_message);
                continue;
            }
        }
        CL_ParseServerMessage(&net_message);
    }
}

void CL_SendCmd(void) {
    if (cls.state == ca_disconnected || cls.state == ca_connecting) {
        return;
    }
    Netchan_Transmit(NS_CLIENT, &cls.netchan);
}

/* Set up the netchan to point at a remote server and send an initial
 * connection request.  The server will respond with an out-of-band
 * "client_connect" packet which triggers CL_ConnectionlessPacket(). */
void CL_Connect(LPCSTR host, unsigned short port) {
    netadr_t adr;
    if (!NET_StringToAdr(host, port, &adr)) {
        fprintf(stderr, "CL_Connect: bad server address \"%s\"\n", host);
        return;
    }
    cls.netchan.remote_address = adr;
    SZ_Init(&cls.netchan.message, cls.netchan.message_buf, MAX_MSGLEN);
    cls.state = ca_connecting;
    // Send an out-of-band "connect" request; the server will register this
    // client slot and reply with "client_connect".
    Netchan_OutOfBandPrint(NS_CLIENT, adr, "connect");
    fprintf(stderr, "CL_Connect: connecting to %d.%d.%d.%d:%u\n",
            adr.ip[0], adr.ip[1], adr.ip[2], adr.ip[3], ntohs(adr.port));
}

void CL_Shutdown(void) {
    FOR_LOOP(modelIndex, MAX_MODELS) {
        SAFE_DELETE(cl.models[modelIndex], re.ReleaseModel);
        SAFE_DELETE(cl.portraits[modelIndex], re.ReleaseModel);
    }
    V_Shutdown();
    re.Shutdown();
}

void CL_SendCommand(void) {
    CL_SendCmd();

    Cbuf_Execute();
}

/* Main client tick called from the platform event loop.
 * Advances the client clock, applies incoming server state, samples input,
 * sends commands, and renders the current frame. */
void CL_Frame(DWORD msec) {
    cl.time += msec;

    CL_Input();
    CL_ReadPackets();
    CL_SendCommand();
    CL_PrepRefresh();
    SCR_UpdateScreen();
}
