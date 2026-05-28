#include <stdarg.h>
#include <unistd.h>

#include "server.h"

/* UI layout byte tracking (legacy - now handled client-side) */
DWORD layoutBytesWritten = 0;

void PF_WriteByte(int c) { MSG_WriteByte(&sv.multicast, c); }
void PF_WriteShort(int c) { MSG_WriteShort(&sv.multicast, c); }
void PF_WriteLong(int c) { MSG_WriteLong(&sv.multicast, c); }
void PF_WriteFloat(float f) { MSG_WriteFloat(&sv.multicast, f); }
void PF_WriteString(LPCSTR s) { MSG_WriteString(&sv.multicast, s); }
void PF_WritePos(LPCVECTOR3 pos) { MSG_WritePos(&sv.multicast, pos); }
void PF_WriteDir(LPCVECTOR3 dir) { MSG_WriteDir(&sv.multicast, dir); }
void PF_WriteAngle(float f) { MSG_WriteAngle(&sv.multicast, f); }

void PF_WriteEntity(LPCENTITYSTATE ent) {
    entityState_t empty;
    memset(&empty, 0, sizeof(entityState_t));
    MSG_WriteDeltaEntity(&sv.multicast, &empty, ent, true);
}

void PF_WriteUIFrame(LPCUIFRAME frame) {
    DWORD before = sv.multicast.cursize;
    uiFrame_t empty;
    memset(&empty, 0, sizeof(uiFrame_t));
    empty.tex.coord[1] = 0xff;
    empty.tex.coord[3] = 0xff;
    MSG_WriteDeltaUIFrame(&sv.multicast, &empty, frame, true);
    MSG_WriteShort(&sv.multicast, frame->buffer.size);
    MSG_Write(&sv.multicast, frame->buffer.data, frame->buffer.size);
    if (sv.multicast.cursize >= before) {
        extern DWORD layoutBytesWritten;
        layoutBytesWritten += sv.multicast.cursize - before;
    }
}

void PF_Confignstring(DWORD index, LPCSTR value, DWORD len) {
    if (index < 0 || index >= MAX_CONFIGSTRINGS) {
        fprintf(stderr, "configstring: bad index %i\n", index);
        return;
    }
    
    memset(sv.configstrings[index], 0, sizeof(sv.configstrings[index]));
    memcpy(sv.configstrings[index], value, len);
    
    //    if (sv.state != ss_loading)
    //    {    // send the update to everyone
    //        SZ_Clear (&sv.multicast);
    //        MSG_WriteChar (&sv.multicast, svc_configstring);
    //        MSG_WriteShort (&sv.multicast, index);
    //        MSG_WriteString (&sv.multicast, value);
    //
    //        SV_Multicast (vec3_origin, MULTICAST_ALL_R);
    //    }
}

void PF_Configstring(DWORD index, LPCSTR value) {
    if (!value) {
        value = "";
    }

    PF_Confignstring(index, value, (DWORD)(strlen(value) + 1));
    strcpy(sv.configstrings[index], value);
}

DWORD SV_GetTime(void) {
    return sv.time;
}

void PF_Multicast(LPCVECTOR3 origin, multicast_t to) {
    SV_Multicast(origin, to);
}

void PF_Unicast(edict_t *ent) {
    if (!ent) {
        SZ_Clear(&sv.multicast);
        return;
    }
    DWORD p = NUM_FOR_EDICT(ent);
    LPCLIENT client = NULL;
    if (p >= 1 && p <= ge->max_clients && p <= svs.num_clients) {
        client = svs.clients + (p - 1);
    } else if (svs.num_clients == 1) {
        client = svs.clients;
    } else {
        SZ_Clear(&sv.multicast);
        return;
    }
    SZ_Write(&client->netchan.message, sv.multicast.data, sv.multicast.cursize);
    SZ_Clear(&sv.multicast);
    Netchan_Transmit(NS_SERVER, &client->netchan);
}

void PF_error(LPCSTR fmt, ...) {
    char msg[1024];
    va_list argptr;
    va_start(argptr,fmt);
    vsprintf(msg, fmt, argptr);
    va_end(argptr);
    fprintf(stderr, "Game Error: %s\n", msg);
}

enum {
//    ID_MDLX = MAKEFOURCC('M','D','L','X'),
    ID_SEQS = MAKEFOURCC('S','E','Q','S'),
    ID_CLID = MAKEFOURCC('C','L','I','D'),
    ID_PIVT = MAKEFOURCC('P','I','V','T'),
};

