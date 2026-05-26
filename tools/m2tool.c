#include "tools/tool_common.h"

#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

typedef struct {
    int32_t count;
    int32_t offset;
} m2Array_t;

typedef struct {
    DWORD magic;
    DWORD version;
    m2Array_t name;
    DWORD flags;
    m2Array_t global_sequences;
    m2Array_t animations;
    m2Array_t animation_lookup;
    m2Array_t playable_animation_lookup;
    m2Array_t bones;
    m2Array_t key_bone_lookup;
    m2Array_t vertices;
    m2Array_t views;
    m2Array_t colors;
    m2Array_t textures;
    m2Array_t transparency_lookup;
    m2Array_t texture_flipbooks;
    m2Array_t texture_animations;
    m2Array_t color_replacements;
    m2Array_t render_flags;
    m2Array_t bone_lookup_table;
    m2Array_t texture_lookup_table;
    m2Array_t texture_units;
    m2Array_t transparency_lookup_table;
    m2Array_t texture_animation_lookup_table;
    VECTOR3 bounding_box_min;
    VECTOR3 bounding_box_max;
    float bounding_sphere_radius;
    VECTOR3 collision_box_min;
    VECTOR3 collision_box_max;
    float collision_sphere_radius;
    m2Array_t bounding_triangles;
    m2Array_t bounding_vertices;
    m2Array_t bounding_normals;
    m2Array_t attachments;
    m2Array_t attachment_lookup;
    m2Array_t events;
    m2Array_t lights;
    m2Array_t cameras;
    m2Array_t camera_lookup;
    m2Array_t ribbon_emitters;
    m2Array_t particle_emitters;
} m2HeaderInfo_t;

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
} m2SequenceClassic_t;

typedef struct {
    WORD animation_id;
    WORD sub_animation_id;
    DWORD length;
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
} m2SequenceModern_t;

typedef struct {
    uint16_t track_type;
    uint16_t loop_index;
    m2Array_t sequence_times;
    m2Array_t sequence_keys;
} m2Track_t;

typedef struct {
    m2Array_t times;
} m2SequenceTimes_t;

typedef struct {
    m2Array_t keys;
} m2SequenceKeys_t;

typedef struct {
    uint16_t track_type;
    uint16_t loop_index;
    m2Array_t ranges;
    m2Array_t times;
    m2Array_t keys;
} m2TrackClassic_t;

typedef struct {
    WORD end;
    WORD start;
} m2Range_t;

typedef struct {
    DWORD auCompQ[2];
} m2CompQuat_t;

typedef struct {
    DWORD bone_id;
    DWORD flags;
    WORD parent_index;
    WORD dist_to_parent;
    DWORD union_data;
    m2Track_t translation_track;
    m2Track_t rotation_track;
    m2Track_t scale_track;
    VECTOR3 pivot;
} m2CompBoneModern_t;

typedef struct {
    DWORD bone_id;
    DWORD flags;
    WORD parent_index;
    WORD submesh_id;
    m2TrackClassic_t translation_track;
    m2TrackClassic_t rotation_track;
    m2TrackClassic_t scale_track;
    VECTOR3 pivot;
} m2CompBoneClassic_t;

typedef struct {
    VECTOR3 pos;
    BYTE bone_weights[4];
    BYTE bone_indices[4];
    VECTOR3 normal;
    VECTOR2 tex_coords[2];
} m2Vertex_t;

typedef struct {
    DWORD type;
    DWORD flags;
    m2Array_t filename;
} m2Texture_t;

typedef struct {
    DWORD magic;
    m2Array_t vertices;
    m2Array_t indices;
    m2Array_t bones;
    m2Array_t sections;
    m2Array_t batches;
    DWORD bone_count_max;
} m2SkinHeader_t;

typedef struct {
    m2Array_t vertices;
    m2Array_t indices;
    m2Array_t bones;
    m2Array_t sections;
    m2Array_t batches;
    DWORD bone_count_max;
} m2EmbeddedView_t;

static HANDLE archives[64] = { 0 };
static LPCSTR g_model_path = NULL;
static LPCSTR g_skin_path = NULL;
static LPCSTR g_anim_check = NULL;
static bool g_dump_all = false;

static void usage(void) {
    fprintf(stderr,
            "Usage:\n"
            "  m2tool -mpq <archive.mpq> -model <file.m2|file.mdx> [--skin <file00.skin>] [--info] [--dump-all] [--anim <name|index>]\n"
            "\n"
            "Examples:\n"
            "  m2tool -mpq data/world-of-warcraft/installed/Data/model.MPQ -model \"Character\\\\Orc\\\\Male\\\\OrcMale.m2\"\n"
            "  m2tool -mpq data/world-of-warcraft/installed/Data/model.MPQ -model \"Character\\\\Orc\\\\Male\\\\OrcMale.m2\" --dump-all\n"
            "\n"
            "Notes:\n"
            "  --info is the default; it prints header arrays, bounds, vertex bounds, textures, and skin counts.\n"
            "  --dump-all additionally prints animation rows and texture rows.\n"
            "  --anim prints per-sequence bone track diagnostics for a named or numeric animation.\n");
}

static void errorf(LPCSTR fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);
    exit(1);
}

static BOOL PathHasExtension(LPCSTR path, LPCSTR extension) {
    size_t path_len;
    size_t ext_len;

    if (!path || !extension) {
        return false;
    }
    path_len = strlen(path);
    ext_len = strlen(extension);
    return path_len >= ext_len && strcasecmp(path + path_len - ext_len, extension) == 0;
}

static BOOL CopyWithExtension(LPCSTR path, LPCSTR extension, LPSTR out, DWORD out_size) {
    LPCSTR dot;
    size_t stem_len;

    if (!path || !extension || !out || out_size == 0) {
        return false;
    }
    dot = strrchr(path, '.');
    stem_len = dot ? (size_t)(dot - path) : strlen(path);
    if (stem_len + strlen(extension) + 1 > out_size) {
        return false;
    }
    memcpy(out, path, stem_len);
    snprintf(out + stem_len, out_size - stem_len, "%s", extension);
    return true;
}

