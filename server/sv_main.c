/*
 * sv_main.c — Main server loop and frame processing.
 *
 * The server advances the game in fixed-size time steps (FRAMETIME).
 * Every frame it reads pending client commands, calls the game library to
 * run one simulation step, and then sends the resulting entity/player state
 * to every connected client.
 *
 * Entry point called from the platform main loop: SV_Frame().
 */
#include "server.h"

//#define PRINT_ANIMATIONS

struct game_export *ge;
struct server sv;
struct server_static svs;

void SV_WriteConfigString(LPSIZEBUF msg, DWORD i) {
    MSG_WriteByte(msg, svc_configstring);
    MSG_WriteShort(msg, i);
    if (i == CS_STATUSBAR) {
        MSG_Write(msg, sv.configstrings[i], sizeof(*sv.configstrings));
    } else {
        MSG_WriteString(msg, ge->GetThemeValue(sv.configstrings[i]));
    }
    sv.syncstrings[i] = true;
}

static void SV_SendClientDatagram(LPCLIENT client) {
    SV_BuildClientFrame(client);
    SV_WriteFrameToClient(client);
    Netchan_Transmit(NS_SERVER, &client->netchan);
}

/* Flush any un-synced config strings to all clients, then send a per-frame
 * datagram to every spawned client containing the current entity snapshot. */
static void SV_SendClientMessages(void) {
    FOR_LOOP(i, MAX_CONFIGSTRINGS) {
        if (*sv.configstrings[i] && !sv.syncstrings[i]) {
            SV_WriteConfigString(&sv.multicast, i);
            SV_Multicast(&(VECTOR3){0,0,0}, MULTICAST_ALL_R);
        }
    }
    FOR_LOOP(i, svs.num_clients) {
        LPCLIENT client = &svs.clients[i];
        if (client->state == cs_spawned) {
            SV_SendClientDatagram(client);
        }
    }
}

/* Read and dispatch all pending client messages from the network buffers. */
static void SV_ReadPackets(void) {
    static BYTE net_message_buffer[MAX_MSGLEN];
    static sizeBuf_t net_message = {
        .data = net_message_buffer,
        .maxsize = MAX_MSGLEN,
        .cursize = 0,
        .readcount = 0,
    };
    netadr_t from;
    int r;
    while ((r = NET_GetPacket(NS_SERVER, &from, &net_message)) != 0) {
        // Out-of-band packets (first 4 bytes == -1) are connection requests.
        // Validate that the payload starts with "connect" before allocating
        // a client slot, to prevent trivial slot-filling / DoS.
        if (r >= 4) {
            int hdr;
            memcpy(&hdr, net_message.data, sizeof(hdr));
            if (hdr == -1) {
                if (r >= 4 + 7 &&
                    memcmp(net_message.data + 4, "connect", 7) == 0) {
                    SV_DirectConnect(&from);
                }
                continue;
            }
        }
        LPCLIENT client = SV_FindClientByAddr(&from);
        if (client) {
            SV_ParseClientMessage(&net_message, client);
        }
    }
}

static int SV_FindIndex(LPCSTR name, int start, int max, bool create) {
    if (!name || !name[0])
        return 0;
    int i;
    for (i=1 ; i<max && sv.configstrings[start+i][0] ; i++)
        if (!strcmp(sv.configstrings[start+i], name))
            return i;
    if (!create)
        return 0;
    strncpy(sv.configstrings[start+i], name, sizeof(*sv.configstrings));
    return i;
}

int SV_ModelIndex(LPCSTR name) {
//    if (!strcmp(name, "units\\human\\Peasant\\Peasant.mdx")) {
//        name = "Assets\\Units\\Terran\\MarineTychus\\MarineTychus.m3";
//    }
    PATHSTR model_filename = { 0 };
    strcpy(model_filename, name);
    if (!strstr(model_filename, ".mdx")) {
        LPSTR mdl = strstr(model_filename, ".mdl");
        mdl = mdl ? mdl : (model_filename + strlen(model_filename));
        strcpy(mdl, ".mdx");
    }
    int modelindex = SV_FindIndex(model_filename, CS_MODELS, MAX_MODELS, true);
    if (!sv.models[modelindex]) {
        sv.models[modelindex] = SV_LoadModel(sv.configstrings[CS_MODELS + modelindex]);
    }
#if 0
    if (!strstr(name, "Doodads\\")) {
        printf("%s\n", name);
        FOR_LOOP(i, sv.models[modelindex]->num_animations){
            LPANIMATION anim = &sv.models[modelindex]->animations[i];
            printf("    %s\n", anim->name);
        }
    }
#endif
    return modelindex;
}

void SV_LoadModels(void) {
    for (DWORD i = 2; i < MAX_MODELS && *sv.configstrings[CS_MODELS + i]; i++) {
        if (sv.models[i])
            continue;
//        LPCSTR filename = sv.configstrings[CS_MODELS + i];
        sv.models[i] = SV_LoadModel(sv.configstrings[CS_MODELS + i]);
    }
}

int SV_SoundIndex(LPCSTR name) {
    return SV_FindIndex(name, CS_SOUNDS, MAX_SOUNDS, true);
}

int SV_ImageIndex(LPCSTR name) {
    return SV_FindIndex(name, CS_IMAGES, MAX_IMAGES, true);
}

int SV_FontIndex(LPCSTR name, DWORD fontSize) {
    PATHSTR fontspec;
    sprintf(fontspec, "%s,%d", name, fontSize);
    return SV_FindIndex(fontspec, CS_FONTS, MAX_FONTSTYLES, true);
}

/* Advance the simulation by one frame: increment the frame counter and game
 * clock, then invoke the game library's RunFrame callback. */
void SV_RunGameFrame(void) {
    sv.framenum++;
    sv.time += FRAMETIME;
    ge->RunFrame();
}

/* Main server tick called from the platform event loop with the elapsed
 * milliseconds since the last call.  Returns immediately if it is not yet
 * time for a new game frame so the caller can do other work. */
void SV_Frame(DWORD msec) {
    svs.realtime += msec;
    
    if (svs.realtime < sv.time) {
        return;
    }

    SV_ReadPackets();

    SV_RunGameFrame();

    SV_SendClientMessages();
}
