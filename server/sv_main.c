#include "server.h"

struct game_export *ge;
struct server sv;
struct server_static svs;

static void SV_WriteConfigStrings(struct client *cl) {
    FOR_LOOP(i, MAX_CONFIGSTRINGS) {
        if (!*sv.configstrings[i])
            continue;
        MSG_WriteByte(&cl->netchan.message, svc_configstring);
        MSG_WriteShort(&cl->netchan.message, i);
        MSG_WriteString(&cl->netchan.message, sv.configstrings[i]);
    }
    Netchan_Transmit(&cl->netchan);
}

static void SV_WritePacketEntities(struct client *cl) {
    MSG_WriteByte(&cl->netchan.message, svc_packetentities);
    MSG_WriteShort(&cl->netchan.message, ge->num_edicts);
    FOR_LOOP(index, ge->num_edicts) {
        struct edict *e = EDICT_NUM(index);
        MSG_Write(&cl->netchan.message, &e->s.position, 12);
        MSG_Write(&cl->netchan.message, &e->s.angle, 4);
        MSG_Write(&cl->netchan.message, &e->s.scale, 12);
        MSG_Write(&cl->netchan.message, &e->s.model, 4);
        MSG_Write(&cl->netchan.message, &e->s.image, 4);
    }
    Netchan_Transmit(&cl->netchan);
}

void SV_SendClientMessages(void) {
    FOR_LOOP(i, svs.num_clients) {
        struct client *client = &svs.clients[i];
        if (!client->initialized) {
            SV_WriteConfigStrings(client);
            SV_WritePacketEntities(client);
            client->initialized = true;
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
//    if (sv.state != ss_loading)
//    {    // send the update to everyone
//        SZ_Clear (&sv.multicast);
//        MSG_WriteChar (&sv.multicast, svc_configstring);
//        MSG_WriteShort (&sv.multicast, start+i);
//        MSG_WriteString (&sv.multicast, name);
//        SV_Multicast (vec3_origin, MULTICAST_ALL_R);
//    }
    return i;
}

int SV_ModelIndex(LPCSTR name) {
    return SV_FindIndex(name, CS_MODELS, MAX_MODELS, true);
}

int SV_SoundIndex(LPCSTR name) {
    return SV_FindIndex(name, CS_SOUNDS, MAX_SOUNDS, true);
}

int SV_ImageIndex(LPCSTR name) {
    return SV_FindIndex(name, CS_IMAGES, MAX_IMAGES, true);
}

void __netchan_init(struct netchan *netchan){
    memset(netchan, 0, sizeof(struct netchan));
    netchan->message.data = netchan->message_buf;
    netchan->message.maxsize = sizeof(netchan->message_buf);
}

void SV_Init(void) {
    ge = GetGameAPI(&(struct game_import) {
        .MemAlloc = MemAlloc,
        .MemFree = MemFree,
        .ModelIndex = SV_ModelIndex,
        .ImageIndex = SV_ImageIndex,
        .SoundIndex = SV_SoundIndex,
    });
    ge->Init();
    memset(&svs, 0, sizeof(struct server_static));
    memset(&sv, 0, sizeof(struct server));
    __netchan_init(&svs.clients[0].netchan);
    svs.num_clients = 1;
}

void SV_Frame(int msec) {
    SV_SendClientMessages();
}

void SV_Shutdown(void) {
    ge->Shutdown();
}