static HANDLE OpenFileFallback(LPCSTR path, LPSTR resolved, DWORD resolved_size) {
    HANDLE file;
    PATHSTR fallback;

    if (!path || !*path) {
        return NULL;
    }
    file = Tool_OpenFile(archives, sizeof(archives) / sizeof(archives[0]), path);
    if (file) {
        snprintf(resolved, resolved_size, "%s", path);
        return file;
    }
    if (PathHasExtension(path, ".mdx") && CopyWithExtension(path, ".m2", fallback, sizeof(fallback))) {
        file = Tool_OpenFile(archives, sizeof(archives) / sizeof(archives[0]), fallback);
        if (file) {
            snprintf(resolved, resolved_size, "%s", fallback);
            return file;
        }
    }
    if (PathHasExtension(path, ".m2") && CopyWithExtension(path, ".mdx", fallback, sizeof(fallback))) {
        file = Tool_OpenFile(archives, sizeof(archives) / sizeof(archives[0]), fallback);
        if (file) {
            snprintf(resolved, resolved_size, "%s", fallback);
            return file;
        }
    }
    return NULL;
}

static LPBYTE ReadWholeFile(LPCSTR path, LPDWORD out_size, LPSTR resolved, DWORD resolved_size) {
    HANDLE file = OpenFileFallback(path, resolved, resolved_size);
    DWORD size;
    DWORD read_size = 0;
    LPBYTE data;

    if (!file) {
        return NULL;
    }
    size = SFileGetFileSize(file, NULL);
    data = Tool_MemAlloc(size ? size : 1);
    SFileSetFilePointer(file, 0, NULL, FILE_BEGIN);
    if (!SFileReadFile(file, data, size, &read_size, NULL) || read_size != size) {
        Tool_CloseFile(file);
        Tool_MemFree(data);
        return NULL;
    }
    Tool_CloseFile(file);
    *out_size = size;
    return data;
}

static BOOL TagEquals(BYTE const *tag, DWORD fourcc) {
    DWORD value;
    memcpy(&value, tag, sizeof(value));
    return value == fourcc;
}

static BOOL FindM2Payload(BYTE const *data, DWORD size, BYTE const **payload, DWORD *payload_size) {
    DWORD magic;
    DWORD offset;

    if (!data || size < sizeof(DWORD) || !payload || !payload_size) {
        return false;
    }
    memcpy(&magic, data, sizeof(magic));
    if (magic == ID_MD20) {
        *payload = data;
        *payload_size = size;
        return true;
    }
    if (magic != ID_MD21 && magic != ID_12DM) {
        return false;
    }
    offset = 0;
    while (offset + 8 <= size) {
        BYTE const *tag = data + offset;
        DWORD chunk_size;
        memcpy(&chunk_size, data + offset + 4, sizeof(chunk_size));
        offset += 8;
        if (chunk_size > size - offset) {
            return false;
        }
        if (TagEquals(tag, ID_MD20) ||
            TagEquals(tag, ID_MD21) ||
            TagEquals(tag, ID_12DM)) {
            if (chunk_size >= sizeof(DWORD)) {
                DWORD inner_magic;
                memcpy(&inner_magic, data + offset, sizeof(inner_magic));
                if (inner_magic == ID_MD20) {
                    *payload = data + offset;
                    *payload_size = chunk_size;
                    return true;
                }
            }
        }
        offset += chunk_size;
    }
    return false;
}

static DWORD ReadU32(BYTE const *p) {
    DWORD value;
    memcpy(&value, p, sizeof(value));
    return value;
}

static float ReadFloat(BYTE const *p) {
    float value;
    memcpy(&value, p, sizeof(value));
    return value;
}

static m2Array_t ReadArray(BYTE const *p) {
    m2Array_t array;
    memcpy(&array, p, sizeof(array));
    return array;
}

static VECTOR3 ReadVec3(BYTE const *p) {
    return (VECTOR3){ ReadFloat(p), ReadFloat(p + 4), ReadFloat(p + 8) };
}

static void SkipArray(DWORD *offset) {
    *offset += sizeof(m2Array_t);
}

static m2Array_t NextArray(BYTE const *data, DWORD *offset) {
    m2Array_t value = ReadArray(data + *offset);
    *offset += sizeof(m2Array_t);
    return value;
}

static BOOL ParseHeader(BYTE const *data, DWORD size, m2HeaderInfo_t *out) {
    DWORD offset;
    BOOL legacy;

    if (!data || size < 180 || !out || ReadU32(data) != ID_MD20) {
        return false;
    }
    memset(out, 0, sizeof(*out));
    out->magic = ReadU32(data);
    out->version = ReadU32(data + 4);
    legacy = out->version <= 263;

    offset = 8;
    out->name = NextArray(data, &offset);
    out->flags = ReadU32(data + offset);
    offset += sizeof(DWORD);
    out->global_sequences = NextArray(data, &offset);
    out->animations = NextArray(data, &offset);
    out->animation_lookup = NextArray(data, &offset);
    if (legacy) {
        out->playable_animation_lookup = NextArray(data, &offset);
    }
    out->bones = NextArray(data, &offset);
    out->key_bone_lookup = NextArray(data, &offset);
    out->vertices = NextArray(data, &offset);
    if (legacy) {
        out->views = NextArray(data, &offset);
    } else {
        out->views = (m2Array_t){ (int32_t)ReadU32(data + offset), 0 };
        offset += sizeof(DWORD);
    }
    out->colors = NextArray(data, &offset);
    out->textures = NextArray(data, &offset);
    out->transparency_lookup = NextArray(data, &offset);
    if (legacy) {
        out->texture_flipbooks = NextArray(data, &offset);
    }
    out->texture_animations = NextArray(data, &offset);
    out->color_replacements = NextArray(data, &offset);
    out->render_flags = NextArray(data, &offset);
    out->bone_lookup_table = NextArray(data, &offset);
    out->texture_lookup_table = NextArray(data, &offset);
    out->texture_units = NextArray(data, &offset);
    out->transparency_lookup_table = NextArray(data, &offset);
    out->texture_animation_lookup_table = NextArray(data, &offset);

    if (offset + 7 * sizeof(float) > size) {
        return false;
    }
    out->bounding_box_min = ReadVec3(data + offset);
    offset += 12;
    out->bounding_box_max = ReadVec3(data + offset);
    offset += 12;
    out->bounding_sphere_radius = ReadFloat(data + offset);
    offset += 4;
    out->collision_box_min = ReadVec3(data + offset);
    offset += 12;
    out->collision_box_max = ReadVec3(data + offset);
    offset += 12;
    out->collision_sphere_radius = ReadFloat(data + offset);
    offset += 4;

    if (offset + 3 * sizeof(m2Array_t) <= size) {
        out->bounding_triangles = NextArray(data, &offset);
        out->bounding_vertices = NextArray(data, &offset);
        out->bounding_normals = NextArray(data, &offset);
    }
    if (offset + 8 * sizeof(m2Array_t) <= size) {
        out->attachments = NextArray(data, &offset);
        out->attachment_lookup = NextArray(data, &offset);
        out->events = NextArray(data, &offset);
        out->lights = NextArray(data, &offset);
        out->cameras = NextArray(data, &offset);
        out->camera_lookup = NextArray(data, &offset);
        out->ribbon_emitters = NextArray(data, &offset);
        out->particle_emitters = NextArray(data, &offset);
    }
    (void)SkipArray;
    return true;
}