DWORD fnv1a32(LPCSTR str) {
    DWORD prime = 16777619;
    DWORD hash = 2166136261;
    while (*str) {
        hash = (hash ^ *str++) * prime;
    }
    return hash;
}

void ConvertMDLXAnimationName(LPANIMATION seq) {
    char buffer[80];
    char *last_char = buffer;
    memset(buffer, 0, sizeof(buffer));
    strcpy(buffer, seq->name);
    for (char *ch = buffer; *ch; ch++) {
        if (isdigit(*ch) || *ch == '-') {
            while (*(++last_char)) {
                *last_char = '\0';
            }
            seq->syncpoint = fnv1a32(buffer);
            return;
        } else if (isalpha(*ch)) {
            *ch = tolower(*ch);
            last_char = ch;
        }
    }
    for (DWORD i = (DWORD)strlen(buffer)-1; i > 0 && isspace(buffer[i]); i--) {
        buffer[i] = '\0';
    }
    seq->syncpoint = fnv1a32(buffer);
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
    DWORD fileSize = SFileGetFileSize(file, NULL);
    DWORD payloadSize = fileSize > 4 ? fileSize - 4 : 0;
    BYTE *payload = NULL;
    BYTE *ptr;
    BYTE *end;

    if (payloadSize > 0) {
        DWORD bytesRead = 0;
        payload = MemAlloc(payloadSize);
        SFileReadFile(file, payload, payloadSize, &bytesRead, NULL);
        payloadSize = bytesRead;
    }
    ptr = payload;
    end = payload + payloadSize;
    while (ptr && ptr + 8 <= end) {
        DWORD header, size;
        memcpy(&header, ptr, sizeof(DWORD));
        memcpy(&size, ptr + 4, sizeof(DWORD));
        ptr += 8;
        if (ptr + size > end) {
            size = (DWORD)(end - ptr);
        }
        switch (header) {
            case ID_SEQS:
                model->animations = MemAlloc(size);
                model->num_animations = size / sizeof(*model->animations);
                memcpy(model->animations, ptr, size);
                FOR_LOOP(i, model->num_animations) {
                    ConvertMDLXAnimationName(model->animations+i);
                }
                break;
        }
        ptr += size;
    }
    MemFree(payload);
#ifdef PRINT_ANIMATIONS
    FOR_LOOP(i, model->num_animations){
        LPANIMATION anim = &model->animations[i];
        printf("  %s %d %d\n",  anim->name, anim->interval[0], anim->interval[1]);
    }
#endif
    return model;
}

#ifdef WOW
typedef struct {
    int32_t size;
    int32_t offset;
} svM2Array_t;

typedef struct {
    DWORD magic;
    DWORD version;
    svM2Array_t name;
    DWORD flags;
    svM2Array_t global_loops;
    svM2Array_t sequences;
} svM2Header_t;

typedef struct {
    WORD animation_id;
    WORD sub_animation_id;
    DWORD start_timestamp;
    DWORD end_timestamp;
    FLOAT movement_speed;
    DWORD flags;
    SHORT probability;
    WORD padding;
    DWORD minimum_repetitions;
    DWORD maximum_repetitions;
    DWORD blend_time;
    VECTOR3 min;
    VECTOR3 max;
    FLOAT radius;
    SHORT next_animation;
    WORD alias_next;
} svM2SequenceClassic_t;

typedef struct {
    WORD animation_id;
    WORD sub_animation_id;
    DWORD length;
    FLOAT movement_speed;
    DWORD flags;
    DWORD frequency;
    DWORD minimum_repetitions;
    DWORD maximum_repetitions;
    DWORD blend_time;
    VECTOR3 min;
    VECTOR3 max;
    FLOAT radius;
    SHORT next_animation;
    WORD alias_next;
} svM2SequenceModern_t;

static BOOL SV_M2ArrayRange(svM2Array_t array, DWORD elem_size, DWORD file_size, DWORD *offset, DWORD *bytes) {
    if (array.size <= 0 || array.offset < 0 || elem_size == 0) {
        return false;
    }
    if ((DWORD)array.size > (((DWORD)~0u) / elem_size)) {
        return false;
    }
    *offset = (DWORD)array.offset;
    *bytes = (DWORD)array.size * elem_size;
    return *offset <= file_size && *bytes <= file_size - *offset;
}

