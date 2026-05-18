#include "server.h"

#include <arpa/inet.h>
#include <stdlib.h>

#define LAN_FETCH_MSEC 3000

static DWORD SV_LanPlayerCount(void) {
    DWORD count = 0;
    FOR_LOOP(i, svs.num_clients) {
        if (svs.clients[i].state == cs_connected || svs.clients[i].state == cs_spawned) {
            count++;
        }
    }
    return count;
}

static void SV_LanSanitizeValue(LPCSTR in, LPSTR out, size_t out_size) {
    size_t write = 0;

    if (!out || out_size == 0) {
        return;
    }
    out[0] = '\0';
    if (!in) {
        return;
    }

    for (; *in && write + 1 < out_size; in++) {
        char c = *in;
        if (c == '\\') {
            c = '/';
        } else if (c == '\n' || c == '\r') {
            c = ' ';
        }
        out[write++] = c;
    }
    out[write] = '\0';
}

static void SV_LanInfo(const netadr_t *from) {
    char mapname[80];

    if (sv.state != ss_game || !from) {
        return;
    }
    SV_LanSanitizeValue(sv.configstrings[CS_WORLD], mapname, sizeof(mapname));
    Netchan_OutOfBandPrint(NS_SERVER,
                           *from,
                           "info\n\\hostname\\OpenWarcraft3\\mapname\\%s\\players\\%u\\maxplayers\\%u",
                           mapname,
                           (unsigned)SV_LanPlayerCount(),
                           (unsigned)(ge ? ge->max_clients : MAX_CLIENTS));
}

static void SV_LanReadInfoValue(LPCSTR status, LPCSTR key, LPSTR out, size_t out_size) {
    size_t key_len = strlen(key);
    if (!out || out_size == 0) {
        return;
    }
    out[0] = '\0';
    if (!status || !*status) {
        return;
    }
    for (LPCSTR p = status; *p;) {
        if (*p != '\\') {
            p++;
            continue;
        }
        p++;
        LPCSTR k = p;
        while (*p && *p != '\\') {
            p++;
        }
        if (*p != '\\') {
            return;
        }
        size_t len = (size_t)(p - k);
        p++;
        LPCSTR v = p;
        while (*p && *p != '\\' && *p != '\n') {
            p++;
        }
        if (len == key_len && !strncmp(k, key, key_len)) {
            size_t copy = (size_t)(p - v);
            if (copy >= out_size) {
                copy = out_size - 1;
            }
            memcpy(out, v, copy);
            out[copy] = '\0';
            return;
        }
    }
}

static void SV_FetchGamesStart(serverListFetch_t *state) {
    netadr_t adr = { 0 };

    state->deadline = svs.realtime + LAN_FETCH_MSEC;
    state->numRows = 0;
    memset(state->keys, 0, sizeof(state->keys));

    adr.type = NA_BROADCAST;
    adr.port = htons(PORT_SERVER);
    Netchan_OutOfBandPrint(NS_SERVER, adr, "info %i", OW3_PROTOCOL_VERSION);
}

static void SV_FetchGamesFrame(serverListFetch_t *state) {
    if (state->loading && svs.realtime >= state->deadline) {
        SV_ListFetchDone(state);
    }
}

static void SV_FetchGamesInfo(const netadr_t *from, LPCSTR status) {
    DWORD count;
    serverListFetch_t *states = SV_ListFetchStates(&count);
    char hostname[80];
    char mapname[80];
    char players[16];
    char maxplayers[16];
    char address[64];
    char label[256];

    FOR_LOOP(i, count) {
        BOOL duplicate = false;
        serverListFetch_t *state = &states[i];
        if (!state->inuse || strcmp(state->command, "fetch-games") || !state->loading) {
            continue;
        }

        snprintf(address, sizeof(address), "%s", NET_AdrToString(from));
        FOR_LOOP(n, state->numRows) {
            if (!strcmp(state->keys[n], address)) {
                duplicate = true;
                break;
            }
        }
        if (duplicate || state->numRows >= MAX_LIST_FETCH_ROWS) {
            continue;
        }

        SV_LanReadInfoValue(status, "hostname", hostname, sizeof(hostname));
        SV_LanReadInfoValue(status, "mapname", mapname, sizeof(mapname));
        SV_LanReadInfoValue(status, "players", players, sizeof(players));
        SV_LanReadInfoValue(status, "maxplayers", maxplayers, sizeof(maxplayers));
        snprintf(label,
                 sizeof(label),
                 "%s    %s    %u/%u",
                 hostname[0] ? hostname : address,
                 mapname[0] ? mapname : "Unknown map",
                 (unsigned)atoi(players),
                 (unsigned)atoi(maxplayers));

        snprintf(state->keys[state->numRows],
                 sizeof(state->keys[0]),
                 "%s",
                 address);
        SV_ListFetchAdd(state, label);
    }
}

serverListFetchProvider_t const *SV_ListFetchProviders(void) {
    static serverListFetchProvider_t const providers[] = {
        { "fetch-games", SV_FetchGamesStart, SV_FetchGamesFrame, SV_FetchGamesInfo },
        { NULL, NULL, NULL, NULL },
    };

    return providers;
}

void SV_ConnectionlessPacket(const netadr_t *from, LPSIZEBUF msg) {
    char payload[256];
    char command[32] = { 0 };
    char *status;
    DWORD length;

    if (!msg || msg->cursize <= 4) {
        return;
    }

    length = msg->cursize - 4;
    if (length >= sizeof(payload)) {
        length = sizeof(payload) - 1;
    }
    memcpy(payload, msg->data + 4, length);
    payload[length] = '\0';

    status = strchr(payload, '\n');
    if (status) {
        *status++ = '\0';
    }
    sscanf(payload, "%31s", command);
    if (!strcmp(command, "connect")) {
        SV_DirectConnect(from);
    } else if (!strcmp(command, "info") && status && *status) {
        SV_ListFetchInfoResponse(from, status);
    } else if (!strcmp(command, "info")) {
        SV_LanInfo(from);
    }
}
