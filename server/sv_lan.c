/*
 * sv_lan.c - Central OOB packet dispatcher for the LAN protocol.
 *
 * Architectural pattern adopted from upstream feature/lan-games at
 * corepunch/openwarcraft3@634b82e ("Add create game setup UI" and
 * preceding LAN work by Igor Chernakov).  Their `SV_ConnectionlessPacket`
 * collapses the per-OOB-token dispatch out of `SV_ReadPackets` into a
 * single entry point; this file is the analogue for our branch.
 *
 * Wire format intentionally diverges from upstream: we keep the Quake 3
 * / dpmaster-flavored tokens (OOB_GETINFO, OOB_INFORESPONSE) defined in
 * common/net_oob.h rather than upstream's single-token `info` form.
 * Token dispatch uses our strict OOB_TOKEN_MATCHES_LITERAL macro so
 * "connectXYZ" does not match "connect".
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>     /* gethostname */
#include <arpa/inet.h>  /* ntohs */

#include "server.h"
#include "sv_info.h"
#include "../common/net_oob.h"

/* Cached at first use; gethostname doesn't change once the system is
 * up.  Filled lazily so we don't need a separate init hook. */
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

/* Wire-level connect handler.  Validates the protocol version embedded
 * in the OOB connect payload before allocating a slot.  On any pre-slot
 * rejection, sends an OOB rejectMsg so the client can surface a real
 * error instead of timing out. */
void SV_HandleConnectOOB(const char *payload, int len,
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

/* Reply to a LAN server-browser getinfo probe with the current server's
 * advertised info.  Format follows Quake 3 / dpmaster: backslash-delimited
 * key/value blob built by SV_BuildInfoResponseString.  No authentication:
 * anyone on the network can solicit this info, by design.
 *
 * The formatted body is cached and only rebuilt when
 * sv_advertised_state_version moves, so a server browsed by N clients
 * pays one snprintf per state change, not N. */
void SV_OOB_GetInfoResponse(const netadr_t *from) {
    static char body[512];
    static int  body_len = 0;
    static DWORD body_for_version = (DWORD)-1;

    if (body_for_version != sv_advertised_state_version) {
        /* Report the live client count, not svs.num_clients, which
         * also counts cs_zombie / cs_free slots. */
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

void SV_ConnectionlessPacket(const netadr_t *from, LPSIZEBUF msg) {
    if (!msg || msg->cursize < 4) return;

    int hdr;
    memcpy(&hdr, msg->data, sizeof(hdr));
    if (hdr != -1) return;

    const char *payload  = (const char *)msg->data + 4;
    int         pay_len  = (int)msg->cursize - 4;

    if (OOB_TOKEN_MATCHES_LITERAL(payload, pay_len, OOB_CONNECT)) {
        SV_HandleConnectOOB(payload, pay_len, from);
        return;
    }
    if (OOB_TOKEN_MATCHES_LITERAL(payload, pay_len, OOB_GETINFO)) {
        SV_OOB_GetInfoResponse(from);
        return;
    }
    if (OOB_TOKEN_MATCHES_LITERAL(payload, pay_len, OOB_DISCONNECT)) {
        /* Peer-initiated disconnect.  Match the address to a known
         * slot; SV_DropClient is a no-op on cs_free / cs_zombie. */
        LPCLIENT cl = SV_FindClientByAddr(from);
        if (cl) SV_DropClient(cl, "peer_request");
        return;
    }
    /* Unknown OOB token: silently drop.  Logging is the caller's job. */
}
