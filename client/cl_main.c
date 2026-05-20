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
uiExport_t ui;

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

/* Forward declarations for UI callbacks */
static BOOL CL_FileExtractWrapper(LPCSTR toExtract, LPCSTR extracted);
static HANDLE CL_LoadFileWrapper(LPCSTR fileName, LPDWORD size);
static LPCPLAYER CL_UIGetPlayerState(void);
static DWORD CL_UIGetNumEntities(void);
static LPCENTITYSTATE CL_UIGetEntity(DWORD idx);
static void CL_UIRequestUnitUI(DWORD num_selected, DWORD *entity_nums);

/* UI library asset indexing wrappers */
static BOOL CL_FileExtractWrapper(LPCSTR toExtract, LPCSTR extracted) {
    return (BOOL)FS_ExtractFile(toExtract, extracted);
}

static HANDLE CL_LoadFileWrapper(LPCSTR fileName, LPDWORD size) {
    /* For text files like FDF, use the string-oriented loader */
    LPSTR text = FS_ReadFileIntoString(fileName);
    if (text && size) {
        *size = (DWORD)strlen(text);
    }
    return text;
}

/* Game state access callbacks for UI library */
static LPCPLAYER CL_UIGetPlayerState(void) {
    return &cl.playerstate;
}

static DWORD CL_UIGetNumEntities(void) {
    return cl.num_entities;
}

static LPCENTITYSTATE CL_UIGetEntity(DWORD idx) {
    if (idx >= MAX_CLIENT_ENTITIES) {
        return NULL;
    }
    return &cl.ents[idx].current;
}

/* Request unit UI data (command card, inventory, build queue) */
static void CL_UIRequestUnitUI(DWORD num_selected, DWORD *entity_nums) {
    if (!entity_nums || num_selected == 0 || num_selected > 12) {
        return;
    }
    
    MSG_WriteByte(&cls.netchan.message, clc_request_unit_ui);
    MSG_WriteByte(&cls.netchan.message, (BYTE)num_selected);
    for (DWORD i = 0; i < num_selected; i++) {
        MSG_WriteShort(&cls.netchan.message, (SHORT)entity_nums[i]);
    }
}

/* Public wrapper for UI library and input system (Phase 8.6) */
void CL_RequestUnitUI(DWORD num_selected, DWORD *entity_nums) {
    CL_UIRequestUnitUI(num_selected, entity_nums);
}

int CL_ModelIndex(LPCSTR modelName) {
    /* Find or register model */
    for (DWORD i = 1; i < MAX_MODELS; i++) {
        if (cl.models[i] == NULL) {
            cl.models[i] = re.LoadModel(modelName);
            return (int)i;
        }
        /* Check if already loaded - compare configstring */
        if (*cl.configstrings[CS_MODELS + i] && !strcmp(cl.configstrings[CS_MODELS + i], modelName)) {
            return (int)i;
        }
    }
    return 0;
}

int CL_ImageIndex(LPCSTR imageName) {
    /* Find or register image */
    for (DWORD i = 1; i < MAX_IMAGES; i++) {
        if (cl.pics[i] == NULL) {
            cl.pics[i] = re.LoadTexture(imageName);
            return (int)i;
        }
        /* Check if already loaded */
        if (*cl.configstrings[CS_IMAGES + i] && !strcmp(cl.configstrings[CS_IMAGES + i], imageName)) {
            return (int)i;
        }
    }
    return 0;
}

int CL_FontIndex(LPCSTR fontName, DWORD fontSize) {
    /* Create font spec string */
    char fontspec[256];
    snprintf(fontspec, sizeof(fontspec), "%s,%u", fontName, fontSize);
    
    /* Find or register font */
    for (DWORD i = 1; i < MAX_FONTSTYLES; i++) {
        if (cl.fonts[i] == NULL) {
            cl.fonts[i] = re.LoadFont(fontName, fontSize);
            return (int)i;
        }
        /* Check if already loaded */
        if (*cl.configstrings[CS_FONTS + i] && !strcmp(cl.configstrings[CS_FONTS + i], fontspec)) {
            return (int)i;
        }
    }
    return 0;
}