static BOOL ArrayRange(m2Array_t array, DWORD elem_size, DWORD file_size, DWORD *offset, DWORD *bytes) {
    if (array.count <= 0 || array.offset < 0 || elem_size == 0) {
        return false;
    }
    if ((DWORD)array.count > (((DWORD)~0u) / elem_size)) {
        return false;
    }
    *offset = (DWORD)array.offset;
    *bytes = (DWORD)array.count * elem_size;
    return *offset <= file_size && *bytes <= file_size - *offset;
}

static void const *ArrayPtr(BYTE const *base, DWORD file_size, m2Array_t array, DWORD elem_size) {
    DWORD offset;
    DWORD bytes;
    if (!ArrayRange(array, elem_size, file_size, &offset, &bytes)) {
        return NULL;
    }
    return base + offset;
}

static LPCSTR StringPtr(BYTE const *base, DWORD file_size, m2Array_t array) {
    DWORD offset;
    DWORD bytes;
    if (!ArrayRange(array, 1, file_size, &offset, &bytes) || bytes == 0) {
        return NULL;
    }
    if (!memchr(base + offset, '\0', bytes)) {
        return NULL;
    }
    return (LPCSTR)(base + offset);
}

static void PrintVec3(LPCSTR label, VECTOR3 v) {
    printf("  %-22s %.6f %.6f %.6f\n", label, v.x, v.y, v.z);
}

static void PrintBoundsMetrics(LPCSTR label, VECTOR3 min, VECTOR3 max) {
    VECTOR3 extent = { max.x - min.x, max.y - min.y, max.z - min.z };
    VECTOR3 center = {
        (min.x + max.x) * 0.5f,
        (min.y + max.y) * 0.5f,
        (min.z + max.z) * 0.5f,
    };
    char extent_label[64];
    char center_label[64];

    snprintf(extent_label, sizeof(extent_label), "%s.extent", label);
    snprintf(center_label, sizeof(center_label), "%s.center", label);
    printf("  %-22s %.6f %.6f %.6f\n", extent_label, extent.x, extent.y, extent.z);
    printf("  %-22s %.6f %.6f %.6f\n", center_label, center.x, center.y, center.z);
}

static void PrintArray(LPCSTR label, m2Array_t array, BOOL count_only) {
    if (count_only) {
        printf("  %-28s count=%d\n", label, array.count);
    } else {
        printf("  %-28s count=%d offset=%d\n", label, array.count, array.offset);
    }
}

static void UpdateBounds(BOX3 *bounds, VECTOR3 p, BOOL *has_bounds) {
    if (!*has_bounds) {
        bounds->min = p;
        bounds->max = p;
        *has_bounds = true;
        return;
    }
    bounds->min.x = MIN(bounds->min.x, p.x);
    bounds->min.y = MIN(bounds->min.y, p.y);
    bounds->min.z = MIN(bounds->min.z, p.z);
    bounds->max.x = MAX(bounds->max.x, p.x);
    bounds->max.y = MAX(bounds->max.y, p.y);
    bounds->max.z = MAX(bounds->max.z, p.z);
}

static BOOL CalculateVertexBounds(BYTE const *data, DWORD size, m2Array_t vertices, BOX3 *bounds) {
    m2Vertex_t const *items = ArrayPtr(data, size, vertices, sizeof(*items));
    BOOL has_bounds = false;

    if (!items || !bounds) {
        return false;
    }
    FOR_LOOP(i, (DWORD)vertices.count) {
        UpdateBounds(bounds, items[i].pos, &has_bounds);
    }
    return has_bounds;
}

static LPCSTR AnimationName(WORD id) {
    switch (id) {
        case 0: return "Stand";
        case 1: return "Death";
        case 2: return "Spell";
        case 3: return "Stop";
        case 4: return "Walk";
        case 5: return "Run";
        case 6: return "Dead";
        case 7: return "Rise";
        case 8: return "StandWound";
        case 9: return "CombatWound";
        case 10: return "CombatCritical";
        case 11: return "ShuffleLeft";
        case 12: return "ShuffleRight";
        case 13: return "WalkBackwards";
        case 14: return "Stun";
        case 15: return "HandsClosed";
        case 16: return "AttackUnarmed";
        case 17: return "Attack1H";
        case 18: return "Attack2H";
        case 19: return "Attack2HL";
        case 20: return "ParryUnarmed";
        case 21: return "Parry1H";
        case 22: return "Parry2H";
        case 23: return "Parry2HL";
        case 24: return "ShieldBlock";
        case 25: return "ReadyUnarmed";
        case 26: return "Ready1H";
        case 27: return "Ready2H";
        case 28: return "Ready2HL";
        case 29: return "ReadyBow";
        case 30: return "Dodge";
        case 37: return "JumpStart";
        case 38: return "Jump";
        case 39: return "JumpEnd";
        case 40: return "Fall";
        case 41: return "SwimIdle";
        case 42: return "Swim";
        default: return NULL;
    }
}

