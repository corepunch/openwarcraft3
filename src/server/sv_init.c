#include "server.h"
#include <stdarg.h>

void SV_CreateBaseline(void) {
    sv.baselines = MemAlloc(sizeof(ENTITYSTATE) * ge->max_edicts);
    FOR_LOOP(entnum, ge->num_edicts) {
        LPEDICT svent = EDICT_NUM(entnum);
        sv.baselines[entnum] = svent->s;
        svent->s.number = entnum;
    }
}

void SV_Map(LPCSTR szMapFilename) {
    SV_InitGame();
    memset(&sv, 0, sizeof(struct server));
    strcpy(sv.configstrings[CS_MODELS+1], szMapFilename);
    CM_LoadMap(szMapFilename);
    SV_CreateBaseline();
    LPCDOODAD doodads;
    DWORD num_doodads = CM_GetDoodadsArray(&doodads);
    ge->SpawnEntities(doodads, num_doodads);
}

void SV_InitGame(void) {
    if (svs.initialized) {
        SV_Shutdown();
    }
    svs.initialized = true;
    svs.num_client_entities = UPDATE_BACKUP * MAX_CLIENTS * MAX_PACKET_ENTITIES;
    svs.client_entities = MemAlloc(sizeof(ENTITYSTATE) * svs.num_client_entities);
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

static struct AnimationInfo SV_GetAnimation(int modelindex, LPCSTR animname) {
    struct cmodel *model = sv.models[modelindex];
    if (!model) {
        return (struct AnimationInfo) { 0 };
    }
//    FOR_LOOP(i, model->num_animations){
//        struct mdx_sequence *anim = &model->animations[i];
//        printf("%s %d %d\n", anim->name, anim->interval[0], anim->interval[1]);
//    }
    FOR_LOOP(i, model->num_animations){
        struct mdx_sequence *anim = &model->animations[i];
        if (!strcmp(anim->name, animname)) {
            return (struct AnimationInfo) {
                .firstframe = anim->interval[0],
                .lastframe = anim->interval[1],
//                .framerate = anim->,
            };
        }
    }
    return (struct AnimationInfo) {};
}

void PF_error(char *fmt, ...) {
    char msg[1024];
    va_list argptr;
    va_start(argptr,fmt);
    vsprintf(msg, fmt, argptr);
    va_end(argptr);
    fprintf(stderr, "Game Error: %s\n", msg);
}

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
        .error = PF_error,
    });
    ge->Init();
    memset(&svs, 0, sizeof(struct server_static));
    memset(&sv, 0, sizeof(struct server));
    __netchan_init(&svs.clients[0].netchan);
    svs.num_clients = 1;
    sv.baselines = MemAlloc(sizeof(ENTITYSTATE) * ge->max_edicts);
}