static BOOL SV_M2FindPayload(BYTE const *data, DWORD size, BYTE const **payload, DWORD *payload_size) {
    BYTE const *ptr;
    BYTE const *end;

    if (!data || size < sizeof(DWORD)) {
        return false;
    }
    if (*(DWORD const *)data == ID_MD20) {
        *payload = data;
        *payload_size = size;
        return true;
    }
    if (*(DWORD const *)data != ID_MD21 && *(DWORD const *)data != ID_12DM) {
        return false;
    }

    ptr = data;
    end = data + size;
    while (ptr + 8 <= end) {
        DWORD tag;
        DWORD chunk_size;
        memcpy(&tag, ptr, sizeof(tag));
        memcpy(&chunk_size, ptr + 4, sizeof(chunk_size));
        ptr += 8;
        if (chunk_size > (DWORD)(end - ptr)) {
            return false;
        }
        if (tag == ID_MD20 ||
            ((tag == ID_MD21 || tag == ID_12DM) && chunk_size >= sizeof(DWORD) && *(DWORD const *)ptr == ID_MD20)) {
            *payload = ptr;
            *payload_size = chunk_size;
            return true;
        }
        ptr += chunk_size;
    }
    return false;
}

static void SV_M2AnimationName(WORD id, LPSTR out, DWORD out_size) {
    LPCSTR name = NULL;

    switch (id) {
        case 0: name = "Stand"; break;
        case 1: name = "Death"; break;
        case 2: name = "Spell"; break;
        case 3: name = "Stop"; break;
        case 4: name = "Walk"; break;
        case 5: name = "Run"; break;
        case 6: name = "Dead"; break;
        case 7: name = "Rise"; break;
        case 8: name = "StandWound"; break;
        case 9: name = "CombatWound"; break;
        case 10: name = "CombatCritical"; break;
        case 11: name = "ShuffleLeft"; break;
        case 12: name = "ShuffleRight"; break;
        case 13: name = "WalkBackwards"; break;
        case 14: name = "Stun"; break;
        case 15: name = "HandsClosed"; break;
        case 16: name = "AttackUnarmed"; break;
        case 17: name = "Attack1H"; break;
        case 18: name = "Attack2H"; break;
        case 19: name = "Attack2HL"; break;
        case 20: name = "ParryUnarmed"; break;
        case 21: name = "Parry1H"; break;
        case 22: name = "Parry2H"; break;
        case 23: name = "Parry2HL"; break;
        case 24: name = "ShieldBlock"; break;
        case 25: name = "ReadyUnarmed"; break;
        case 26: name = "Ready1H"; break;
        case 27: name = "Ready2H"; break;
        case 28: name = "Ready2HL"; break;
        case 29: name = "ReadyBow"; break;
        case 30: name = "Dodge"; break;
        case 37: name = "JumpStart"; break;
        case 38: name = "Jump"; break;
        case 39: name = "JumpEnd"; break;
        case 40: name = "Fall"; break;
        case 41: name = "SwimIdle"; break;
        case 42: name = "Swim"; break;
        default: break;
    }

    if (name) {
        snprintf(out, out_size, "%s", name);
    } else {
        snprintf(out, out_size, "Animation%u", (unsigned)id);
    }
}

static WORD SV_M2SequenceAnimationId(BYTE const *sequence, BOOL classic) {
    return classic
        ? ((svM2SequenceClassic_t const *)sequence)->animation_id
        : ((svM2SequenceModern_t const *)sequence)->animation_id;
}

static DWORD SV_M2SequenceLength(BYTE const *sequence, BOOL classic) {
    if (classic) {
        svM2SequenceClassic_t const *classic_sequence = (svM2SequenceClassic_t const *)sequence;
        return classic_sequence->end_timestamp > classic_sequence->start_timestamp
            ? classic_sequence->end_timestamp - classic_sequence->start_timestamp
            : 0;
    }
    return ((svM2SequenceModern_t const *)sequence)->length;
}

static FLOAT SV_M2SequenceMoveSpeed(BYTE const *sequence, BOOL classic) {
    return classic
        ? ((svM2SequenceClassic_t const *)sequence)->movement_speed
        : ((svM2SequenceModern_t const *)sequence)->movement_speed;
}

static DWORD SV_M2SequenceFlags(BYTE const *sequence, BOOL classic) {
    return classic
        ? ((svM2SequenceClassic_t const *)sequence)->flags
        : ((svM2SequenceModern_t const *)sequence)->flags;
}

static SHORT SV_M2SequenceRarity(BYTE const *sequence, BOOL classic) {
    return classic
        ? ((svM2SequenceClassic_t const *)sequence)->probability
        : (SHORT)((svM2SequenceModern_t const *)sequence)->frequency;
}

