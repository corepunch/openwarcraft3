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
#include "../common/net_oob.h"
#include "sv_info.h"
#include "sv_mdns.h"
#include <arpa/inet.h>  // ntohs
#include <unistd.h>     // gethostname

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

/* Wire-level connect handler invoked by SV_ReadPackets.  Validates the
 * protocol version embedded in the OOB connect payload before allocating
 * a slot.  On any pre-slot rejection, send an OOB rejectMsg so the
 * client can surface a real error instead of timing out. */
static void SV_HandleConnectOOB(const char *payload, int len,
                                const netadr_t *from) {
    int requested = SV_ParseConnectVersion(payload, len);
    if (requested != PROTOCOL_VERSION) {
        Netchan_OutOfBandPrint(NS_SERVER, *from,
                               OOB_REJECT_MSG " "
                               OOB_REJECT_VERSION_MISMATCH);
        fprintf(stderr,
                "SV_HandleConnectOOB: rejecting %d.%d.%d.%d:%u "
                "(version %d, expected %d)\n",
                from->ip[0], from->ip[1], from->ip[2], from->ip[3],
                ntohs(from->port), requested, PROTOCOL_VERSION);
        return;
    }
    SV_DirectConnect(from);
}

/* Cached at first use; gethostname is cheap but pointless to call on
 * every getinfo / mDNS refresh.  Filled lazily so we don't need a
 * separate init hook on the server-startup path. */
static const char *SV_Hostname(void) {
    static char cached[64];
    static bool filled = false;
    if (!filled) {
        if (gethostname(cached, sizeof(cached)) != 0) {
            strncpy(cached, "owc3-server", sizeof(cached) - 1);
        }
        cached[sizeof(cached) - 1] = '\0';
        filled = true;
    }
    return cached;
}

/* Reply to a LAN server-browser getinfo probe with the current server's
 * advertised info.  Format follows Quake 3 / dpmaster: backslash-delimited
 * key/value blob built by SV_BuildInfoResponseString.  No authentication:
 * anyone on the network can solicit this info, by design.
 *
 * The formatted body is cached and only rebuilt when
 * sv_advertised_state_version moves, so a server browsed by N clients
 * pays one snprintf per state change, not N. */
static void SV_OOB_GetInfoResponse(const netadr_t *from) {
    static char body[512];
    static int  body_len = 0;
    static DWORD body_for_version = (DWORD)-1;

    if (body_for_version != sv_advertised_state_version) {
        /* Report the live client count, not svs.num_clients, which
         * also counts cs_zombie / cs_free slots that aren't really
         * "in" the game. */
        body_len = SV_BuildInfoResponseString(body, sizeof(body),
                                              SV_Hostname(),
                                              sv.configstrings[CS_MODELS + 1],
                                              SV_NumLiveClients(),
                                              ge ? ge->max_clients : 0,
                                              PROTOCOL_VERSION);
        body_for_version = sv_advertised_state_version;
    }
    Netchan_OutOfBand(NS_SERVER, *from, (DWORD)body_len, (BYTE *)body);
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
        // Out-of-band packets (first 4 bytes == -1) are stateless requests:
        // connect handshake, server-browser getinfo, etc.  Dispatch by the
        // ASCII token that follows the -1 marker.  No client slot is
        // allocated by anything but the connect path.
        if (r >= 4) {
            int hdr;
            memcpy(&hdr, net_message.data, sizeof(hdr));
            if (hdr == -1) {
                const char *oob_token = (const char *)net_message.data + 4;
                int oob_token_max = r - 4;
                if (OOB_TOKEN_MATCHES_LITERAL(oob_token, oob_token_max,
                                              OOB_CONNECT)) {
                    SV_HandleConnectOOB(oob_token, oob_token_max, &from);
                } else if (OOB_TOKEN_MATCHES_LITERAL(oob_token, oob_token_max,
                                                    OOB_GETINFO)) {
                    SV_OOB_GetInfoResponse(&from);
                } else if (OOB_TOKEN_MATCHES_LITERAL(oob_token, oob_token_max,
                                                    OOB_DISCONNECT)) {
                    /* Peer-initiated disconnect.  cs_free addresses
                     * don't match; cs_zombie ones match but
                     * SV_DropClient no-ops on them, so a duplicate
                     * disconnect is harmless. */
                    LPCLIENT cl = SV_FindClientByAddr(&from);
                    if (cl) SV_DropClient(cl, "peer_request");
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

#define SV_MDNS_POLL_INTERVAL_MS 10

/* Main server tick called from the platform event loop with the elapsed
 * milliseconds since the last call.  Returns immediately if it is not yet
 * time for a new game frame so the caller can do other work. */
void SV_Frame(DWORD msec) {
    svs.realtime += msec;

    // mDNS housekeeping at ~100 Hz max, not the host frame rate.  The
    // syscall floor inside avahi_simple_poll_iterate(0) isn't worth
    // paying at 200-1000 Hz on modern displays.
    static DWORD sv_mdns_last_poll_ms = 0;
    if (svs.realtime - sv_mdns_last_poll_ms >= SV_MDNS_POLL_INTERVAL_MS) {
        SV_MDNS_Tick();
        sv_mdns_last_poll_ms = svs.realtime;
    }

    if (svs.realtime < sv.time) {
        return;
    }

    /* Zombie -> free transition for slots whose grace period elapsed.
     * Cheap walk; bounded by svs.num_clients <= MAX_CLIENTS. */
    SV_RunZombieExpiry();

    // Push current state to mDNS unconditionally each game tick; the
    // SV_MDNS_UpdateInfo implementation no-ops when
    // sv_advertised_state_version hasn't moved, so this is essentially
    // free when nothing has changed.  Live count, not allocated count.
    SV_MDNS_UpdateInfo(sv.configstrings[CS_MODELS + 1],
                       SV_NumLiveClients(),
                       ge ? ge->max_clients : 0);

    SV_ReadPackets();

    SV_RunGameFrame();

    SV_SendClientMessages();
}
