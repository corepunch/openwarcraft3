#include "server.h"
#include "../common/net_oob.h"
#include <arpa/inet.h>

/* Defined here (not sv_main.c) so the test binary, which links sv_init.c
 * but not sv_main.c, gets the symbol too. */
DWORD sv_advertised_state_version = 0;

void SV_CreateBaseline(void) {
    sv.baselines = MemAlloc(sizeof(entityState_t) * ge->max_edicts);
    FOR_LOOP(entnum, ge->num_edicts) {
        edict_t *svent = EDICT_NUM(entnum);
        sv.baselines[entnum] = svent->s;
        svent->s.number = entnum;
    }
}

void SV_ClientConnect(void) {
    // Reuse slot 0 if it already holds a loopback client (e.g. repeated SV_Map
    // calls without a full SV_Shutdown in between).
    if (svs.num_clients > 0 &&
        svs.clients[0].netchan.remote_address.type == NA_LOOPBACK) {
        netadr_t adr = { NA_LOOPBACK };
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
    sv_advertised_state_version++;
    cl->state = cs_connected;
    // Local client uses the in-process loopback path
    memset(&cl->netchan.remote_address, 0, sizeof(cl->netchan.remote_address));
    cl->netchan.remote_address.type = NA_LOOPBACK;
    SZ_Init(&cl->netchan.message, cl->netchan.message_buf, MAX_MSGLEN);
    netadr_t adr = { NA_LOOPBACK };
    Netchan_OutOfBandPrint(NS_SERVER, adr, "client_connect");
}

/* Find the client slot whose netchan address matches from.  For loopback
 * addresses, slot 0 (the local client) is always returned.
 *
 * cs_free slots are skipped: their netchan.remote_address may be stale
 * from a previous occupant and we must not match on that.  cs_zombie
 * slots ARE matched so that a reconnect during the grace period is
 * detected as "already registered" and rejected by SV_DirectConnect. */
LPCLIENT SV_FindClientByAddr(const netadr_t *from) {
    FOR_LOOP(i, svs.num_clients) {
        LPCLIENT cl = &svs.clients[i];
        if (cl->state == cs_free)
            continue;
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

DWORD SV_NumLiveClients(void) {
    DWORD count = 0;
    FOR_LOOP(i, svs.num_clients) {
        clientState_t s = svs.clients[i].state;
        if (s == cs_connected || s == cs_spawned) count++;
    }
    return count;
}

void SV_DropClient(LPCLIENT client, LPCSTR reason) {
    if (!client) return;
    if (client->state == cs_free || client->state == cs_zombie) return;

    /* Notify the peer if there's a real UDP socket on the other end.
     * Loopback clients don't need an OOB notification; we own both
     * sides of the conversation. */
    if (client->netchan.remote_address.type == NA_IP) {
        if (reason && *reason) {
            Netchan_OutOfBandPrint(NS_SERVER, client->netchan.remote_address,
                                   OOB_DISCONNECT " %s", reason);
        } else {
            Netchan_OutOfBandPrint(NS_SERVER, client->netchan.remote_address,
                                   OOB_DISCONNECT);
        }
    }

    client->state = cs_zombie;
    client->drop_time = svs.realtime;
    sv_advertised_state_version++;
}

/* Sweep called from SV_Frame: transition cs_zombie slots whose grace
 * period has elapsed into cs_free.  The slot is not zeroed here; the
 * next SV_DirectConnect that reuses it does a clean memset. */
void SV_RunZombieExpiry(void) {
    FOR_LOOP(i, svs.num_clients) {
        LPCLIENT cl = &svs.clients[i];
        if (cl->state == cs_zombie &&
            svs.realtime - cl->drop_time >= ZOMBIE_TIME_MS) {
            cl->state = cs_free;
        }
    }
}

/* Register a new remote client that sent the first connection packet.
 *
 * Slot allocation policy: reuse the lowest cs_free slot if any exists,
 * otherwise grow svs.num_clients up to MAX_CLIENTS / ge->max_clients.
 * A reconnect during a slot's cs_zombie grace period is rejected here
 * via SV_FindClientByAddr (which still matches cs_zombie addresses). */
void SV_DirectConnect(const netadr_t *from) {
    if (SV_FindClientByAddr(from))
        return;

    LPCLIENT cl = NULL;
    FOR_LOOP(i, svs.num_clients) {
        if (svs.clients[i].state == cs_free) {
            cl = &svs.clients[i];
            break;
        }
    }
    if (!cl) {
        if (svs.num_clients >= MAX_CLIENTS ||
            svs.num_clients >= ge->max_clients) {
            fprintf(stderr, "SV_DirectConnect: server full\n");
            return;
        }
        cl = &svs.clients[svs.num_clients];
        svs.num_clients++;
    }

    /* Reset on reuse so frames / lastframe / pings from the previous
     * occupant of this slot don't leak into the new session. */
    memset(cl, 0, sizeof(*cl));
    sv_advertised_state_version++;
    cl->state = cs_connected;
    cl->netchan.remote_address = *from;
    SZ_Init(&cl->netchan.message, cl->netchan.message_buf, MAX_MSGLEN);
    Netchan_OutOfBandPrint(NS_SERVER, *from, "client_connect");
    fprintf(stderr, "SV_DirectConnect: new client from %d.%d.%d.%d:%u\n",
            from->ip[0], from->ip[1], from->ip[2], from->ip[3],
            ntohs(from->port));
}

void SV_Map(LPCSTR mapFilename) {
    SV_InitGame();
    memset(&sv, 0, sizeof(struct server));
    sv.state = ss_loading;
    strcpy(sv.configstrings[CS_MODELS+1], mapFilename);
    SZ_Init(&sv.multicast, sv.multicast_buf, MAX_MSGLEN);
    CM_LoadMap(mapFilename);
    SV_ClearWorld();
    SV_CreateBaseline();
    ge->SpawnEntities(CM_GetMapInfo(), CM_GetDoodads());
    sv.state = ss_game;
    sv_advertised_state_version++;  /* map + (later) max_clients changed */
    // Register slot 0 as the local (loopback) client now that the map is ready
    SV_ClientConnect();
}

void SV_InitGame(void) {
    if (svs.initialized) {
        SV_Shutdown();
    }
    svs.initialized = true;
    svs.num_client_entities = UPDATE_BACKUP * MAX_CLIENTS * MAX_PACKET_ENTITIES;
    svs.client_entities = MemAlloc(sizeof(entityState_t) * svs.num_client_entities);
    
    FOR_LOOP(i, ge->max_clients) {
        edict_t *ent = EDICT_NUM(i);
        ent->s.number = i;
//        svs.clients[i].edict = ent;
    }
}

void SV_Shutdown(void) {
    SAFE_DELETE(sv.baselines, MemFree);
    SAFE_DELETE(svs.client_entities, MemFree);
    svs.num_clients = 0;
    svs.initialized = false;
    ge->Shutdown();
}

void SV_Init(void) {
    memset(&svs, 0, sizeof(struct server_static));
    memset(&sv, 0, sizeof(struct server));

    SV_InitGameProgs();
}
