#include "server.h"

struct game_export *ge;
struct server sv;
struct server_static svs;

void SV_WriteConfigString(LPSIZEBUF msg, DWORD i) {
    MSG_WriteByte(msg, svc_configstring);
    MSG_WriteShort(msg, i);
    if (i == CS_STATUSBAR) {
        MSG_Write(msg, sv.configstrings[i], sizeof(*sv.configstrings));
    } else {
        MSG_WriteString(msg, ge->GetThemeValue(sv.configstrings[i]));
    }
}

static void SV_SendClientDatagram(LPCLIENT client) {
    SV_BuildClientFrame(client);
    SV_WriteFrameToClient(client);
    Netchan_Transmit(NS_SERVER, &client->netchan);
}

static void SV_SendClientMessages(void) {
    FOR_LOOP(i, svs.num_clients) {
        LPCLIENT client = &svs.clients[i];
        if (client->state == cs_spawned) {
            SV_SendClientDatagram(client);
        }
    }
}

static void SV_ReadPackets(void) {
    static BYTE net_message_buffer[MAX_MSGLEN];
    static sizeBuf_t net_message = {
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
        sizeBuf_t tmp = sv.multicast;
        static BYTE buf[MAX_PATHLEN + 32];
        memset(buf, 0, sizeof(buf));
        SZ_Init(&sv.multicast, buf, sizeof(buf));
        SV_WriteConfigString(&sv.multicast, start+i);
        SV_Multicast(&(VECTOR3){0,0,0}, MULTICAST_ALL_R);
        sv.multicast = tmp;
    }
    return i;
}

enum {
    ID_MDLX = MAKEFOURCC('M','D','L','X'),
    ID_SEQS = MAKEFOURCC('S','E','Q','S'),
    ID_CLID = MAKEFOURCC('C','L','I','D'),
    ID_PIVT = MAKEFOURCC('P','I','V','T'),
};

void ConvertMDXAnimationName(animation_t *seq) {
    char *last_char = seq->name;
    for (char *ch = seq->name; *ch; ch++) {
        if (isnumber(*ch) || *ch == '-') {
            while (*(++last_char)) {
                *last_char = '\0';
            }
            return;
        } else if (isalpha(*ch)) {
            *ch = tolower(*ch);
            last_char = ch;
        }
    }
}

static struct cmodel *SV_LoadModelMDX(HANDLE file) {
    struct cmodel *model = MemAlloc(sizeof(struct cmodel));
    DWORD header, size;
    while (SFileReadFile(file, &header, 4, NULL, NULL)) {
        SFileReadFile(file, &size, 4, NULL, NULL);
        switch (header) {
            case ID_SEQS:
                model->animations = MemAlloc(size);
                model->num_animations = size / sizeof(*model->animations);
                SFileReadFile(file, model->animations, size, NULL, NULL);
                FOR_LOOP(i, model->num_animations) {
                    ConvertMDXAnimationName(model->animations+i);
                }
                break;
            default:
                SFileSetFilePointer(file, size, NULL, FILE_CURRENT);
                break;
        }
    }
//    FOR_LOOP(i, model->num_animations){
//        animation_t *anim = &model->animations[i];
//        printf("  %s %d %d\n",  anim->name, anim->interval[0], anim->interval[1]);
//    }
    return model;
}

static struct cmodel *SV_LoadModel(LPCSTR filename) {
    DWORD fileheader;
    HANDLE file = FS_OpenFile(filename);
    if (!file) {
        PATHSTR path;
        strcpy(path, filename);
        path[strlen(path)-1] = 'x';
        if (!(file = FS_OpenFile(path))) {
            return NULL;
        }
    }
//    printf("%s\n", filename);
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
//            animation_t *anim = &sv.models[modelindex]->animations[i];
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

int SV_FontIndex(LPCSTR name, DWORD fontSize) {
    PATHSTR fontspec;
    sprintf(fontspec, "%s,%d", name, fontSize);
    return SV_FindIndex(fontspec, CS_FONTS, MAX_FONTSTYLES, true);
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
