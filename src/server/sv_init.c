#include "server.h"
#include <arpa/inet.h>

void SV_CreateBaseline(void) {
    sv.baselines = MemAlloc(sizeof(entityState_t) * ge->max_edicts);
    FOR_LOOP(entnum, ge->num_edicts) {
        edict_t *svent = EDICT_NUM(entnum);
        sv.baselines[entnum] = svent->s;
        svent->s.number = entnum;
    }
}

void SV_ClientConnect(void) {
    if (svs.num_clients >= MAX_CLIENTS) {
        fprintf(stderr, "SV_ClientConnect: server full\n");
        return;
    }
    LPCLIENT cl = &svs.clients[svs.num_clients];
    svs.num_clients++;
    cl->state = cs_connected;
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
    if (svs.num_clients >= MAX_CLIENTS) {
        fprintf(stderr, "SV_DirectConnect: server full\n");
        return;
    }
    LPCLIENT cl = &svs.clients[svs.num_clients];
    svs.num_clients++;
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
    ge->Shutdown();
}

void SV_Init(void) {
    memset(&svs, 0, sizeof(struct server_static));
    memset(&sv, 0, sizeof(struct server));

    SV_InitGameProgs();
}

