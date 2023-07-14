#include "server.h"

//#define PRINT_ANIMATIONS

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
//    ID_MDLX = MAKEFOURCC('M','D','L','X'),
    ID_SEQS = MAKEFOURCC('S','E','Q','S'),
    ID_CLID = MAKEFOURCC('C','L','I','D'),
    ID_PIVT = MAKEFOURCC('P','I','V','T'),
};

void ConvertMDLXAnimationName(LPANIMATION seq) {
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

typedef struct {
    DWORD nEntries;
    DWORD offset;
    DWORD flags;
} Reference;

struct ReferenceEntry {
    DWORD id;
    DWORD offset;
    DWORD nEntries;
    DWORD version;
};

struct MD33 {
    DWORD ofsRefs;
    DWORD nRefs;
    Reference MODL;
};

struct BoundingSphere {
    VECTOR3 min;
    VECTOR3 max;
    float radius;
};

struct Reference {
    DWORD nEntries;
    DWORD ref;
    DWORD flags;
};

struct Sequence {
    DWORD unknown[2];
    struct Reference name;
    DWORD interval[2];
    float movementSpeed;
    DWORD flags;
    DWORD frequency;
    LONG unk[3];
    LONG unk2;
    struct BoundingSphere boundingSphere;
    LONG d5[3];
};

static HANDLE ReadEntry(HANDLE file, DWORD offset, DWORD size) {
    HANDLE data = MemAlloc(size);
    SFileSetFilePointer(file, offset, NULL, FILE_BEGIN);
    SFileReadFile(file, data, size, NULL, NULL);
    return data;
}

int compare_animation_name(const void *a, const void *b) {
    LPCANIMATION value1 = (LPCANIMATION )a;
    LPCANIMATION value2 = (LPCANIMATION )b;
    return strcmp(value1->name, value2->name);
}

static struct cmodel *SV_LoadModelMD34(HANDLE file) {
    struct cmodel *model = MemAlloc(sizeof(struct cmodel));
    struct MD33 md33;
    SFileReadFile(file, &md33, sizeof(struct MD33), NULL, NULL);
    struct ReferenceEntry *ent = ReadEntry(file, md33.ofsRefs, sizeof(struct ReferenceEntry) * md33.nRefs);
    FOR_LOOP(i, md33.nRefs) {
        struct ReferenceEntry const *re = ent+i;
        if (re->id != MAKEFOURCC('S','Q','E','S'))
            continue;
        struct Sequence *seq = ReadEntry(file, re->offset, re->nEntries * sizeof(struct Sequence));
        model->animations = MemAlloc(sizeof(animation_t) * re->nEntries);
        model->num_animations = re->nEntries;
        DWORD startanim = 0;
        FOR_LOOP(j, re->nEntries) {
            struct Sequence *src = seq+j;
            char *name = ReadEntry(file, ent[src->name.ref].offset, src->name.nEntries);
            LPANIMATION dest = model->animations+j;
            strncpy(model->animations[j].name, name, sizeof(model->animations[j].name));
            dest->interval[0] = startanim + src->interval[0];
            dest->interval[1] = startanim + src->interval[1];
            startanim += src->interval[1];
            MemFree(name);
        }
        qsort(model->animations, model->num_animations, sizeof(animation_t), compare_animation_name);
        FOR_LOOP(j, re->nEntries) {
            LPANIMATION dest = model->animations+j;
            ConvertMDLXAnimationName(dest);
        }
        break;
    }
    return model;
}

static struct cmodel *SV_LoadModelMDLX(HANDLE file) {
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
                    ConvertMDLXAnimationName(model->animations+i);
                }
                break;
            default:
                SFileSetFilePointer(file, size, NULL, FILE_CURRENT);
                break;
        }
    }
#ifdef PRINT_ANIMATIONS
    FOR_LOOP(i, model->num_animations){
        LPANIMATION anim = &model->animations[i];
        printf("  %s %d %d\n",  anim->name, anim->interval[0], anim->interval[1]);
    }
#endif
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
#ifdef PRINT_ANIMATIONS
    printf("%s\n", filename);
#endif
    struct cmodel *model = NULL;
    SFileReadFile(file, &fileheader, 4, NULL, NULL);
    switch (fileheader) {
        case ID_MDLX:
            model = SV_LoadModelMDLX(file);
            break;
        case ID_43DM:
            model = SV_LoadModelMD34(file);
            break;
        default:
            fprintf(stderr, "Unknown model format %.5s in file %s\n", (LPSTR)&fileheader, filename);
            break;
    }
    FS_CloseFile(file);
    return model;
}

int SV_ModelIndex(LPCSTR name) {
//    if (!strcmp(name, "units\\human\\Peasant\\Peasant.mdx")) {
//        name = "Assets\\Units\\Terran\\MarineTychus\\MarineTychus.m3";
//    }
    int modelindex = SV_FindIndex(name, CS_MODELS, MAX_MODELS, true);
    if (!sv.models[modelindex]) {
        sv.models[modelindex] = SV_LoadModel(sv.configstrings[CS_MODELS + modelindex]);
    }
#if 0
    if (!strstr(name, "Doodads\\")) {
        printf("%s\n", name);
//        FOR_LOOP(i, sv.models[modelindex]->num_animations){
//            LPANIMATION anim = &sv.models[modelindex]->animations[i];
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
