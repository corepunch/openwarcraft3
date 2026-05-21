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
static LPCPLAYER CL_UIGetPlayerState(void);
static DWORD CL_UIGetNumEntities(void);
static LPCENTITYSTATE CL_UIGetEntity(DWORD idx);
static void CL_UIRequestUnitUI(DWORD num_selected, DWORD *entity_nums);
static LPRENDERER CL_UIGetRenderer(void);

static refExport_t CL_GetRendererAPI(refImport_t imp) {
    LPCSTR module = Cvar_String("r_module", "renderer");

    if (module && (!strcmp(module, "stdout") || !strcmp(module, "text"))) {
        return R_StdoutGetAPI(imp);
    }
    if (module && *module && strcmp(module, "renderer")) {
        fprintf(stderr, "Unknown renderer module \"%s\", using renderer\n", module);
    }
    return R_GetAPI(imp);
}

/* UI library FS_ReadFile wrapper that converts to Quake 3 pattern */
static int CL_UI_ReadFile(LPCSTR fileName, void **buf) {
    return FS_ReadFileQ3(fileName, buf);
}

static BOOL CL_UI_HasExtension(LPCSTR name, LPCSTR extension) {
    LPCSTR dot;

    if (!extension || !*extension) {
        return true;
    }
    dot = strrchr(name, '.');
    return dot && !strcasecmp(dot, extension);
}

static int CL_UI_CompareFileNames(const void *a, const void *b) {
    return strcasecmp((LPCSTR)a, (LPCSTR)b);
}

static BOOL CL_UI_ListHasFile(PATHSTR *files, int count, LPCSTR name) {
    for (int i = 0; i < count; i++) {
        if (!strcasecmp(files[i], name)) {
            return true;
        }
    }
    return false;
}

static int CL_UI_GetFileList(LPCSTR path, LPCSTR extension, char *listbuf, int bufsize) {
    enum { MAX_UI_FILELIST = 1024 };
    PATHSTR files[MAX_UI_FILELIST];
    char mask[MAX_PATHLEN * 2];
    SFILE_FIND_DATA find;
    HANDLE handle;
    int count = 0;
    int used = 0;

    if (!path || !*path || !listbuf || bufsize <= 0) {
        return 0;
    }
    listbuf[0] = '\0';
    snprintf(mask, sizeof(mask), "%s\\*", path);

    handle = FS_FindFirstFile(mask, &find);
    while (handle && count < MAX_UI_FILELIST) {
        if (CL_UI_HasExtension(find.cFileName, extension) &&
            !CL_UI_ListHasFile(files, count, find.cFileName)) {
            snprintf(files[count], sizeof(files[count]), "%s", find.cFileName);
            for (char *p = files[count]; *p; p++) {
                if (*p == '/') {
                    *p = '\\';
                }
            }
            count++;
        }
        if (!FS_FindNextFile(handle, &find)) {
            break;
        }
    }
    if (handle) {
        FS_FindClose(handle);
    }

    qsort(files, count, sizeof(files[0]), CL_UI_CompareFileNames);
    for (int i = 0; i < count; i++) {
        int len = (int)strlen(files[i]) + 1;
        if (used + len >= bufsize) {
            break;
        }
        memcpy(listbuf + used, files[i], len);
        used += len;
    }
    if (used < bufsize) {
        listbuf[used] = '\0';
    }
    return count;
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

/* Renderer access callback for UI rendering */
static LPRENDERER CL_UIGetRenderer(void) {
    return &re;
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

    re = CL_GetRendererAPI((refImport_t) {
        .MemAlloc = MemAlloc,
        .MemFree = MemFree,
        .FS_ReadFile = FS_ReadFileQ3,
        .FS_FreeFile = FS_FreeFile,
        .FileExtract = FS_ExtractFile,
        .ReadSheet = FS_ParseSLK,
        .FindSheetCell = FS_FindSheetCell,
        .error = CON_printf,
    });
    
    re.Init(WINDOW_WIDTH, WINDOW_HEIGHT);
    
    /* Initialize UI library */
    ui = UI_GetAPI((uiImport_t) {
        .FS_ReadFile = CL_UI_ReadFile,
        .FS_FreeFile = FS_FreeFile,
        .FS_GetFileList = CL_UI_GetFileList,
        .ReadMapInfo = CM_ReadMapInfo,
        .FindMapPreviewTexture = CM_FindMapPreviewTexture,
        .FreeMapInfo = CM_FreeMapInfo,
        .DefaultMapName = CM_DefaultMapName,
        .ResolveMapInfoString = CM_ResolveMapInfoString,
        .MapNameMatchesFile = CM_MapNameMatchesFile,
        .MapTilesetName = CM_TilesetName,
        .MapSizeName = CM_MapSizeName,
        .SanitizeMapListField = CM_SanitizeMapListField,
        .SanitizeMapInfoText = CM_SanitizeMapInfoText,
        .MemAlloc = MemAlloc,
        .MemFree = MemFree,
        .ModelIndex = CL_ModelIndex,
        .ImageIndex = CL_ImageIndex,
        .FontIndex = CL_FontIndex,
        .Cmd_ExecuteText = Cbuf_AddText,
        .Cvar_String = Cvar_String,
        .GetPlayerState = CL_UIGetPlayerState,
        .GetNumEntities = CL_UIGetNumEntities,
        .GetEntity = CL_UIGetEntity,
        .RequestUnitUI = CL_UIRequestUnitUI,
        .GetRenderer = CL_UIGetRenderer,
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
    SCR_UpdateScreen(msec);
}
