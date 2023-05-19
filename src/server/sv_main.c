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

#define SET_BIT_IF(FLAG, VALUE) \
if (from->VALUE != to->VALUE) \
    bits |= (1 << FLAG);

#define WRITE_IF(FLAG, VALUE, TYPE) \
if (bits & (1 << FLAG)) \
    MSG_Write##TYPE(msg, to->VALUE);

void
MSG_WriteDeltaEntity(LPSIZEBUF msg,
                     LPCENTITYSTATE from,
                     LPCENTITYSTATE to)
{
    int bits = 0;

    SET_BIT_IF(U_ORIGIN1, origin.x);
    SET_BIT_IF(U_ORIGIN2, origin.y);
    SET_BIT_IF(U_ORIGIN3, origin.z);
    SET_BIT_IF(U_ANGLE, angle);
    SET_BIT_IF(U_SCALE, scale);
    SET_BIT_IF(U_FRAME, frame);
    SET_BIT_IF(U_MODEL, model);
    SET_BIT_IF(U_IMAGE, image);
    SET_BIT_IF(U_TEAM, team);

    if (bits == 0)
        return;
    
    MSG_WriteShort(msg, bits);
    MSG_WriteShort(msg, to->number);

    WRITE_IF(U_ORIGIN1, origin.x, Short);
    WRITE_IF(U_ORIGIN2, origin.y, Short);
    WRITE_IF(U_ORIGIN3, origin.z, Short);
    WRITE_IF(U_ANGLE, angle * 100, Short);
    WRITE_IF(U_SCALE, scale * 100, Short);
    WRITE_IF(U_FRAME, frame, Short);
    WRITE_IF(U_MODEL, model, Short);
    WRITE_IF(U_IMAGE, image, Short);
    WRITE_IF(U_TEAM, team, Byte);
}

static void SV_Baseline(LPCLIENT cl) {
    ENTITYSTATE nullstate;
    memset(&nullstate, 0, sizeof(ENTITYSTATE));
    FOR_LOOP(index, ge->num_edicts) {
        LPEDICT e = EDICT_NUM(index);
        MSG_WriteByte(&cl->netchan.message, svc_spawnbaseline);
        MSG_WriteDeltaEntity(&cl->netchan.message, &nullstate, &e->s);
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
            SV_WriteConfigStrings(client);
            SV_Baseline(client);
            client->camera_position.x = 700;
            client->camera_position.y = -1800;
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
            fprintf(stderr, "Unknown model format %.5s in file %s\n", (LPSTR)&fileheader, filename);
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
