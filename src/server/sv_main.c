#include "server.h"

struct game_export *ge;
struct server sv;
struct server_static svs;

static void SV_WriteConfigStrings(LPCLIENT cl) {
    FOR_LOOP(i, MAX_CONFIGSTRINGS) {
        if (!*sv.configstrings[i])
            continue;
        MSG_WriteByte(&cl->netchan.message, svc_configstring);
        MSG_WriteShort(&cl->netchan.message, i);
        MSG_WriteString(&cl->netchan.message, sv.configstrings[i]);
    }
    Netchan_Transmit(NS_SERVER, &cl->netchan);
}

static void SV_WritePlayerInfo(LPCLIENT cl) {
    mapPlayer_t const *player = CM_GetPlayer(1);
    if (player) {
        MSG_WriteByte(&cl->netchan.message, svc_playerinfo);
        MSG_Write(&cl->netchan.message, &player->internalPlayerNumber, sizeof(DWORD));
        MSG_Write(&cl->netchan.message, &player->startingPosition, sizeof(VECTOR2));
        Netchan_Transmit(NS_SERVER, &cl->netchan);
    }
}

static void SV_Baseline(LPCLIENT cl) {
    entityState_t nullstate;
    memset(&nullstate, 0, sizeof(entityState_t));
    FOR_LOOP(index, ge->num_edicts) {
        edict_t *e = EDICT_NUM(index);
        if (e->svflags & SVF_NOCLIENT)
            continue;
        MSG_WriteByte(&cl->netchan.message, svc_spawnbaseline);
        MSG_WriteDeltaEntity(&cl->netchan.message, &nullstate, &e->s, true);
    }
    Netchan_Transmit(NS_SERVER, &cl->netchan);
}

static void SV_SendClientDatagram(LPCLIENT client) {
    SV_BuildClientFrame(client);
    SV_WriteFrameToClient(client);
}

static void SV_SendClientMessages(void) {
    FOR_LOOP(i, svs.num_clients) {
        LPCLIENT client = &svs.clients[i];
        if (!client->initialized) {
            SV_WritePlayerInfo(client);
            SV_WriteConfigStrings(client);
            SV_Baseline(client);
            client->initialized = true;
        } else {
            SV_SendClientDatagram(client);
        }
    }
}

static void SV_ReadPackets(void) {
    static BYTE net_message_buffer[MAX_MSGLEN];
    static struct sizebuf net_message = {
        .data = net_message_buffer,
        .maxsize = MAX_MSGLEN,
        .cursize = 0,
        .readcount = 0,
    };
    FOR_LOOP(i, svs.num_clients) {
        LPCLIENT client = &svs.clients[i];
        while (NET_GetPacket(NS_SERVER, client->netchan.sock, &net_message)) {
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
    if (sv.state != ss_loading) {    // send the update to everyone
        SZ_Clear(&sv.multicast);
        MSG_WriteByte(&sv.multicast, svc_configstring);
        MSG_WriteShort(&sv.multicast, start+i);
        MSG_WriteString(&sv.multicast, name);
        SV_Multicast(&(VECTOR3){0,0,0}, MULTICAST_ALL_R);
    }
    return i;
}

enum {
    ID_MDLX = MAKEFOURCC('M','D','L','X'),
    ID_SEQS = MAKEFOURCC('S','E','Q','S'),
};

typedef struct {
    LPCSTR name;
    animationType_t type;
} animmap_t;

animmap_t animation_map[] = {
    { "stand ready", ANIM_STAND_READY },
    { "stand victory", ANIM_STAND_VICTORY },
    { "stand channel", ANIM_STAND_CHANNEL },
    { "stand hit", ANIM_STAND_HIT },
    { "stand", ANIM_STAND },
    { "walk", ANIM_WALK },
    { "attack", ANIM_ATTACK },
    { "death", ANIM_DEATH },
    { NULL }
};

static struct cmodel *SV_LoadModelMDX(HANDLE file) {
    struct cmodel *model = MemAlloc(sizeof(struct cmodel));
    DWORD header, size;
    while (SFileReadFile(file, &header, 4, NULL, NULL)) {
        SFileReadFile(file, &size, 4, NULL, NULL);
        if (header == ID_SEQS) {
            model->animations = MemAlloc(size);
            model->num_animations = size / sizeof(struct mdx_sequence);
            SFileReadFile(file, model->animations, size, NULL, NULL);
        } else {
            SFileSetFilePointer(file, size, NULL, FILE_CURRENT);
        }
    }
    FOR_LOOP(i, model->num_animations){
        struct mdx_sequence *anim = &model->animations[i];
        for (animmap_t const *map = animation_map; map->name; map++) {
            if (!_strnicmp(anim->name, map->name, strlen(map->name))) {
                animationInfo_t animation = {
                    .firstframe = anim->interval[0],
                    .lastframe = anim->interval[1],
                    .movespeed = anim->movespeed,
                };
                animationTypeVariants_t *vars = &model->animtypes[map->type];
                vars->animations[vars->num_animations++] = animation;
            }
        }
    }
    return model;
}

static struct cmodel *SV_LoadModel(LPCSTR filename) {
    DWORD fileheader;
    HANDLE file = FS_OpenFile(filename);
    if (!file)
        return NULL;
    struct cmodel *model = NULL;
    SFileReadFile(file, &fileheader, 4, NULL, NULL);
    switch (fileheader) {
        case ID_MDLX:
            model = SV_LoadModelMDX(file);
            break;
        default:
            fprintf(stderr, "Unknown model format %.5s in file %s\n", (LPSTR)&fileheader, filename);
            break;
    }
    FS_CloseFile(file);
    return model;
}

int SV_ModelIndex(LPCSTR name) {
    int modelindex = SV_FindIndex(name, CS_MODELS, MAX_MODELS, true);
    if (!sv.models[modelindex]) {
        sv.models[modelindex] = SV_LoadModel(sv.configstrings[CS_MODELS + modelindex]);
    }
#if 0
    if (!strstr(name, "Doodads\\")) {
        printf("%s\n", name);
//        FOR_LOOP(i, sv.models[modelindex]->num_animations){
//            struct mdx_sequence *anim = &sv.models[modelindex]->animations[i];
//            printf("    %s\n", anim->name);
//        }
    }
#endif
    return modelindex;
}

int SV_SoundIndex(LPCSTR name) {
    return SV_FindIndex(name, CS_SOUNDS, MAX_SOUNDS, true);
}

int SV_ImageIndex(LPCSTR name) {
    return SV_FindIndex(name, CS_IMAGES, MAX_IMAGES, true);
}

void SV_RunGameFrame(void) {
    sv.framenum++;
    sv.time += FRAMETIME;
    ge->RunFrame();
}

void SV_Frame(DWORD msec) {
    svs.realtime += msec;
    
    if (svs.realtime < sv.time) {
        return;
    }

    SV_ReadPackets();
    SV_RunGameFrame();
    SV_SendClientMessages();
}
