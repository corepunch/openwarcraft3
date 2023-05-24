#include "server.h"
#include <stdarg.h>

void SV_CreateBaseline(void) {
    sv.baselines = MemAlloc(sizeof(entityState_t) * ge->max_edicts);
    FOR_LOOP(entnum, ge->num_edicts) {
        LPEDICT svent = EDICT_NUM(entnum);
        sv.baselines[entnum] = svent->s;
        svent->s.number = entnum;
    }
}

void SV_Map(LPCSTR mapFilename) {
    SV_InitGame();
    memset(&sv, 0, sizeof(struct server));
    strcpy(sv.configstrings[CS_MODELS+1], mapFilename);
    CM_LoadMap(mapFilename);
    SV_CreateBaseline();
    ge->SpawnDoodads(CM_GetDoodads());
}

void SV_InitGame(void) {
    if (svs.initialized) {
        SV_Shutdown();
    }
    svs.initialized = true;
    svs.num_client_entities = UPDATE_BACKUP * MAX_CLIENTS * MAX_PACKET_ENTITIES;
    svs.client_entities = MemAlloc(sizeof(entityState_t) * svs.num_client_entities);
}

void SV_Shutdown(void) {
    SAFE_DELETE(sv.baselines, MemFree);
    SAFE_DELETE(svs.client_entities, MemFree);
    ge->Shutdown();
}

void __netchan_init(struct netchan *netchan) {
    memset(netchan, 0, sizeof(struct netchan));
    netchan->message.data = netchan->message_buf;
    netchan->message.maxsize = sizeof(netchan->message_buf);
}

static LPCANIMATION SV_GetAnimation(int modelindex, animationType_t animname) {
    struct cmodel *model = sv.models[modelindex];
    return model ? &model->animtypes[animname] : NULL;
}

void PF_error(char *fmt, ...) {
    char msg[1024];
    va_list argptr;
    va_start(argptr,fmt);
    vsprintf(msg, fmt, argptr);
    va_end(argptr);
    fprintf(stderr, "Game Error: %s\n", msg);
}
VECTOR2 get_flow_direction(float fx, float fy);

void SV_Init(void) {
    ge = GetGameAPI(&(struct game_import) {
        .MemAlloc = MemAlloc,
        .MemFree = MemFree,
        .ModelIndex = SV_ModelIndex,
        .ImageIndex = SV_ImageIndex,
        .SoundIndex = SV_SoundIndex,
        .ParseSheet = FS_ParseSheet,
        .GetAnimation = SV_GetAnimation,
        .GetHeightAtPoint = CM_GetHeightAtPoint,
        .FindPath = CM_FindPath,
        .GetFlowDirection = get_flow_direction,
        .error = PF_error,
    });
    ge->Init();
    memset(&svs, 0, sizeof(struct server_static));
    memset(&sv, 0, sizeof(struct server));
    __netchan_init(&svs.clients[0].netchan);
    svs.num_clients = 1;
    sv.baselines = MemAlloc(sizeof(entityState_t) * ge->max_edicts);
}