static VECTOR3 SV_M2SequenceMin(BYTE const *sequence, BOOL classic) {
    return classic
        ? ((svM2SequenceClassic_t const *)sequence)->min
        : ((svM2SequenceModern_t const *)sequence)->min;
}

static VECTOR3 SV_M2SequenceMax(BYTE const *sequence, BOOL classic) {
    return classic
        ? ((svM2SequenceClassic_t const *)sequence)->max
        : ((svM2SequenceModern_t const *)sequence)->max;
}

static FLOAT SV_M2SequenceRadius(BYTE const *sequence, BOOL classic) {
    return classic
        ? ((svM2SequenceClassic_t const *)sequence)->radius
        : ((svM2SequenceModern_t const *)sequence)->radius;
}

static BOOL SV_M2AnimationNameExists(LPCANIMATION animations, DWORD count, LPCSTR name) {
    FOR_LOOP(i, count) {
        if (!strcasecmp(animations[i].name, name)) {
            return true;
        }
    }
    return false;
}

static DWORD SV_M2AnimationSyncPoint(LPCSTR name) {
    char buffer[80];
    DWORD i;

    memset(buffer, 0, sizeof(buffer));
    strncpy(buffer, name, sizeof(buffer) - 1);
    for (i = 0; buffer[i]; i++) {
        buffer[i] = (char)tolower(buffer[i]);
    }
    return fnv1a32(buffer);
}

static struct cmodel *SV_LoadModelM2(HANDLE file) {
    struct cmodel *model = MemAlloc(sizeof(struct cmodel));
    DWORD file_size = SFileGetFileSize(file, NULL);
    DWORD read_size = 0;
    BYTE *data = NULL;
    BYTE const *payload = NULL;
    DWORD payload_size = 0;
    svM2Header_t const *header;
    DWORD sequences_offset;
    DWORD sequences_bytes;
    BYTE const *sequences;
    DWORD sequence_stride;
    BOOL classic_sequences;
    DWORD sequence_count;

    memset(model, 0, sizeof(*model));
    if (file_size < sizeof(DWORD)) {
        return model;
    }

    data = MemAlloc(file_size);
    SFileSetFilePointer(file, 0, NULL, FILE_BEGIN);
    SFileReadFile(file, data, file_size, &read_size, NULL);
    if (read_size < sizeof(svM2Header_t) ||
        !SV_M2FindPayload(data, read_size, &payload, &payload_size) ||
        payload_size < sizeof(svM2Header_t)) {
        MemFree(data);
        return model;
    }

    header = (svM2Header_t const *)payload;
    classic_sequences = header->version <= 263;
    sequence_stride = classic_sequences ? sizeof(svM2SequenceClassic_t) : sizeof(svM2SequenceModern_t);
    if (!SV_M2ArrayRange(header->sequences, sequence_stride, payload_size, &sequences_offset, &sequences_bytes)) {
        MemFree(data);
        return model;
    }

    sequences = payload + sequences_offset;
    sequence_count = sequences_bytes / sequence_stride;
    model->animations = MemAlloc(sizeof(animation_t) * sequence_count);
    memset(model->animations, 0, sizeof(animation_t) * sequence_count);

    DWORD frame_base = 0;
    FOR_LOOP(i, sequence_count) {
        BYTE const *src = sequences + i * sequence_stride;
        char name[80];
        DWORD length = MAX(SV_M2SequenceLength(src, classic_sequences), 1);

        SV_M2AnimationName(SV_M2SequenceAnimationId(src, classic_sequences), name, sizeof(name));
        if (!SV_M2AnimationNameExists(model->animations, model->num_animations, name)) {
            LPANIMATION dest = model->animations + model->num_animations++;

            strncpy(dest->name, name, sizeof(dest->name) - 1);
            dest->interval[0] = frame_base;
            dest->interval[1] = frame_base + length;
            dest->movespeed = SV_M2SequenceMoveSpeed(src, classic_sequences);
            dest->flags = SV_M2SequenceFlags(src, classic_sequences);
            dest->rarity = SV_M2SequenceRarity(src, classic_sequences);
            dest->syncpoint = SV_M2AnimationSyncPoint(dest->name);
            dest->radius = SV_M2SequenceRadius(src, classic_sequences);
            dest->min = SV_M2SequenceMin(src, classic_sequences);
            dest->max = SV_M2SequenceMax(src, classic_sequences);
        }
        frame_base += length;
    }