static void PrintAnimations(BYTE const *data, DWORD size, m2HeaderInfo_t const *header) {
    DWORD offset;
    DWORD bytes;

    if (!ArrayRange(header->animations,
                    header->version <= 263 ? sizeof(m2SequenceClassic_t) : sizeof(m2SequenceModern_t),
                    size,
                    &offset,
                    &bytes)) {
        printf("animations: unavailable\n");
        return;
    }
    printf("animations:\n");
    FOR_LOOP(i, (DWORD)header->animations.count) {
        if (header->version <= 263) {
            m2SequenceClassic_t const *seq = (m2SequenceClassic_t const *)(data + offset + i * sizeof(*seq));
            LPCSTR name = AnimationName(seq->animation_id);
            printf("  [%03u] id=%u%s%s sub=%u start=%u end=%u speed=%.3f flags=0x%08x blend=%u radius=%.3f next=%d alias=%u bounds=(%.3f %.3f %.3f)..(%.3f %.3f %.3f)\n",
                   (unsigned)i,
                   (unsigned)seq->animation_id,
                   name ? " " : "",
                   name ? name : "",
                   (unsigned)seq->sub_animation_id,
                   (unsigned)seq->start_timestamp,
                   (unsigned)seq->end_timestamp,
                   seq->movement_speed,
                   (unsigned)seq->flags,
                   (unsigned)seq->blend_time,
                   seq->radius,
                   (int)seq->next_animation,
                   (unsigned)seq->alias_next,
                   seq->min.x, seq->min.y, seq->min.z,
                   seq->max.x, seq->max.y, seq->max.z);
        } else {
            m2SequenceModern_t const *seq = (m2SequenceModern_t const *)(data + offset + i * sizeof(*seq));
            LPCSTR name = AnimationName(seq->animation_id);
            printf("  [%03u] id=%u%s%s sub=%u length=%u speed=%.3f flags=0x%08x blend=%u radius=%.3f next=%d alias=%u bounds=(%.3f %.3f %.3f)..(%.3f %.3f %.3f)\n",
                   (unsigned)i,
                   (unsigned)seq->animation_id,
                   name ? " " : "",
                   name ? name : "",
                   (unsigned)seq->sub_animation_id,
                   (unsigned)seq->length,
                   seq->movement_speed,
                   (unsigned)seq->flags,
                   (unsigned)seq->blend_time,
                   seq->radius,
                   (int)seq->next_animation,
                   (unsigned)seq->alias_next,
                   seq->min.x, seq->min.y, seq->min.z,
                   seq->max.x, seq->max.y, seq->max.z);
        }
    }
}

static DWORD SequenceStart(BYTE const *sequence, BOOL classic) {
    return classic ? ((m2SequenceClassic_t const *)sequence)->start_timestamp : 0;
}

static DWORD SequenceDuration(BYTE const *sequence, BOOL classic) {
    if (classic) {
        m2SequenceClassic_t const *seq = (m2SequenceClassic_t const *)sequence;
        return seq->end_timestamp > seq->start_timestamp ? seq->end_timestamp - seq->start_timestamp : 0;
    }
    return ((m2SequenceModern_t const *)sequence)->length;
}

static WORD SequenceAnimationId(BYTE const *sequence, BOOL classic) {
    return classic
        ? ((m2SequenceClassic_t const *)sequence)->animation_id
        : ((m2SequenceModern_t const *)sequence)->animation_id;
}

static BOOL ParseSequenceSelector(LPCSTR selector, DWORD max_count, DWORD *out_index) {
    char *end = NULL;
    unsigned long value;

    if (!selector || !*selector || !out_index) {
        return false;
    }
    value = strtoul(selector, &end, 10);
    if (end && *end == '\0' && value < max_count) {
        *out_index = (DWORD)value;
        return true;
    }
    return false;
}

static BOOL FindSequenceByName(BYTE const *sequences,
                               DWORD sequence_count,
                               DWORD sequence_stride,
                               BOOL classic,
                               LPCSTR selector,
                               DWORD *out_index) {
    if (ParseSequenceSelector(selector, sequence_count, out_index)) {
        return true;
    }
    FOR_LOOP(i, sequence_count) {
        BYTE const *sequence = sequences + i * sequence_stride;
        LPCSTR name = AnimationName(SequenceAnimationId(sequence, classic));
        if (name && !strcasecmp(name, selector)) {
            *out_index = i;
            return true;
        }
    }
    return false;
}

static DWORD ClassicTrackKeyRange(BYTE const *data,
                                  DWORD size,
                                  m2TrackClassic_t const *track,
                                  DWORD sequence_index,
                                  DWORD elem_size,
                                  m2Range_t *out_range,
                                  DWORD const **out_times,
                                  BYTE const **out_keys) {
    m2Range_t const *ranges;
    m2Range_t range;
    DWORD const *times;
    BYTE const *keys;
    DWORD count;

    if (!track || !out_range || !out_times || !out_keys) {
        return 0;
    }
    ranges = ArrayPtr(data, size, track->ranges, sizeof(*ranges));
    if (!ranges || track->ranges.count <= 0) {
        return 0;
    }
    if (sequence_index >= (DWORD)track->ranges.count) {
        sequence_index = 0;
    }
    range = ranges[sequence_index];
    if (range.end < range.start) {
        return 0;
    }
    times = ArrayPtr(data, size, track->times, sizeof(*times));
    keys = ArrayPtr(data, size, track->keys, elem_size);
    if (!times || !keys ||
        range.start >= (WORD)track->times.count ||
        range.start >= (WORD)track->keys.count) {
        return 0;
    }
    count = (DWORD)range.end - (DWORD)range.start + 1;
    count = MIN(count, (DWORD)track->times.count - range.start);
    count = MIN(count, (DWORD)track->keys.count - range.start);
    if (count == 0) {
        return 0;
    }
    *out_range = range;
    *out_times = times + range.start;
    *out_keys = keys + (DWORD)range.start * elem_size;
    return count;
}