void CL_UIMenuCommand(LPCSTR route) {
    CON_printf("CL_UIMenuCommand: %s\\n", route);
    /* Forward menu commands to UI library */
    if (ui.MenuCommand) {
        ui.MenuCommand(route);
    }
}

void CL_Init(void) {
    CON_printf("OpenWarcraft3 v0.1");
    fprintf(stderr, "Console initialized.\n");

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
    
    /* Initialize UI library */
    ui = UI_GetAPI((uiImport_t) {
        .FileOpen = FS_OpenFile,
        .FileExtract = CL_FileExtractWrapper,
        .FileClose = FS_CloseFile,
        .LoadFile = CL_LoadFileWrapper,
        .MemAlloc = MemAlloc,
        .MemFree = MemFree,
        .ModelIndex = CL_ModelIndex,
        .ImageIndex = CL_ImageIndex,
        .FontIndex = CL_FontIndex,
        .GetString = NULL, /* TODO: implement string table */
        .SendCommand = Cbuf_AddText,
        .LocalCommand = Cbuf_AddText,
        .RequestMapList = CL_RequestMapList,
        .RequestMapInfo = CL_RequestMapInfo,
        .RequestGameList = CL_RequestGameList,
        .RequestPlayerInfo = CL_RequestPlayerList,
        .GetPlayerState = CL_UIGetPlayerState,
        .GetNumEntities = CL_UIGetNumEntities,
        .GetEntity = CL_UIGetEntity,
        .RequestUnitUI = CL_UIRequestUnitUI,
        .Error = CON_printf,
        .Printf = CON_printf,
    });
    
    if (ui.Init) {
        ui.Init();
    }

    SZ_Init(&cls.netchan.message, cls.netchan.message_buf, MAX_MSGLEN);
    
    CL_ClearState();

    Cmd_AddCommand("quit", Com_Quit);

    CL_InitInput();

    if (cls.key_dest == key_menu) {
        CL_SetMenuBindings();
        cls.state = ca_connecting;
    } else {
        CL_SetGameplayBindings();
    }
}

void CL_ConnectionlessPacket(const netadr_t *from, LPSIZEBUF msg) {
    char payload[1024] = { 0 };
    char command[256] = { 0 };
    DWORD length;

    (void)from;
    if (msg->cursize <= 4) {
        return;
    }
    length = msg->cursize - 4;
    if (length >= sizeof(payload)) {
        length = sizeof(payload) - 1;
    }
    memcpy(payload, msg->data + 4, length);
    sscanf(payload, "%255s", command);
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
    while ((r = (cls.netchan.remote_address.type == NA_LOOPBACK
                 ? NET_GetLoopPacket(NS_CLIENT, &from, &net_message)
                 : NET_GetPacket(NS_CLIENT, &from, &net_message))) != 0) {
        // Guard: need at least 4 bytes for the OOB marker.
        // Read the header via memcpy to avoid strict-aliasing UB.
        if (r >= 4) {
            int hdr;
            memcpy(&hdr, net_message.data, sizeof(hdr));
            if (hdr == -1) {
                CL_ConnectionlessPacket(&from, &net_message);
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
    if (ui.Shutdown) {
        ui.Shutdown();
    }
    FOR_LOOP(modelIndex, MAX_MODELS) {
        SAFE_DELETE(cl.models[modelIndex], re.ReleaseModel);
        SAFE_DELETE(cl.portraits[modelIndex], re.ReleaseModel);
    }
    FOR_LOOP(imageIndex, MAX_DYNAMIC_IMAGES) {
        SAFE_DELETE(cl.dynamicPics[imageIndex], re.ReleaseTexture);
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

    /* Update UI library */
    if (ui.Refresh) {
        ui.Refresh(msec);
    }

    CL_Input();
    CL_ReadPackets();
    CL_SendCommand();
    CL_PrepRefresh();
    SCR_UpdateScreen();
}
