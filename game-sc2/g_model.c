#include "g_sc2_local.h"
#include <ctype.h>

static DWORD fnv1a32(LPCSTR str) {
    DWORD prime = 16777619;
    DWORD hash  = 2166136261;
    while (*str) {
        hash = (hash ^ *str++) * prime;
    }
    return hash;
}

static void ConvertMD34AnimationName(LPANIMATION seq) {
    char buffer[80];
    memset(buffer, 0, sizeof(buffer));
    strncpy(buffer, seq->name, sizeof(buffer) - 1);
    for (DWORD i = 0; buffer[i]; i++)
        buffer[i] = (char)tolower(buffer[i]);
    for (DWORD i = (DWORD)strlen(buffer) - 1; i > 0 && isspace(buffer[i]); i--)
        buffer[i] = '\0';
    seq->syncpoint = fnv1a32(buffer);
}

/* ---- MD34 (StarCraft II M3) ---- */

typedef struct {
    DWORD nEntries;
    DWORD offset;
    DWORD flags;
} md34Reference_t;

struct md33Header {
    DWORD ofsRefs;
    DWORD nRefs;
    md34Reference_t MODL;
};

struct md34ReferenceEntry {
    DWORD id;
    DWORD offset;
    DWORD nEntries;
    DWORD version;
};

struct md34NameRef {
    DWORD nEntries;
    DWORD ref;
    DWORD flags;
};

struct md34Sequence {
    DWORD unknown[2];
    struct md34NameRef name;
    DWORD interval[2];
    float movementSpeed;
    DWORD flags;
    DWORD frequency;
    LONG unk[3];
    LONG unk2;
    struct { VECTOR3 min; VECTOR3 max; float radius; } boundingSphere;
    LONG d5[3];
};

static void *ReadEntry(HANDLE file, DWORD offset, DWORD size) {
    void *data = gi.MemAlloc(size);
    SFileSetFilePointer(file, offset, NULL, FILE_BEGIN);
    SFileReadFile(file, data, size, NULL, NULL);
    return data;
}

static int compare_animation_name(const void *a, const void *b) {
    return strcmp(((LPCANIMATION)a)->name, ((LPCANIMATION)b)->name);
}

static animation_t *LoadModelMD34(HANDLE file, DWORD *out_count) {
    struct md33Header hdr;
    SFileReadFile(file, &hdr, sizeof(hdr), NULL, NULL);

    struct md34ReferenceEntry *ent = ReadEntry(file, hdr.ofsRefs,
        sizeof(struct md34ReferenceEntry) * hdr.nRefs);

    animation_t *animations = NULL;
    DWORD num = 0;

    FOR_LOOP(i, hdr.nRefs) {
        struct md34ReferenceEntry const *re = ent + i;
        if (re->id != MAKEFOURCC('S','Q','E','S'))
            continue;
        struct md34Sequence *seq = ReadEntry(file, re->offset,
            re->nEntries * sizeof(struct md34Sequence));
        animations = gi.MemAlloc(sizeof(animation_t) * re->nEntries);
        num = re->nEntries;
        DWORD startanim = 0;
        FOR_LOOP(j, re->nEntries) {
            struct md34Sequence *src = seq + j;
            char *name = ReadEntry(file, ent[src->name.ref].offset, src->name.nEntries);
            LPANIMATION dest = animations + j;
            strncpy(dest->name, name, sizeof(dest->name) - 1);
            dest->interval[0] = startanim + src->interval[0];
            dest->interval[1] = startanim + src->interval[1];
            startanim += src->interval[1];
            gi.MemFree(name);
        }
        qsort(animations, num, sizeof(animation_t), compare_animation_name);
        FOR_LOOP(j, num) {
            ConvertMD34AnimationName(animations + j);
        }
        gi.MemFree(seq);
        break;
    }
    gi.MemFree(ent);
    *out_count = num;
    return animations;
}

/* ---- model cache ---- */

#define G_MAX_MODELS MAX_MODELS

typedef struct {
    animation_t *animations;
    DWORD        num_animations;
    char         filename[MAX_PATHLEN];
} g_cmodel_t;

static g_cmodel_t g_models[G_MAX_MODELS];

int G_RegisterModel(LPCSTR filename) {
    int index = gi.ModelIndex(filename);
    if (index > 0 && index < G_MAX_MODELS && !g_models[index].filename[0])
        strncpy(g_models[index].filename, filename, MAX_PATHLEN - 1);
    return index;
}

static g_cmodel_t *LoadModel(LPCSTR filename) {
    DWORD fileheader;
    HANDLE file = FS_OpenFile(filename);
    if (!file) {
        /* Try .m3x variant */
        PATHSTR path;
        strcpy(path, filename);
        LPSTR ext = strstr(path, ".m3");
        if (ext && ext[3] == '\0') {
            strcpy(ext, ".m3x");
            file = FS_OpenFile(path);
        }
        if (!file)
            return NULL;
    }

    g_cmodel_t *model = gi.MemAlloc(sizeof(g_cmodel_t));
    memset(model, 0, sizeof(*model));

    SFileReadFile(file, &fileheader, 4, NULL, NULL);
    if (fileheader == ID_43DM)
        model->animations = LoadModelMD34(file, &model->num_animations);

    FS_CloseFile(file);
    return model;
}

static g_cmodel_t *GetModel(DWORD modelindex) {
    if (modelindex == 0 || modelindex >= G_MAX_MODELS)
        return NULL;
    g_cmodel_t *entry = &g_models[modelindex];
    if (!entry->animations && entry->filename[0]) {
        g_cmodel_t *m = LoadModel(entry->filename);
        if (!m)
            return NULL;
        entry->animations     = m->animations;
        entry->num_animations = m->num_animations;
        gi.MemFree(m);
    }
    return entry->animations ? entry : NULL;
}

LPCANIMATION G_GetAnimation(DWORD modelindex, LPCSTR animname) {
    g_cmodel_t *model = GetModel(modelindex);
    if (!model)
        return NULL;
    DWORD hash = fnv1a32(animname);
    FOR_LOOP(i, model->num_animations) {
        if (model->animations[i].syncpoint == hash)
            return &model->animations[i];
    }
    FOR_LOOP(i, model->num_animations) {
        if (!strcasecmp(model->animations[i].name, animname))
            return &model->animations[i];
    }
    return NULL;
}

void G_FreeModels(void) {
    FOR_LOOP(i, G_MAX_MODELS) {
        if (g_models[i].animations)
            gi.MemFree(g_models[i].animations);
        memset(&g_models[i], 0, sizeof(g_models[i]));
    }
}