static DWORD ModernTrackKeyRange(BYTE const *data,
                                 DWORD size,
                                 m2Track_t const *track,
                                 DWORD sequence_index,
                                 DWORD elem_size,
                                 DWORD const **out_times,
                                 BYTE const **out_keys) {
    m2SequenceTimes_t const *sequence_times;
    m2SequenceKeys_t const *sequence_keys;
    DWORD const *times;
    BYTE const *keys;

    if (!track || !out_times || !out_keys) {
        return 0;
    }
    sequence_times = ArrayPtr(data, size, track->sequence_times, sizeof(*sequence_times));
    sequence_keys = ArrayPtr(data, size, track->sequence_keys, sizeof(*sequence_keys));
    if (!sequence_times || !sequence_keys ||
        track->sequence_times.count <= 0 ||
        track->sequence_keys.count <= 0) {
        return 0;
    }
    if (sequence_index >= (DWORD)track->sequence_times.count ||
        sequence_index >= (DWORD)track->sequence_keys.count) {
        sequence_index = 0;
    }
    times = ArrayPtr(data, size, sequence_times[sequence_index].times, sizeof(*times));
    keys = ArrayPtr(data, size, sequence_keys[sequence_index].keys, elem_size);
    if (!times || !keys) {
        return 0;
    }
    *out_times = times;
    *out_keys = keys;
    return MIN((DWORD)sequence_times[sequence_index].times.count,
               (DWORD)sequence_keys[sequence_index].keys.count);
}

static VECTOR3 KeyVec3(BYTE const *keys, DWORD index) {
    return *(VECTOR3 const *)(keys + index * sizeof(VECTOR3));
}

static void PrintTrackLine(LPCSTR label,
                           DWORD bone_index,
                           DWORD flags,
                           WORD parent_index,
                           VECTOR3 pivot,
                           DWORD count,
                           m2Range_t range,
                           DWORD const *times,
                           BYTE const *keys,
                           DWORD elem_size,
                           BOOL classic,
                           BOOL vector_keys) {
    printf("  bone[%03u] %-5s flags=0x%08x parent=%u pivot=(%.3f %.3f %.3f) keys=%u",
           (unsigned)bone_index,
           label,
           (unsigned)flags,
           (unsigned)parent_index,
           pivot.x, pivot.y, pivot.z,
           (unsigned)count);
    if (classic) {
        printf(" range=%u..%u", (unsigned)range.start, (unsigned)range.end);
    }
    if (count && times) {
        printf(" time=%u..%u", (unsigned)times[0], (unsigned)times[count - 1]);
    }
    if (count && keys) {
        if (vector_keys) {
            VECTOR3 first = KeyVec3(keys, 0);
            VECTOR3 mid = KeyVec3(keys, count / 2);
            VECTOR3 last = KeyVec3(keys, count - 1);
            printf(" first=(%.3f %.3f %.3f) mid=(%.3f %.3f %.3f) last=(%.3f %.3f %.3f)",
                   first.x, first.y, first.z,
                   mid.x, mid.y, mid.z,
                   last.x, last.y, last.z);
        } else if (elem_size == sizeof(QUATERNION)) {
            QUATERNION const *first = (QUATERNION const *)keys;
            QUATERNION const *mid = (QUATERNION const *)(keys + (count / 2) * elem_size);
            QUATERNION const *last = (QUATERNION const *)(keys + (count - 1) * elem_size);
            printf(" firstQuat=(%.6f %.6f %.6f %.6f) midQuat=(%.6f %.6f %.6f %.6f) lastQuat=(%.6f %.6f %.6f %.6f)",
                   first->x, first->y, first->z, first->w,
                   mid->x, mid->y, mid->z, mid->w,
                   last->x, last->y, last->z, last->w);
        } else if (elem_size == sizeof(m2CompQuat_t)) {
            m2CompQuat_t const *first = (m2CompQuat_t const *)keys;
            m2CompQuat_t const *last = (m2CompQuat_t const *)(keys + (count - 1) * elem_size);
            printf(" firstQuatRaw=(%08x %08x) lastQuatRaw=(%08x %08x)",
                   (unsigned)first->auCompQ[0],
                   (unsigned)first->auCompQ[1],
                   (unsigned)last->auCompQ[0],
                   (unsigned)last->auCompQ[1]);
        }
    }
    printf("\n");
}

