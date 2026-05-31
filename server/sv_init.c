#include "server.h"
#include <arpa/inet.h>

static playerType_t SV_LobbyPlayerType(LPCSTR value, playerType_t fallback) {
    if (!value || !*value) {
        return fallback;
    }
    if (!strcmp(value, "human") || !strcmp(value, "open")) {
        return kPlayerTypeHuman;
    }
    if (!strcmp(value, "computer")) {
        return kPlayerTypeComputer;
    }
    if (!strcmp(value, "closed") || !strcmp(value, "none")) {
        return kPlayerTypeNone;
    }
    return fallback;
}

static playerRace_t SV_LobbyPlayerRace(LPCSTR value, playerRace_t fallback) {
    if (!value || !*value) {
        return fallback;
    }
    if (!strcmp(value, "random")) {
        return kPlayerRaceNone;
    }
    if (!strcmp(value, "human")) {
        return kPlayerRaceHuman;
    }
    if (!strcmp(value, "orc")) {
        return kPlayerRaceOrc;
    }
    if (!strcmp(value, "undead")) {
        return kPlayerRaceUndead;
    }
    if (!strcmp(value, "nightelf")) {
        return kPlayerRaceNightElf;
    }
    return fallback;
}

static BOOL SV_LobbyEnsureTeams(LPMAPINFO info, DWORD num_teams) {
    mapTeam_t *teams;

    if (!info) {
        return false;
    }
    if (num_teams <= info->num_teams) {
        return true;
    }
    if (num_teams > MAX_PLAYERS) {
        num_teams = MAX_PLAYERS;
    }
    teams = MemAlloc(sizeof(*teams) * num_teams);
    if (!teams) {
        return false;
    }
    memset(teams, 0, sizeof(*teams) * num_teams);
    if (info->teams && info->num_teams > 0) {
        memcpy(teams, info->teams, sizeof(*teams) * info->num_teams);
        MemFree(info->teams);
    }
    info->teams = teams;
    info->num_teams = num_teams;
    return true;
}

static void SV_LobbyClearPlayerTeams(LPMAPINFO info, DWORD player) {
    if (!info || !info->teams || player >= MAX_PLAYERS) {
        return;
    }
    FOR_LOOP(i, info->num_teams) {
        info->teams[i].playerMasks &= ~(1u << player);
    }
}

static void SV_LobbyMovePlayerToTeam(LPMAPINFO info, DWORD player, DWORD team) {
    if (!info || player >= MAX_PLAYERS || team >= MAX_PLAYERS) {
        return;
    }
    if (!SV_LobbyEnsureTeams(info, team + 1)) {
        return;
    }
    SV_LobbyClearPlayerTeams(info, player);
    info->teams[team].playerMasks |= 1u << player;
}

static void SV_ApplyLobbySettings(LPMAPINFO info) {
    DWORD slots;

    if (!info) {
        return;
    }
    slots = (DWORD)Cvar_Integer("sv_lobby_slots", 0);
    if (slots > MAX_PLAYERS) {
        slots = MAX_PLAYERS;
    }
    FOR_LOOP(slot, slots) {
        char name[64];
        DWORD player;
        LPCSTR value;
        playerType_t type;

        snprintf(name, sizeof(name), "sv_slot%u_map_player", (unsigned)slot);
        player = (DWORD)Cvar_Integer(name, (int)slot);
        if (player >= MAX_PLAYERS || !info->players[player].used) {
            continue;
        }

        snprintf(name, sizeof(name), "sv_slot%u_type", (unsigned)slot);
        value = Cvar_String(name, "");
        type = SV_LobbyPlayerType(value, info->players[player].playerType);
        info->players[player].playerType = type;

        snprintf(name, sizeof(name), "sv_slot%u_race", (unsigned)slot);
        value = Cvar_String(name, "");
        info->players[player].playerRace = SV_LobbyPlayerRace(value, info->players[player].playerRace);

        snprintf(name, sizeof(name), "sv_slot%u_team", (unsigned)slot);
        if (type == kPlayerTypeNone) {
            SV_LobbyClearPlayerTeams(info, player);
        } else {
            SV_LobbyMovePlayerToTeam(info, player, (DWORD)Cvar_Integer(name, (int)player));
        }
    }
}

void SV_CreateBaseline(void) {
    sv.baselines = MemAlloc(sizeof(entityState_t) * ge->max_edicts);
    memset(sv.baselines, 0, sizeof(entityState_t) * ge->max_edicts);
    FOR_LOOP(entnum, ge->num_edicts) {
        edict_t *svent = EDICT_NUM(entnum);
        sv.baselines[entnum] = svent->s;
        svent->s.number = entnum;
    }
}

static void SV_InitMulticast(void) {
    if (sv.multicast.maxsize == 0) {
        SZ_Init(&sv.multicast, sv.multicast_buf, MAX_MSGLEN);
    }
}

