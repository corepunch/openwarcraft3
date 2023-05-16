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

#define SET_BIT_IF(flag, value) \
if (from->value != to->value) \
    bits |= (1 << kEntityChangeFlag_##flag);

#define WRITE_IF(flag, value, type) \
if (bits & (1 << kEntityChangeFlag_##flag)) \
    MSG_Write##type(msg, to->value);

void
MSG_WriteDeltaEntity(struct sizebuf *msg,
                     struct entity_state const *from,
                     struct entity_state const *to)
{
    int bits = 0;

    SET_BIT_IF(originX, origin.x);
    SET_BIT_IF(originY, origin.y);
    SET_BIT_IF(originZ, origin.z);
    SET_BIT_IF(angle, angle);
    SET_BIT_IF(scale, scale);
    SET_BIT_IF(frame, frame);
    SET_BIT_IF(model, model);
    SET_BIT_IF(image, image);
    
    if (bits == 0)
        return;
    
    MSG_WriteShort(msg, bits);
    MSG_WriteShort(msg, to->number);

    WRITE_IF(originX, origin.x, Short);
    WRITE_IF(originY, origin.y, Short);
    WRITE_IF(originZ, origin.z, Short);
    WRITE_IF(angle, angle * 100, Short);
    WRITE_IF(scale, scale * 100, Short);
    WRITE_IF(frame, frame, Short);
    WRITE_IF(model, model, Short);
    WRITE_IF(image, image, Short);
}

static void SV_Baseline(struct client *cl) {
    struct entity_state nullstate;
    memset(&nullstate, 0, sizeof(struct entity_state));
    FOR_LOOP(index, ge->num_edicts) {
        struct edict *e = EDICT_NUM(index);
        MSG_WriteByte(&cl->netchan.message, svc_spawnbaseline);
        MSG_WriteDeltaEntity(&cl->netchan.message, &nullstate, &e->s);
    }
    Netchan_Transmit(&cl->netchan);
}

void SV_SendClientDatagram(struct client *client) {
    SV_BuildClientFrame(client);
    SV_WriteFrameToClient(client);
}

void SV_SendClientMessages(void) {
    FOR_LOOP(i, svs.num_clients) {
        struct client *client = &svs.clients[i];
        if (!client->initialized) {
            SV_WriteConfigStrings(client);
            SV_Baseline(client);
            client->camera_position.x = 700;
            client->camera_position.y = -1200;
            client->initialized = true;
        } else {
            SV_SendClientDatagram(client);
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

enum {
    ID_MDLX = MAKEFOURCC('M','D','L','X'),
    ID_SEQS = MAKEFOURCC('S','E','Q','S'),
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
            fprintf(stderr, "Unknown model format %.5s in file %s\n", (char *)&fileheader, filename);
            break;
    }
    SFileCloseFile(file);
    return model;
}

int SV_ModelIndex(LPCSTR name) {
    int modelindex = SV_FindIndex(name, CS_MODELS, MAX_MODELS, true);
    if (!sv.models[modelindex]) {
        sv.models[modelindex] = SV_LoadModel(sv.configstrings[CS_MODELS + modelindex]);
    }
    return modelindex;
}

int SV_SoundIndex(LPCSTR name) {
    return SV_FindIndex(name, CS_SOUNDS, MAX_SOUNDS, true);
}

int SV_ImageIndex(LPCSTR name) {
    return SV_FindIndex(name, CS_IMAGES, MAX_IMAGES, true);
}

void SV_RunGameFrame(int msec) {
    sv.framenum++;
    sv.time += msec;
    ge->RunFrame(msec);
}

void SV_Frame(int msec) {
    SV_RunGameFrame(msec);
    SV_SendClientMessages();
}