static void PrintAnimationDiagnostics(BYTE const *data, DWORD size, m2HeaderInfo_t const *header, LPCSTR selector) {
    DWORD sequence_offset;
    DWORD sequence_bytes;
    BYTE const *sequences;
    BYTE const *sequence;
    DWORD sequence_stride;
    DWORD sequence_index;
    BOOL classic = header->version <= 263;
    DWORD bone_stride = classic ? sizeof(m2CompBoneClassic_t) : sizeof(m2CompBoneModern_t);
    BYTE const *bones;
    DWORD bones_offset;
    DWORD bones_bytes;
    DWORD sequence_count;
    DWORD trans_bones = 0;
    DWORD rot_bones = 0;
    DWORD scale_bones = 0;
    DWORD printed = 0;

    sequence_stride = classic ? sizeof(m2SequenceClassic_t) : sizeof(m2SequenceModern_t);
    if (!ArrayRange(header->animations, sequence_stride, size, &sequence_offset, &sequence_bytes)) {
        printf("anim_debug: animations unavailable\n");
        return;
    }
    if (!ArrayRange(header->bones, bone_stride, size, &bones_offset, &bones_bytes)) {
        printf("anim_debug: bones unavailable for stride=%u\n", (unsigned)bone_stride);
        return;
    }
    sequences = data + sequence_offset;
    bones = data + bones_offset;
    sequence_count = sequence_bytes / sequence_stride;
    if (!FindSequenceByName(sequences, sequence_count, sequence_stride, classic, selector, &sequence_index)) {
        printf("anim_debug: sequence '%s' not found\n", selector);
        return;
    }
    sequence = sequences + sequence_index * sequence_stride;
    printf("anim_debug: selector=%s seq=%u id=%u%s%s layout=%s start=%u duration=%u bones=%d bone_stride=%u\n",
           selector,
           (unsigned)sequence_index,
           (unsigned)SequenceAnimationId(sequence, classic),
           AnimationName(SequenceAnimationId(sequence, classic)) ? " " : "",
           AnimationName(SequenceAnimationId(sequence, classic)) ? AnimationName(SequenceAnimationId(sequence, classic)) : "",
           classic ? "classic" : "modern",
           (unsigned)SequenceStart(sequence, classic),
           (unsigned)SequenceDuration(sequence, classic),
           header->bones.count,
           (unsigned)bone_stride);

    FOR_LOOP(i, (DWORD)header->bones.count) {
        DWORD const *times = NULL;
        BYTE const *keys = NULL;
        DWORD trans_count = 0;
        DWORD rot_count = 0;
        DWORD scale_count = 0;
        m2Range_t trans_range = { 0, 0 };
        m2Range_t rot_range = { 0, 0 };
        m2Range_t scale_range = { 0, 0 };
        DWORD flags;
        WORD parent_index;
        VECTOR3 pivot;

        if (classic) {
            m2CompBoneClassic_t const *bone = (m2CompBoneClassic_t const *)(bones + i * bone_stride);
            flags = bone->flags;
            parent_index = bone->parent_index;
            pivot = bone->pivot;
            trans_count = ClassicTrackKeyRange(data, size, &bone->translation_track, sequence_index, sizeof(VECTOR3), &trans_range, &times, &keys);
            if (trans_count) {
                trans_bones++;
            }
            times = NULL;
            keys = NULL;
            rot_count = ClassicTrackKeyRange(data, size, &bone->rotation_track, sequence_index, sizeof(QUATERNION), &rot_range, &times, &keys);
            if (rot_count) {
                rot_bones++;
            }
            times = NULL;
            keys = NULL;
            scale_count = ClassicTrackKeyRange(data, size, &bone->scale_track, sequence_index, sizeof(VECTOR3), &scale_range, &times, &keys);
            if (scale_count) {
                scale_bones++;
            }
        } else {
            m2CompBoneModern_t const *bone = (m2CompBoneModern_t const *)(bones + i * bone_stride);
            flags = bone->flags;
            parent_index = bone->parent_index;
            pivot = bone->pivot;
            trans_count = ModernTrackKeyRange(data, size, &bone->translation_track, sequence_index, sizeof(VECTOR3), &times, &keys);
            if (trans_count) {
                trans_bones++;
            }
            times = NULL;
            keys = NULL;
            rot_count = ModernTrackKeyRange(data, size, &bone->rotation_track, sequence_index, sizeof(m2CompQuat_t), &times, &keys);
            if (rot_count) {
                rot_bones++;
            }
            times = NULL;
            keys = NULL;
            scale_count = ModernTrackKeyRange(data, size, &bone->scale_track, sequence_index, sizeof(VECTOR3), &times, &keys);
            if (scale_count) {
                scale_bones++;
            }
        }

        if (printed < 16 && (trans_count || rot_count || scale_count || i == 0 || i == 22)) {
            if (classic) {
                m2CompBoneClassic_t const *bone = (m2CompBoneClassic_t const *)(bones + i * bone_stride);
                if (trans_count) {
                    trans_count = ClassicTrackKeyRange(data, size, &bone->translation_track, sequence_index, sizeof(VECTOR3), &trans_range, &times, &keys);
                    PrintTrackLine("trans", i, flags, parent_index, pivot, trans_count, trans_range, times, keys, sizeof(VECTOR3), true, true);
                }
                if (rot_count) {
                    rot_count = ClassicTrackKeyRange(data, size, &bone->rotation_track, sequence_index, sizeof(QUATERNION), &rot_range, &times, &keys);
                    PrintTrackLine("rot", i, flags, parent_index, pivot, rot_count, rot_range, times, keys, sizeof(QUATERNION), true, false);
                }
                if (scale_count) {
                    scale_count = ClassicTrackKeyRange(data, size, &bone->scale_track, sequence_index, sizeof(VECTOR3), &scale_range, &times, &keys);
                    PrintTrackLine("scale", i, flags, parent_index, pivot, scale_count, scale_range, times, keys, sizeof(VECTOR3), true, true);
                }
            } else {
                m2CompBoneModern_t const *bone = (m2CompBoneModern_t const *)(bones + i * bone_stride);
                if (trans_count) {
                    trans_count = ModernTrackKeyRange(data, size, &bone->translation_track, sequence_index, sizeof(VECTOR3), &times, &keys);
                    PrintTrackLine("trans", i, flags, parent_index, pivot, trans_count, trans_range, times, keys, sizeof(VECTOR3), false, true);
                }
                if (rot_count) {
                    rot_count = ModernTrackKeyRange(data, size, &bone->rotation_track, sequence_index, sizeof(m2CompQuat_t), &times, &keys);
                    PrintTrackLine("rot", i, flags, parent_index, pivot, rot_count, rot_range, times, keys, sizeof(m2CompQuat_t), false, false);
                }
                if (scale_count) {
                    scale_count = ModernTrackKeyRange(data, size, &bone->scale_track, sequence_index, sizeof(VECTOR3), &times, &keys);
                    PrintTrackLine("scale", i, flags, parent_index, pivot, scale_count, scale_range, times, keys, sizeof(VECTOR3), false, true);
                }
            }
            if (!trans_count && !rot_count && !scale_count) {
                printf("  bone[%03u] none  flags=0x%08x parent=%u pivot=(%.3f %.3f %.3f) keys=0\n",
                       (unsigned)i,
                       (unsigned)flags,
                       (unsigned)parent_index,
                       pivot.x, pivot.y, pivot.z);
            }
            printed++;
        }
    }
    printf("anim_debug_summary: trans_bones=%u rot_bones=%u scale_bones=%u\n",
           (unsigned)trans_bones,
           (unsigned)rot_bones,
           (unsigned)scale_bones);
}

