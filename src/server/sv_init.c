#include "server.h"

void SV_CreateBaseline(void) {
    sv.baselines = MemAlloc(sizeof(entityState_t) * ge->max_edicts);
    FOR_LOOP(entnum, ge->num_edicts) {
        edict_t *svent = EDICT_NUM(entnum);
        sv.baselines[entnum] = svent->s;
        svent->s.number = entnum;
    }
}

void SV_Map(LPCSTR mapFilename) {
    SV_InitGame();
    memset(&sv, 0, sizeof(struct server));
    sv.state = ss_loading;
    strcpy(sv.configstrings[CS_MODELS+1], mapFilename);
    SZ_Init(&sv.multicast, sv.multicast_buf, MAX_MSGLEN);
    CM_LoadMap(mapFilename);
    SV_CreateBaseline();
    ge->SpawnEntities(mapFilename, CM_GetDoodads());
    ge->ReadLevel(mapFilename);
    sv.state = ss_game;
}

void SV_InitGame(void) {
    if (svs.initialized) {
        SV_Shutdown();
    }
    svs.initialized = true;
    svs.num_client_entities = UPDATE_BACKUP * MAX_CLIENTS * MAX_PACKET_ENTITIES;
    svs.client_entities = MemAlloc(sizeof(entityState_t) * svs.num_client_entities);
    
    FOR_LOOP(i, ge->max_clients) {
        edict_t *ent = EDICT_NUM(i+1);
        ent->s.number = i+1;
        svs.clients[i].edict = ent;
    }
}

void SV_Shutdown(void) {
    SAFE_DELETE(sv.baselines, MemFree);
    SAFE_DELETE(svs.client_entities, MemFree);
    ge->Shutdown();
}

void SV_ClientConnect(void) {
    svs.num_clients++;
    netadr_t adr = { 0 };
    Netchan_OutOfBandPrint(NS_SERVER, adr, "client_connect");
}

void SV_Init(void) {
    SV_InitGameProgs();
    
    memset(&svs, 0, sizeof(struct server_static));
    memset(&sv, 0, sizeof(struct server));

    FOR_LOOP(index, MAX_CLIENTS) {
        LPCLIENT cl = &svs.clients[index];
        SZ_Init(&cl->netchan.message, cl->netchan.message_buf, MAX_MSGLEN);
    }

    sv.baselines = MemAlloc(sizeof(entityState_t) * ge->max_edicts);
    
    SV_ClientConnect();
}
