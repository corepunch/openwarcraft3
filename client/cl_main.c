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
#include "cl_browser.h"
#include "cl_mdns.h"
#include "renderer.h"
#include "../common/net_oob.h"
#include <arpa/inet.h>

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
    CL_BrowserInit();
    CL_MDNS_Init();
}

void CL_ConnectionlessPacket(void) {
    MSG_WriteByte(&cls.netchan.message, clc_stringcmd);
    MSG_WriteString(&cls.netchan.message, "new");
}

/* Read all available server packets from the network buffer and dispatch each
 * message type to the appropriate CL_Parse* handler in cl_parse.c.
 *
 * OOB packets are routed by the ASCII token that follows the -1 sequence
 * marker.  Previously this function treated every OOB packet as a
 * "client_connect" ack and unconditionally replied with "new"; that broke
 * once any other OOB type (infoResponse from the server browser, future
 * challenge replies) existed.  Unknown OOB tokens are logged and dropped. */
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
                const char *oob_token = (const char *)net_message.data + 4;
                int oob_token_max = r - 4;
                if (OOB_TOKEN_MATCHES_LITERAL(oob_token, oob_token_max,
                                              OOB_CLIENT_CONNECT)) {
                    CL_ConnectionlessPacket();
                } else if (OOB_TOKEN_MATCHES_LITERAL(oob_token, oob_token_max,
                                                    OOB_INFORESPONSE)) {
                    int token_len = (int)(sizeof(OOB_INFORESPONSE) - 1);
                    CL_BrowserHandleInfoResponse(
                        &from,
                        oob_token + token_len,
                        oob_token_max - token_len);
                } else {
                    fprintf(stderr,
                            "CL_ReadPackets: unknown OOB token (%d bytes)\n",
                            oob_token_max);
                }
                continue;
            }
        }
        CL_ParseServerMessage(&net_message);
    }
}

void CL_SendCmd(void) {
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
    // Send an out-of-band "connect" request; the server will register this
    // client slot and reply with "client_connect".
    Netchan_OutOfBandPrint(NS_CLIENT, adr, "connect");
    fprintf(stderr, "CL_Connect: connecting to %d.%d.%d.%d:%u\n",
            adr.ip[0], adr.ip[1], adr.ip[2], adr.ip[3], ntohs(adr.port));
}

void CL_Shutdown(void) {
    CL_MDNS_Shutdown();
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

/* Main client tick called from the platform event loop.
 * Advances the client clock, applies incoming server state, samples input,
 * sends commands, and renders the current frame. */
void CL_Frame(DWORD msec) {
    cl.time += msec;

    CL_ReadPackets();

    CL_MDNS_Tick();
    /* Tick advances the browser's cached now and rate-limits the
     * expired-entry purge to 1 Hz so the frame-rate loop isn't paying
     * for a full-list scan dozens of times per second. */
    CL_BrowserTick(cl.time);

    CL_Input();

    CL_SendCommand();

    CL_PrepRefresh();

    SCR_UpdateScreen();
}