static LPCSTR TextureTypeName(DWORD type) {
    switch (type) {
        case 0: return "hardcoded";
        case 1: return "body/skin";
        case 2: return "object skin";
        case 6: return "hair/beard";
        case 8: return "tauren mane";
        case 11: return "skin extra";
        default: return "unknown";
    }
}

static void PrintTextures(BYTE const *data, DWORD size, m2HeaderInfo_t const *header) {
    m2Texture_t const *textures = ArrayPtr(data, size, header->textures, sizeof(*textures));

    if (!textures) {
        printf("textures: unavailable\n");
        return;
    }
    printf("textures:\n");
    FOR_LOOP(i, (DWORD)header->textures.count) {
        m2Texture_t const *texture = textures + i;
        LPCSTR path = StringPtr(data, size, texture->filename);
        printf("  [%03u] type=%u (%s) flags=0x%08x path=%s\n",
               (unsigned)i,
               (unsigned)texture->type,
               TextureTypeName(texture->type),
               (unsigned)texture->flags,
               path && *path ? path : "<replaceable>");
    }
}

static BOOL DefaultSkinPath(LPCSTR model_path, LPSTR out, DWORD out_size) {
    return CopyWithExtension(model_path, "00.skin", out, out_size);
}

static void PrintEmbeddedSkinInfo(BYTE const *data, DWORD size, m2HeaderInfo_t const *header) {
    m2EmbeddedView_t const *views;

    if (!header || header->version > 263) {
        return;
    }
    views = ArrayPtr(data, size, header->views, sizeof(*views));
    if (!views) {
        printf("embedded_skins: unavailable\n");
        return;
    }
    printf("embedded_skins: count=%d\n", header->views.count);
    FOR_LOOP(i, (DWORD)header->views.count) {
        m2EmbeddedView_t const *view = views + i;
        printf("  view[%u]\n", (unsigned)i);
        PrintArray("view.vertices", view->vertices, false);
        PrintArray("view.indices", view->indices, false);
        PrintArray("view.bones", view->bones, false);
        PrintArray("view.sections", view->sections, false);
        PrintArray("view.batches", view->batches, false);
        printf("  %-28s %u\n", "view.bone_count_max", (unsigned)view->bone_count_max);
    }
}

static void PrintSkinInfo(LPCSTR model_path, BYTE const *m2_data, DWORD m2_size, m2HeaderInfo_t const *header) {
    PATHSTR requested;
    PATHSTR resolved = { 0 };
    DWORD size = 0;
    LPBYTE data;
    m2SkinHeader_t const *skin;

    if (g_skin_path) {
        snprintf(requested, sizeof(requested), "%s", g_skin_path);
    } else if (!DefaultSkinPath(model_path, requested, sizeof(requested))) {
        printf("skin: unavailable (could not form skin path)\n");
        return;
    }

    data = ReadWholeFile(requested, &size, resolved, sizeof(resolved));
    if (!data) {
        printf("skin: not found (%s)\n", requested);
        PrintEmbeddedSkinInfo(m2_data, m2_size, header);
        return;
    }
    if (size < sizeof(*skin)) {
        printf("skin: %s too small (%u bytes)\n", resolved, (unsigned)size);
        Tool_MemFree(data);
        return;
    }
    skin = (m2SkinHeader_t const *)data;
    if (skin->magic == MAKEFOURCC('S', 'K', 'I', 'N')) {
        printf("skin: %s size=%u\n", resolved, (unsigned)size);
        PrintArray("skin.vertices", skin->vertices, false);
        PrintArray("skin.indices", skin->indices, false);
        PrintArray("skin.bones", skin->bones, false);
        PrintArray("skin.sections", skin->sections, false);
        PrintArray("skin.batches", skin->batches, false);
        printf("  %-28s %u\n", "bone_count_max", (unsigned)skin->bone_count_max);
    } else {
        printf("skin: %s has unexpected magic %.4s\n", resolved, (char const *)&skin->magic);
    }
    Tool_MemFree(data);
}

static void PrintHeaderArrays(m2HeaderInfo_t const *h) {
    BOOL legacy = h->version <= 263;

    printf("arrays:\n");
    PrintArray("name", h->name, false);
    PrintArray("global_sequences", h->global_sequences, false);
    PrintArray("animations", h->animations, false);
    PrintArray("animation_lookup", h->animation_lookup, false);
    if (legacy) {
        PrintArray("playable_animation_lookup", h->playable_animation_lookup, false);
    }
    PrintArray("bones", h->bones, false);
    PrintArray("key_bone_lookup", h->key_bone_lookup, false);
    PrintArray("vertices", h->vertices, false);
    PrintArray(legacy ? "views" : "skin_profiles", h->views, !legacy);
    PrintArray("colors", h->colors, false);
    PrintArray("textures", h->textures, false);
    PrintArray("transparency_lookup", h->transparency_lookup, false);
    if (legacy) {
        PrintArray("texture_flipbooks", h->texture_flipbooks, false);
    }
    PrintArray("texture_animations", h->texture_animations, false);
    PrintArray("color_replacements", h->color_replacements, false);
    PrintArray("render_flags", h->render_flags, false);
    PrintArray("bone_lookup_table", h->bone_lookup_table, false);
    PrintArray("texture_lookup_table", h->texture_lookup_table, false);
    PrintArray("texture_units", h->texture_units, false);
    PrintArray("transparency_lookup_table", h->transparency_lookup_table, false);
    PrintArray("texture_animation_lookup", h->texture_animation_lookup_table, false);
    PrintArray("bounding_triangles", h->bounding_triangles, false);
    PrintArray("bounding_vertices", h->bounding_vertices, false);
    PrintArray("bounding_normals", h->bounding_normals, false);
    PrintArray("attachments", h->attachments, false);
    PrintArray("attachment_lookup", h->attachment_lookup, false);
    PrintArray("events", h->events, false);
    PrintArray("lights", h->lights, false);
    PrintArray("cameras", h->cameras, false);
    PrintArray("camera_lookup", h->camera_lookup, false);
    PrintArray("ribbon_emitters", h->ribbon_emitters, false);
    PrintArray("particle_emitters", h->particle_emitters, false);
}