static void SV_SanitizeLobbyText(LPCSTR in, LPSTR out, DWORD out_size) {
    DWORD write = 0;

    if (!out || out_size == 0) {
        return;
    }
    out[0] = '\0';
    if (!in) {
        return;
    }
    for (; *in && write + 1 < out_size; in++) {
        unsigned char c = (unsigned char)*in;

        if (c == '\n' || c == '\r') {
            c = ' ';
        }
        if (c < 32) {
            continue;
        }
        out[write++] = (char)c;
    }
    out[write] = '\0';
}

static void SV_LobbySendChat(LPCLIENT client, LPCSTR text) {
    if (!client || !text || !text[0]) {
        return;
    }
    if (client->state != cs_connected && client->state != cs_spawned) {
        return;
    }
    MSG_WriteByte(&client->netchan.message, svc_lobby_chat);
    MSG_WriteString(&client->netchan.message, text);
    Netchan_Transmit(NS_SERVER, &client->netchan);
}

void SV_LobbyBroadcastChat(LPCSTR sender, LPCSTR text) {
    char clean_sender[64];
    char clean_text[256];
    char line[384];

    if (sv.state != ss_lobby || !text || !text[0]) {
        return;
    }
    SV_SanitizeLobbyText(sender && sender[0] ? sender : "Player", clean_sender, sizeof(clean_sender));
    SV_SanitizeLobbyText(text, clean_text, sizeof(clean_text));
    if (!clean_text[0]) {
        return;
    }
    snprintf(line, sizeof(line), "%s: %s", clean_sender[0] ? clean_sender : "Player", clean_text);
    FOR_LOOP(i, svs.num_clients) {
        SV_LobbySendChat(&svs.clients[i], line);
    }
}

static void SV_ClearLobbyClients(void) {
    FOR_LOOP(i, MAX_CLIENTS) {
        memset(&svs.clients[i], 0, sizeof(svs.clients[i]));
    }
    svs.num_clients = 0;
}

static DWORD SV_SaveLobbyClients(netadr_t *addrs, DWORD max_addrs) {
    DWORD count = 0;

    if (sv.state != ss_lobby || !addrs || max_addrs == 0) {
        return 0;
    }
    FOR_LOOP(i, svs.num_clients) {
        LPCLIENT cl = &svs.clients[i];

        if (cl->state != cs_connected && cl->state != cs_spawned) {
            continue;
        }
        if (count >= max_addrs) {
            break;
        }
        addrs[count++] = cl->netchan.remote_address;
    }
    return count;
}

static void SV_RestoreLobbyClients(netadr_t const *addrs, DWORD count) {
    if (!addrs || count == 0) {
        SV_ClientConnect();
        return;
    }
    FOR_LOOP(i, count) {
        LPCLIENT cl;

        if (svs.num_clients >= MAX_CLIENTS ||
            svs.num_clients >= ge->max_clients) {
            break;
        }
        cl = &svs.clients[svs.num_clients++];
        memset(cl, 0, sizeof(*cl));
        cl->state = cs_connected;
        cl->netchan.remote_address = addrs[i];
        SZ_Init(&cl->netchan.message, cl->netchan.message_buf, MAX_MSGLEN);
        Netchan_OutOfBandPrint(NS_SERVER, addrs[i], "client_connect");
    }
}

void SV_ClientConnect(void) {
    // Reuse slot 0 if it already holds a loopback client (e.g. repeated SV_Map
    // calls without a full SV_Shutdown in between).
    if (svs.num_clients > 0 &&
        svs.clients[0].netchan.remote_address.type == NA_LOOPBACK) {
        netadr_t adr = { NA_LOOPBACK };
        SV_InitMulticast();
        Netchan_OutOfBandPrint(NS_SERVER, adr, "client_connect");
        return;
    }
    if (svs.num_clients >= MAX_CLIENTS ||
        svs.num_clients >= ge->max_clients) {
        fprintf(stderr, "SV_ClientConnect: server full\n");
        return;
    }
    LPCLIENT cl = &svs.clients[svs.num_clients];
    svs.num_clients++;
    cl->state = cs_connected;
    SV_InitMulticast();
    // Local client uses the in-process loopback path
    memset(&cl->netchan.remote_address, 0, sizeof(cl->netchan.remote_address));
    cl->netchan.remote_address.type = NA_LOOPBACK;
    SZ_Init(&cl->netchan.message, cl->netchan.message_buf, MAX_MSGLEN);
    netadr_t adr = { NA_LOOPBACK };
    Netchan_OutOfBandPrint(NS_SERVER, adr, "client_connect");
}

/* Find the client slot whose netchan address matches from.  For loopback
 * addresses, slot 0 (the local client) is always returned. */
LPCLIENT SV_FindClientByAddr(const netadr_t *from) {
    FOR_LOOP(i, svs.num_clients) {
        LPCLIENT cl = &svs.clients[i];
        if (from->type == NA_LOOPBACK &&
            cl->netchan.remote_address.type == NA_LOOPBACK)
            return cl;
        if (from->type == NA_IP &&
            cl->netchan.remote_address.type == NA_IP &&
            memcmp(cl->netchan.remote_address.ip, from->ip, 4) == 0 &&
            cl->netchan.remote_address.port == from->port)
            return cl;
    }
    return NULL;
}