    if (model->num_animations > 1) {
        qsort(model->animations, model->num_animations, sizeof(animation_t), compare_animation_name);
    }
    MemFree(data);
    return model;
}
#endif

struct cmodel *SV_LoadModel(LPCSTR filename) {
    DWORD fileheader;
    HANDLE file = FS_OpenFile(filename);
    if (!file) {
        PATHSTR path;
        strcpy(path, filename);
        path[strlen(path)-1] = 'x';
        file = FS_OpenFile(path);
#ifdef WOW
        if (!file && strstr(filename, ".mdx")) {
            LPSTR ext;
            strcpy(path, filename);
            ext = strstr(path, ".mdx");
            strcpy(ext, ".m2");
            file = FS_OpenFile(path);
        }
#endif
        if (!file)
            return NULL;
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
        case ID_MD20:
        case ID_MD21:
        case ID_12DM:
#ifdef WOW
            model = SV_LoadModelM2(file);
#else
            model = MemAlloc(sizeof(*model));
            memset(model, 0, sizeof(*model));
#endif
            break;
        default:
            fprintf(stderr, "Unknown model format %.5s in file %s\n", (LPSTR)&fileheader, filename);
            break;
    }
    FS_CloseFile(file);
    return model;
}

LPCANIMATION SV_GetAnimation(DWORD modelindex, LPCSTR animname) {
    struct cmodel *model = sv.models[modelindex];
    if (sv.state == ss_loading) {
        return NULL;
    }
    if (!model) {
        if (modelindex >= MAX_MODELS || !*sv.configstrings[CS_MODELS + modelindex]) {
            return NULL;
        }
        model = sv.models[modelindex] = SV_LoadModel(sv.configstrings[CS_MODELS + modelindex]);
        if (!model) {
            return NULL;
        }
    }
    DWORD hash = fnv1a32(animname);
    FOR_LOOP(i, model->num_animations) {
        LPANIMATION anim = model->animations+i;
        if (anim->syncpoint == hash) {
            return anim;
        }
    }
    FOR_LOOP(i, model->num_animations) {
        LPANIMATION anim = model->animations+i;
        if (!strcasecmp(anim->name, animname)) {
            return anim;
        }
    }
    return NULL;
}



VECTOR2 get_flow_direction(DWORD heatmapindex, float fx, float fy);

void PF_Sleep(DWORD msec) {
    usleep(msec * 1000);
}

void SV_InitGameProgs(void) {
    struct game_import import;
    
    import.multicast = PF_Multicast;
    import.unicast = PF_Unicast;
        
    import.MemAlloc = MemAlloc;
    import.MemFree = MemFree;
    import.ModelIndex = SV_ModelIndex;
    import.ImageIndex = SV_ImageIndex;
    import.SoundIndex = SV_SoundIndex;
    import.FontIndex = SV_FontIndex;
    import.ReadSheet = FS_ParseSLK;
    import.ReadConfig = FS_ParseINI;
    import.FindSheetCell = FS_FindSheetCell;
    import.GetTime = SV_GetTime;
    import.GetAnimation = SV_GetAnimation;
    import.GetHeightAtPoint = CM_GetHeightAtPoint;
    import.BuildHeatmap = CM_BuildHeatmap;
    import.ClosestPathablePoint = CM_ClosestPathablePoint;
    import.ClosestPathablePointForRadius = CM_ClosestPathablePointForRadius;
    import.GetFlowDirection = get_flow_direction;
    import.ReadFile = FS_ReadFile;
    import.error = PF_error;
    import.LinkEntity = SV_LinkEntity;
    import.UnlinkEntity = SV_UnlinkEntity;
    import.BoxEdicts = SV_AreaEdicts;
    import.MenuAction = MenuAction;
    import.configstring = PF_Configstring;
    import.confignstring = PF_Confignstring;
    import.WriteByte = PF_WriteByte;
    import.WriteShort = PF_WriteShort;
    import.WriteLong = PF_WriteLong;
    import.WriteFloat = PF_WriteFloat;
    import.WriteString = PF_WriteString;
    import.WritePosition = PF_WritePos;
    import.WriteDirection = PF_WriteDir;
    import.WriteAngle = PF_WriteAngle;
    import.WriteEntity = PF_WriteEntity;
    import.WriteUIFrame = PF_WriteUIFrame;
    import.CvarString = Cvar_String;

    ge = GetGameAPI(&import);
    ge->Init();
}