static void InspectModel(void) {
    PATHSTR resolved = { 0 };
    DWORD file_size = 0;
    LPBYTE file_data;
    BYTE const *payload;
    DWORD payload_size;
    m2HeaderInfo_t header;
    LPCSTR name;
    BOX3 vertex_bounds;
    BOOL has_vertex_bounds;
    FLOAT header_ground_offset;
    FLOAT vertex_ground_offset = 0.0f;

    file_data = ReadWholeFile(g_model_path, &file_size, resolved, sizeof(resolved));
    if (!file_data) {
        errorf("m2tool: failed to open model %s", g_model_path);
    }
    if (!FindM2Payload(file_data, file_size, &payload, &payload_size)) {
        Tool_MemFree(file_data);
        errorf("m2tool: %s is not an M2/MD21 file", resolved);
    }
    if (!ParseHeader(payload, payload_size, &header)) {
        Tool_MemFree(file_data);
        errorf("m2tool: failed to parse M2 header for %s", resolved);
    }

    name = StringPtr(payload, payload_size, header.name);
    printf("m2tool: model=%s resolved=%s file_size=%u payload_size=%u\n",
           g_model_path,
           resolved,
           (unsigned)file_size,
           (unsigned)payload_size);
    printf("header: magic=%.4s version=%u flags=0x%08x layout=%s name=%s\n",
           (char const *)&header.magic,
           (unsigned)header.version,
           (unsigned)header.flags,
           header.version <= 263 ? "classic/tbc" : "wotlk+",
           name && *name ? name : "<none>");

    printf("bounds:\n");
    PrintVec3("bounding.min", header.bounding_box_min);
    PrintVec3("bounding.max", header.bounding_box_max);
    PrintBoundsMetrics("bounding", header.bounding_box_min, header.bounding_box_max);
    printf("  %-22s %.6f\n", "bounding.radius", header.bounding_sphere_radius);
    PrintVec3("collision.min", header.collision_box_min);
    PrintVec3("collision.max", header.collision_box_max);
    PrintBoundsMetrics("collision", header.collision_box_min, header.collision_box_max);
    printf("  %-22s %.6f\n", "collision.radius", header.collision_sphere_radius);

    has_vertex_bounds = CalculateVertexBounds(payload, payload_size, header.vertices, &vertex_bounds);
    if (has_vertex_bounds) {
        printf("vertex_bounds:\n");
        PrintVec3("vertices.min", vertex_bounds.min);
        PrintVec3("vertices.max", vertex_bounds.max);
        PrintBoundsMetrics("vertices", vertex_bounds.min, vertex_bounds.max);
        vertex_ground_offset = vertex_bounds.min.z < 0.0f ? -vertex_bounds.min.z : 0.0f;
    } else {
        printf("vertex_bounds: unavailable\n");
    }

    header_ground_offset = header.bounding_box_min.z < 0.0f ? -header.bounding_box_min.z : 0.0f;
    printf("placement:\n");
    printf("  %-30s %.6f\n", "header_bbox_ground_offset", header_ground_offset);
    if (has_vertex_bounds) {
        printf("  %-30s %.6f\n", "vertex_rest_ground_offset", vertex_ground_offset);
        printf("  %-30s %.6f\n", "header_minus_vertex_delta", header_ground_offset - vertex_ground_offset);
        printf("  %-30s %s\n", "recommended_ground_source", "vertex/rest bounds");
        printf("  %-30s %.6f\n", "recommended_ground_offset", vertex_ground_offset);
        if (header_ground_offset - vertex_ground_offset > 0.05f) {
            printf("  %-30s %s\n",
                   "diagnostic",
                   "header bbox extends below rest mesh; using it as foot contact will make the model hover");
        }
    } else {
        printf("  %-30s %s\n", "vertex_rest_ground_offset", "unavailable");
        printf("  %-30s %s\n", "recommended_ground_source", "none; keep origin on terrain");
    }

    PrintHeaderArrays(&header);
    PrintTextures(payload, payload_size, &header);
    PrintSkinInfo(resolved, payload, payload_size, &header);
    if (g_dump_all) {
        PrintAnimations(payload, payload_size, &header);
    }
    if (g_anim_check) {
        PrintAnimationDiagnostics(payload, payload_size, &header, g_anim_check);
    }

    Tool_MemFree(file_data);
}

int main(int argc, char **argv) {
    int archive_count = 0;

    if (argc <= 1) {
        usage();
        return 1;
    }
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-?") || !strcmp(argv[i], "--help")) {
            usage();
            return 0;
        } else if (!strcmp(argv[i], "-mpq") && i + 1 < argc) {
            if (archive_count >= (int)(sizeof(archives) / sizeof(archives[0]))) {
                errorf("m2tool: too many MPQ archives");
            }
            if (!Tool_AddArchive(archives, sizeof(archives) / sizeof(archives[0]), argv[++i])) {
                return 1;
            }
            archive_count++;
        } else if (!strcmp(argv[i], "-model") && i + 1 < argc) {
            g_model_path = argv[++i];
        } else if (!strcmp(argv[i], "--skin") && i + 1 < argc) {
            g_skin_path = argv[++i];
        } else if (!strcmp(argv[i], "--anim") && i + 1 < argc) {
            g_anim_check = argv[++i];
        } else if (!strcmp(argv[i], "--info")) {
            /* Default mode. */
        } else if (!strcmp(argv[i], "--dump-all")) {
            g_dump_all = true;
        } else {
            usage();
            errorf("m2tool: unknown or incomplete argument: %s", argv[i]);
        }
    }

    if (!archive_count) {
        errorf("m2tool: at least one -mpq archive is required");
    }
    if (!g_model_path) {
        errorf("m2tool: -model is required");
    }

    InspectModel();
    Tool_CloseArchives(archives, sizeof(archives) / sizeof(archives[0]));
    return 0;
}