/* Register a new remote client that sent the first connection packet. */
void SV_DirectConnect(const netadr_t *from) {
    // Ignore if address is already registered
    if (SV_FindClientByAddr(from))
        return;
    if (svs.num_clients >= MAX_CLIENTS ||
        svs.num_clients >= ge->max_clients) {
        fprintf(stderr, "SV_DirectConnect: server full\n");
        return;
    }
    LPCLIENT cl = &svs.clients[svs.num_clients];
    svs.num_clients++;
    cl->state = cs_connected;
    SV_InitMulticast();
    cl->netchan.remote_address = *from;
    SZ_Init(&cl->netchan.message, cl->netchan.message_buf, MAX_MSGLEN);
    Netchan_OutOfBandPrint(NS_SERVER, *from, "client_connect");
}

void SV_Map(LPCSTR mapFilename) {
    netadr_t lobby_clients[MAX_CLIENTS];
    DWORD num_lobby_clients;

    fprintf(stderr, "Server initialization.\n");
    num_lobby_clients = SV_SaveLobbyClients(lobby_clients, MAX_CLIENTS);
    SV_InitGame();
    memset(&sv, 0, sizeof(struct server));
    sv.state = ss_loading;
    strcpy(sv.configstrings[CS_WORLD], mapFilename);
    SZ_Init(&sv.multicast, sv.multicast_buf, MAX_MSGLEN);
    if (!CM_LoadMap(mapFilename)) {
        fprintf(stderr, "SV_Map: map load failed\n");
        sv.state = ss_dead;
        return;
    }
    SV_ApplyLobbySettings((LPMAPINFO)CM_GetMapInfo());
    SV_ClearWorld();
    SV_CreateBaseline();
    ge->SpawnEntities(CM_GetMapInfo(), CM_GetDoodads());
    sv.state = ss_game;
    // Keep lobby clients connected through the immediate game start.
    SV_RestoreLobbyClients(lobby_clients, num_lobby_clients);
    fprintf(stderr, "Server initialized.\n\n");
}

void SV_StartLobby(LPCSTR mapFilename) {
    if (!mapFilename || !mapFilename[0]) {
        return;
    }
    if (!ge) {
        fprintf(stderr, "SV_StartLobby: game API not initialized\n");
        return;
    }
    if (!svs.initialized) {
        SV_InitGame();
    }
    SAFE_DELETE(sv.baselines, MemFree);
    SV_ClearLobbyClients();
    memset(&sv, 0, sizeof(struct server));
    sv.state = ss_lobby;
    snprintf(sv.configstrings[CS_WORLD], sizeof(sv.configstrings[CS_WORLD]), "%s", mapFilename);
    SV_InitMulticast();
    SV_ClientConnect();
    fprintf(stderr, "Lobby initialized for %s\n", mapFilename);
}

#ifndef TOOL_COMMON_NO_MPQ
static void SV_StartLobby_f(void) {
    if (Cmd_Argc() < 2) {
        fprintf(stderr, "usage: lobby_start <map>\n");
        return;
    }
    SV_StartLobby(Cmd_ArgsFrom(1));
}

static void SV_LobbySay_f(void) {
    SV_LobbyBroadcastChat("Player", Cmd_ArgsFrom(1));
}
#endif

void SV_InitGame(void) {
    if (!ge) {
        fprintf(stderr, "SV_InitGame: game API not initialized\n");
        return;
    }

    if (svs.initialized) {
        SV_Shutdown();
    }

    // SV_Shutdown tears down game state (including ge->edicts), so make sure
    // the game side is initialized before using EDICT_NUM.
    if (!ge->edicts) {
        if (!ge->Init) {
            fprintf(stderr, "SV_InitGame: missing ge->Init callback\n");
            return;
        }
        ge->Init();
    }

    svs.initialized = true;
    svs.num_client_entities = UPDATE_BACKUP * ge->max_clients * MAX_PACKET_ENTITIES;
    svs.client_entities = MemAlloc(sizeof(entityState_t) * svs.num_client_entities);
    
    FOR_LOOP(i, ge->max_clients) {
        edict_t *ent = EDICT_NUM(i);
        ent->s.number = i;
//        svs.clients[i].edict = ent;
    }
}

void SV_Shutdown(void) {
    if (!svs.initialized) {
        return;
    }
    SAFE_DELETE(sv.baselines, MemFree);
    SAFE_DELETE(svs.client_entities, MemFree);
    svs.num_clients = 0;
    svs.initialized = false;
    if (ge && ge->Shutdown) {
        ge->Shutdown();
    }
}

void SV_Init(void) {
    memset(&svs, 0, sizeof(struct server_static));
    memset(&sv, 0, sizeof(struct server));

    SV_InitGameProgs();
    SV_InitGame();
#ifndef TOOL_COMMON_NO_MPQ
    Cmd_AddCommand("lobby_start", SV_StartLobby_f);
    Cmd_AddCommand("lobby_say", SV_LobbySay_f);
#endif
}
