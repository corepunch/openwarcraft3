#include <stdarg.h>
#include <pthread.h>

#include "server.h"

void PF_WriteByte(int c) { MSG_WriteByte(&sv.multicast, c); }
void PF_WriteShort(int c) { MSG_WriteShort(&sv.multicast, c); }
void PF_WriteLong(int c) { MSG_WriteLong(&sv.multicast, c); }
void PF_WriteFloat(float f) { MSG_WriteFloat(&sv.multicast, f); }
void PF_WriteString(LPCSTR s) { MSG_WriteString(&sv.multicast, s); }
void PF_WritePos(LPCVECTOR3 pos) { MSG_WritePos(&sv.multicast, pos); }
void PF_WriteDir(LPCVECTOR3 dir) { MSG_WriteDir(&sv.multicast, dir); }
void PF_WriteAngle(float f) { MSG_WriteAngle(&sv.multicast, f); }

void PF_TextRemoveComments(LPSTR buffer) {
    BOOL in_single_line_comment = false;
    BOOL in_block_comment = false;
    DWORD num_quotes = 0;
    char *src = buffer;
    char *dest = buffer;
    while (*src != '\0') {
        if (!in_single_line_comment && !in_block_comment) {
            if (*src == '"') {
                num_quotes++;
                *dest++ = *src++;
            } else if (num_quotes&1) {
                *dest++ = *src++;
            } else if (*src == '/' && *(src + 1) == '/') {
                in_single_line_comment = true;
                src += 2;
            } else if (*src == '/' && *(src + 1) == '*') {
                in_block_comment = true;
                src += 2;
            } else {
                *dest++ = *src++;
            }
        } else if (in_single_line_comment && *src == '\n') {
            in_single_line_comment = false;
            *dest++ = *src++;
        } else if (in_block_comment && *src == '*' && *(src + 1) == '/') {
            in_block_comment = false;
            src += 2;
        } else {
            src++;
        }
    }
    *dest = '\0';  // Null-terminate the modified buffer
}

BOMStatus PF_TextRemoveBom(LPSTR buffer) {
    unsigned char utf8BOM[] = { 0xEF, 0xBB, 0xBF };
    unsigned char utf16LEBOM[] = { 0xFF, 0xFE };
    unsigned char utf16BEBOM[] = { 0xFE, 0xFF };
    size_t length = strlen(buffer);

    if (length >= 3 && memcmp(buffer, utf8BOM, 3) == 0) {
        memmove(buffer, buffer + 3, length - 3);
        return UTF8_BOM_FOUND;
    } else if (length >= 2 && memcmp(buffer, utf16LEBOM, 2) == 0) {
        memmove(buffer, buffer + 2, length - 2);
        return UTF16LE_BOM_FOUND;
    } else if (length >= 2 && memcmp(buffer, utf16BEBOM, 2) == 0) {
        memmove(buffer, buffer + 2, length - 2);
        return UTF16BE_BOM_FOUND;
    } else {
        return NO_BOM;
    }
}

void PF_WriteEntity(LPCENTITYSTATE ent) {
    entityState_t empty;
    memset(&empty, 0, sizeof(entityState_t));
    MSG_WriteDeltaEntity(&sv.multicast, &empty, ent, true);
}

void PF_WriteUIFrame(LPCUIFRAME frame) {
    uiFrame_t empty;
    memset(&empty, 0, sizeof(uiFrame_t));
    empty.tex.coord[1] = 0xff;
    empty.tex.coord[3] = 0xff;
    MSG_WriteDeltaUIFrame(&sv.multicast, &empty, frame, true);
    MSG_WriteByte(&sv.multicast, frame->buffer.size);
    MSG_Write(&sv.multicast, frame->buffer.data, frame->buffer.size);
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

void PF_Unicast(edict_t *ent) {
    if (!ent)
        return;
    DWORD p = NUM_FOR_EDICT(ent);
    if (p < 1 || p > ge->max_clients)
        return;
    LPCLIENT client = svs.clients + (p-1);
    SZ_Write(&client->netchan.message, sv.multicast.data, sv.multicast.cursize);
    SZ_Clear(&sv.multicast);
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
        if (isnumber(*ch) || *ch == '-') {
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

struct cmodel *SV_LoadModel(LPCSTR filename) {
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

LPCANIMATION SV_GetAnimation(DWORD modelindex, LPCSTR animname) {
    struct cmodel *model = sv.models[modelindex];
    if (!model)
        return NULL;
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
struct thread {
    pthread_t thread;
    BOOL used;
};

struct thread threads[NUM_THREADS] = { 0 };

DWORD PF_CreateThread(HANDLE (func)(HANDLE), HANDLE args) {
    FOR_LOOP(i, NUM_THREADS) {
        if (!threads[i].used) {
            if (pthread_create(&threads[i].thread, NULL, func, args) != 0) {
                fprintf(stderr, "Error creating thread\n");
                exit(EXIT_FAILURE);
            } else {
                return i;
            }
        }
    }
    return -1;
}

void PF_JoinThread(DWORD thread) {
    if (pthread_join(threads[thread].thread, NULL) != 0) {
        fprintf(stderr, "Error joining thread %d\n", thread);
        exit(EXIT_FAILURE);
    }
}

void PF_Sleep(DWORD msec) {
    usleep(msec * 1000);
}

void SV_InitGameProgs(void) {
    struct game_import import;
    
    import.multicast = SV_Multicast;
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
    import.GetFlowDirection = get_flow_direction;
    import.ReadFileIntoString = FS_ReadFileIntoString;
    import.ReadFile = FS_ReadFile;
    import.ExtractFile = FS_ExtractFile;
    import.error = PF_error;
    import.LinkEntity = SV_LinkEntity;
    import.UnlinkEntity = SV_UnlinkEntity;
    import.BoxEdicts = SV_AreaEdicts;
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
    import.CreateThread = PF_CreateThread;
    import.JoinThread = PF_JoinThread;
    import.Sleep = PF_Sleep;
    import.TextRemoveComments = PF_TextRemoveComments;
    import.TextRemoveBom = PF_TextRemoveBom;

    ge = GetGameAPI(&import);
    ge->Init();
}
