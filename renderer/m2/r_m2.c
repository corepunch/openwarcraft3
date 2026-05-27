#include "../r_local.h"
#include <strings.h>

#define M2_MAX_BONES_PER_BATCH 64

typedef struct {
    int32_t size;
    int32_t offset;
} m2Array_t;

typedef struct {
    BYTE v[4];
} m2Ubyte4_t;

typedef struct {
    VECTOR3 min;
    VECTOR3 max;
} m2Box_t;

typedef struct {
    uint16_t track_type;
    uint16_t loop_index;
    m2Array_t sequence_times;
    m2Array_t sequence_keys;
} m2Track_t;

typedef struct {
    uint16_t track_type;
    uint16_t loop_index;
    m2Array_t ranges;
    m2Array_t times;
    m2Array_t keys;
} m2TrackClassic_t;

typedef struct {
    m2Array_t times;
} m2SequenceTimes_t;

typedef struct {
    m2Array_t keys;
} m2SequenceKeys_t;

typedef struct {
    WORD animation_id;
    WORD variation_index;
    DWORD start_timestamp;
    DWORD end_timestamp;
    float movespeed;
    DWORD flags;
    SHORT frequency;
    WORD padding;
    DWORD replay_min;
    DWORD replay_max;
    DWORD blend_time;
    m2Box_t bounds;
    float bounds_radius;
    SHORT next_animation;
    WORD alias_next;
} m2SequenceClassic_t;

typedef struct {
    WORD animation_id;
    WORD variation_index;
    DWORD duration;
    float movespeed;
    DWORD flags;
    DWORD frequency;
    DWORD replay_min;
    DWORD replay_max;
    DWORD blend_time;
    m2Box_t bounds;
    float bounds_radius;
    SHORT next_animation;
    WORD alias_next;
} m2SequenceModern_t;

typedef struct {
    DWORD auCompQ[2];
} m2CompQuat_t;

typedef struct {
    DWORD start;
    DWORD end;
} m2Range_t;

typedef struct {
    DWORD attachment_id;
    WORD bone_index;
    WORD padding;
    VECTOR3 position;
    m2Track_t visibility_track;
} m2AttachmentModern_t;

typedef struct {
    DWORD attachment_id;
    WORD bone_index;
    WORD padding;
    VECTOR3 position;
    m2TrackClassic_t visibility_track;
} m2AttachmentClassic_t;

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
    uint16_t track_type;
    uint16_t loop_index;
    BOOL classic;
    m2Array_t ranges;
    m2Array_t sequence_times;
    m2Array_t sequence_keys;
} m2TrackView_t;

typedef struct {
    DWORD magic;
    DWORD version;
    m2Array_t name;
    DWORD flags;
    m2Array_t global_loops;
    m2Array_t sequences;
    m2Array_t sequence_lookups;
    m2Array_t bones;
    m2Array_t key_bone_lookup;
    m2Array_t vertices;
    DWORD num_skin_profiles;
    m2Array_t colors;
    m2Array_t textures;
    m2Array_t texture_weights;
    m2Array_t texture_transforms;
    m2Array_t replaceable_texture_lookup;
    m2Array_t materials;
    m2Array_t bone_lookup_table;
    m2Array_t texture_lookup_table;
    m2Array_t tex_unit_lookup_table;
    m2Array_t transparency_lookup_table;
    m2Array_t texture_transforms_lookup_table;
    m2Box_t bounding_box;
    float bounding_sphere_radius;
    m2Box_t collision_box;
    float collision_sphere_radius;
    m2Array_t collision_indices;
    m2Array_t collision_positions;
    m2Array_t collision_normals;
    m2Array_t attachments;
    m2Array_t attachment_lookup;
} m2Header_t;

typedef struct {
    DWORD magic;
    DWORD version;
    m2Array_t name;
    DWORD flags;
    m2Array_t global_loops;
    m2Array_t sequences;
    m2Array_t sequence_lookups;
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
    m2Array_t tex_unit_lookup_table;
    m2Array_t transparency_lookup_table;
    m2Array_t texture_transforms_lookup_table;
    m2Box_t bounding_box;
    float bounding_sphere_radius;
    m2Box_t collision_box;
    float collision_sphere_radius;
    m2Array_t collision_indices;
    m2Array_t collision_positions;
    m2Array_t collision_normals;
    m2Array_t attachments;
    m2Array_t attachment_lookup;
} m2HeaderLegacy_t;

typedef struct {
    VECTOR3 pos;
    BYTE bone_weights[4];
    BYTE bone_indices[4];
    VECTOR3 normal;
    VECTOR2 tex_coords[2];
} m2VertexDisk_t;

typedef struct {
    DWORD type;
    DWORD flags;
    m2Array_t filename;
} m2TextureDisk_t;

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
} m2LegacyView_t;

typedef struct {
    WORD skin_section_id;
    WORD level;
    WORD vertex_start;
    WORD vertex_count;
    WORD index_start;
    WORD index_count;
    WORD bone_count;
    WORD bone_combo_index;
    WORD bone_influences;
    WORD center_bone_index;
    VECTOR3 center_position;
    VECTOR3 sort_center_position;
    float sort_radius;
} m2SkinSection_t;

typedef struct {
    WORD skin_section_id;
    WORD level;
    WORD vertex_start;
    WORD vertex_count;
    WORD index_start;
    WORD index_count;
    WORD bone_count;
    WORD bone_combo_index;
    WORD bone_influences;
    WORD center_bone_index;
    VECTOR3 center_position;
} m2SkinSectionLegacy_t;

typedef struct {
    BYTE flags;
    signed char priority_plane;
    WORD shader_id;
    WORD skin_section_index;
    WORD geoset_index;
    SHORT color_index;
    WORD material_index;
    WORD material_layer;
    WORD texture_count;
    WORD texture_combo_index;
    WORD texture_coord_combo_index;
    WORD texture_weight_combo_index;
    WORD texture_transform_combo_index;
} m2Batch_t;

typedef struct m2ModelBatch_s {
    LPBUFFER buffer;
    LPTEXTURE texture;
    DWORD num_vertices;
    WORD bone_count;
    WORD bone_combo_index;
    struct m2ModelBatch_s *next;
} m2ModelBatch_t;

struct m2Model_s {
    m2ModelBatch_t *batches;
    DWORD num_batches;
    BOX3 bounds;
    BOX3 geometry_bounds;
    BOOL has_geometry_bounds;
    BYTE *data;
    DWORD data_size;
    m2Header_t *header;
    BYTE *bones;
    BYTE *sequences;
    WORD *bone_lookup_table;
    m2Array_t global_loops;
    DWORD sequence_stride;
    BOOL classic_sequences;
    DWORD bone_stride;
    BOOL classic_bones;
    DWORD bone_count;
    DWORD sequence_count;
    DWORD bone_lookup_count;
    m2Array_t attachments;
    m2Array_t attachment_lookup;
    DWORD attachment_stride;
    BOOL classic_attachments;
    MATRIX4 *bone_matrices;
};

typedef struct {
    m2Array_t vertices;
    m2Array_t textures;
    m2Array_t texture_lookup_table;
    m2Box_t bounding_box;
} m2GeometryInfo_t;

static LPSHADER m2_shader;

static LPCSTR m2_vs =
"#version 140\n"
"in vec3 i_position;\n"
"in vec2 i_texcoord;\n"
"in vec3 i_normal;\n"
"in vec4 i_color;\n"
"in vec4 i_skin1;\n"
"in vec4 i_boneWeight1;\n"
#ifdef USE_SHADOWMAPS
"out vec4 v_shadow;\n"
#endif
"out vec2 v_texcoord;\n"
"out vec2 v_texcoord2;\n"
"out vec3 v_normal;\n"
"out vec3 v_lightDir;\n"
"out vec4 v_color;\n"
"uniform mat4 uViewProjectionMatrix;\n"
"uniform mat4 uTextureMatrix;\n"
"uniform mat4 uModelMatrix;\n"
"uniform mat4 uLightMatrix;\n"
"uniform mat3 uNormalMatrix;\n"
"uniform mat4 uBones[64];\n"
"vec4 skin_position(vec4 p) {\n"
"    vec4 outp = vec4(0.0);\n"
"    float weight = 0.0;\n"
"    for (int i = 0; i < 4; ++i) {\n"
"        outp += uBones[int(i_skin1[i])] * p * i_boneWeight1[i];\n"
"        weight += i_boneWeight1[i];\n"
"    }\n"
"    return weight > 0.0 ? outp : p;\n"
"}\n"
"vec3 skin_normal(vec4 n) {\n"
"    vec4 outn = vec4(0.0);\n"
"    float weight = 0.0;\n"
"    for (int i = 0; i < 4; ++i) {\n"
"        outn += uBones[int(i_skin1[i])] * n * i_boneWeight1[i];\n"
"        weight += i_boneWeight1[i];\n"
"    }\n"
"    return weight > 0.0 ? outn.xyz : n.xyz;\n"
"}\n"
"void main() {\n"
"    vec4 local = skin_position(vec4(i_position, 1.0));\n"
"    vec4 pos = uModelMatrix * local;\n"
"    v_texcoord = i_texcoord;\n"
"    v_texcoord2 = (uTextureMatrix * pos).xy;\n"
"    v_normal = normalize(uNormalMatrix * skin_normal(vec4(i_normal, 0.0)));\n"
#ifdef USE_SHADOWMAPS
"    v_shadow = uLightMatrix * pos;\n"
#endif
"    v_color = i_color;\n"
"    v_lightDir = -normalize(vec3(uLightMatrix[0][2], uLightMatrix[1][2], uLightMatrix[2][2]))*1.2;\n"
"    gl_Position = uViewProjectionMatrix * pos;\n"
"}\n";

static LPCSTR m2_fs =
"#version 140\n"
"in vec2 v_texcoord;\n"
"in vec2 v_texcoord2;\n"
#ifdef USE_SHADOWMAPS
"in vec4 v_shadow;\n"
#endif
"in vec3 v_normal;\n"
"in vec4 v_color;\n"
"in vec3 v_lightDir;\n"
"out vec4 o_color;\n"
"uniform sampler2D uTexture;\n"
#if defined(USE_SHADOWMAPS) || defined(DEBUG_PATHFINDING)
"uniform sampler2D uShadowmap;\n"
#endif
"uniform sampler2D uFogOfWar;\n"
"float get_light() {\n"
"    return clamp(dot(v_normal, v_lightDir), 0.0, 1.0);\n"
"}\n"
#ifdef USE_SHADOWMAPS
"float get_shadow() {\n"
"    float depth = texture(uShadowmap, vec2(v_shadow.x + 1.0, v_shadow.y + 1.0) * 0.5).r;\n"
"    return depth < (v_shadow.z + 0.99) * 0.5 ? 0.0 : 1.0;\n"
"}\n"
#endif
"float get_lighting() {\n"
#ifdef USE_SHADOWMAPS
"    return mix(0.5, 1.0, get_shadow() * get_light());"
#else
"    return mix(0.5, 1.0, get_light());"
#endif
"}\n"
"float get_fogofwar() {\n"
"    return texture(uFogOfWar, v_texcoord2).r;\n"
"}\n"
"void main() {\n"
"    vec4 col = texture(uTexture, v_texcoord) * v_color;\n"
"    if (col.a < 0.5) discard;\n"
"    col.rgb *= get_fogofwar() * get_lighting();\n"
"    o_color = col;\n"
"}\n";

static LPSHADER M2_Shader(void) {
    if (!m2_shader) {
        m2_shader = R_InitShader(m2_vs, m2_fs);
    }
    return m2_shader ? m2_shader : tr.shader[SHADER_DEFAULT];
}

static void M2_LogFallback(LPCSTR modelFilename, LPCSTR reason) {
    static DWORD fallback_count;

    fallback_count++;
    if (fallback_count <= 64 || (fallback_count % 100) == 0) {
        fprintf(stderr,
                "M2 fallback: %s model=%s count=%u\n",
                reason ? reason : "unknown",
                modelFilename ? modelFilename : "<null>",
                fallback_count);
    }
}

static m2Model_t *M2_CreateFallbackModel(LPCSTR modelFilename, LPCSTR reason) {
    static VERTEX vertices[12];
    static BOOL initialized;
    m2Model_t *model;
    m2ModelBatch_t *batch;
    COLOR32 color = { 90, 230, 130, 255 };

    M2_LogFallback(modelFilename, reason);

    if (!initialized) {
        VECTOR3 base[4] = {
            { -14.0f, -14.0f, 0.0f },
            {  14.0f, -14.0f, 0.0f },
            {  14.0f,  14.0f, 0.0f },
            { -14.0f,  14.0f, 0.0f },
        };
        VECTOR3 top = { 0.0f, 0.0f, 42.0f };
        int tri[12] = { 0, 1, 2, 0, 2, 3, 0, 1, 4, 2, 3, 4 };
        VECTOR3 points[5];
        memcpy(points, base, sizeof(base));
        points[4] = top;
        FOR_LOOP(i, 12) {
            memset(&vertices[i], 0, sizeof(vertices[i]));
            vertices[i].position = points[tri[i]];
            vertices[i].normal = (VECTOR3){ 0.0f, 0.0f, 1.0f };
            vertices[i].texcoord = (VECTOR2){ 0.0f, 0.0f };
            vertices[i].color = color;
        }
        initialized = true;
    }

    model = ri.MemAlloc(sizeof(*model));
    memset(model, 0, sizeof(*model));
    model->bounds = (BOX3){
        .min = { -14.0f, -14.0f, 0.0f },
        .max = { 14.0f, 14.0f, 42.0f },
    };
    model->geometry_bounds = model->bounds;
    model->has_geometry_bounds = true;
    batch = ri.MemAlloc(sizeof(*batch));
    memset(batch, 0, sizeof(*batch));
    batch->buffer = R_MakeVertexArrayObject(vertices, 12);
    batch->texture = tr.texture[TEX_WHITE];
    batch->num_vertices = 12;
    model->batches = batch;
    model->num_batches = 1;
    return model;
}

static BOOL M2_PathHasExtension(LPCSTR path, LPCSTR extension) {
    size_t path_len;
    size_t ext_len;

    if (!path || !extension) {
        return false;
    }
    path_len = strlen(path);
    ext_len = strlen(extension);
    return path_len >= ext_len && strcasecmp(path + path_len - ext_len, extension) == 0;
}

static BOOL M2_TagEquals(BYTE const *tag, LPCSTR reversed) {
    return memcmp(tag, reversed, 4) == 0;
}

static BYTE const *M2_FindChunk(BYTE const *data, DWORD size, LPCSTR reversed_tag, LPDWORD chunk_size) {
    DWORD offset = 0;

    while (offset + 8 <= size) {
        BYTE const *tag = data + offset;
        DWORD current_size;
        memcpy(&current_size, data + offset + 4, sizeof(current_size));
        offset += 8;
        if (offset + current_size > size) {
            break;
        }
        if (M2_TagEquals(tag, reversed_tag)) {
            *chunk_size = current_size;
            return data + offset;
        }
        offset += current_size;
    }
    return NULL;
}

static BOOL M2_CopyWithExtension(LPCSTR path, LPCSTR extension, LPSTR out, DWORD out_size) {
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

static BOOL M2_DefaultCharacterTexturePath(LPCSTR model_path,
                                           DWORD texture_type,
                                           LPSTR out,
                                           DWORD out_size) {
    LPCSTR character;
    LPCSTR race;
    LPCSTR gender;
    LPCSTR model_name;
    char race_buf[64];
    char gender_buf[64];
    char model_buf[64];
    size_t len;

    if (!model_path || !out || out_size == 0) {
        return false;
    }
    character = strcasestr(model_path, "Character\\");
    if (!character) {
        character = strcasestr(model_path, "Character/");
    }
    if (!character) {
        return false;
    }
    race = character + strlen("Character\\");
    gender = strpbrk(race, "\\/");
    if (!gender) {
        return false;
    }
    len = (size_t)(gender - race);
    if (len == 0 || len >= sizeof(race_buf)) {
        return false;
    }
    memcpy(race_buf, race, len);
    race_buf[len] = '\0';

    gender++;
    model_name = strpbrk(gender, "\\/");
    if (!model_name) {
        model_name = strrchr(gender, '.');
        if (!model_name) {
            return false;
        }
        len = (size_t)(model_name - gender);
        if (len == 0 || len >= sizeof(gender_buf)) {
            return false;
        }
        memcpy(gender_buf, gender, len);
        gender_buf[len] = '\0';
        model_name = gender;
    } else {
        len = (size_t)(model_name - gender);
        if (len == 0 || len >= sizeof(gender_buf)) {
            return false;
        }
        memcpy(gender_buf, gender, len);
        gender_buf[len] = '\0';
        model_name++;
    }

    len = strcspn(model_name, ".");
    if (len == 0 || len >= sizeof(model_buf)) {
        return false;
    }
    memcpy(model_buf, model_name, len);
    model_buf[len] = '\0';

    switch (texture_type) {
        case 1:
            snprintf(out, out_size, "Character\\%s\\%s\\%sSkin00_00.blp", race_buf, gender_buf, model_buf);
            return true;
        case 2:
            snprintf(out, out_size, "Character\\%s\\%s\\%sNakedPelvisSkin00_00.blp", race_buf, gender_buf, model_buf);
            return true;
        case 6:
            snprintf(out, out_size, "Character\\%s\\Hair00_00.blp", race_buf);
            return true;
        case 8:
            snprintf(out, out_size, "Character\\%s\\%s\\%sSkin00_100.blp", race_buf, gender_buf, model_buf);
            return true;
        default:
            return false;
    }
}

static BOOL M2_DefaultObjectComponentTexturePath(LPCSTR model_path,
                                                 DWORD texture_type,
                                                 LPSTR out,
                                                 DWORD out_size) {
    LPCSTR filename;
    size_t stem_len;

    if (!model_path || !out || out_size == 0 || texture_type != 2) {
        return false;
    }
    if (!strcasestr(model_path, "Item\\ObjectComponents\\Weapon\\") &&
        !strcasestr(model_path, "Item/ObjectComponents/Weapon/")) {
        return false;
    }

    filename = strrchr(model_path, '\\');
    if (!filename) {
        filename = strrchr(model_path, '/');
    }
    filename = filename ? filename + 1 : model_path;
    stem_len = strcspn(filename, ".");

    if (stem_len == strlen("Axe_1H_Horde_A_01") &&
        !strncasecmp(filename, "Axe_1H_Horde_A_01", stem_len)) {
        snprintf(out, out_size, "Item\\ObjectComponents\\Weapon\\Axe_1H_Horde_A_01Gray.blp");
        return true;
    }
    if (stem_len == 0 || stem_len + strlen("Item\\ObjectComponents\\Weapon\\.blp") + 1 > out_size) {
        return false;
    }

    snprintf(out, out_size, "Item\\ObjectComponents\\Weapon\\%.*s.blp", (int)stem_len, filename);
    return true;
}

static BOOL M2_DefaultCreatureTexturePath(LPCSTR model_path,
                                          DWORD texture_type,
                                          LPSTR out,
                                          DWORD out_size) {
    typedef struct {
        LPCSTR model;
        DWORD texture_type;
        LPCSTR texture;
    } defaultCreatureTexture_t;
    static defaultCreatureTexture_t const defaults[] = {
        { "Creature\\Wolf\\Wolf.mdx",     11, "Creature\\Wolf\\WolfSkinCoyote.blp" },
        { "Creature\\Wolf\\Wolf.m2",      11, "Creature\\Wolf\\WolfSkinCoyote.blp" },
        { "Creature\\Wolf\\Wolf.mdx",     12, "Creature\\Wolf\\WolfSkinCoyoteAlpha.blp" },
        { "Creature\\Wolf\\Wolf.m2",      12, "Creature\\Wolf\\WolfSkinCoyoteAlpha.blp" },
        { "Creature\\Boar\\Boar.mdx",     11, "Creature\\Boar\\BoarSkinIvory.blp" },
        { "Creature\\Boar\\Boar.m2",      11, "Creature\\Boar\\BoarSkinIvory.blp" },
        { "Creature\\Kobold\\Kobold.mdx", 11, "Creature\\Kobold\\koboldskinAlbino.blp" },
        { "Creature\\Kobold\\Kobold.m2",  11, "Creature\\Kobold\\koboldskinAlbino.blp" },
        { "Creature\\Murloc\\Murloc.mdx", 11, "Creature\\Murloc\\SahauginskinBlue.blp" },
        { "Creature\\Murloc\\Murloc.m2",  11, "Creature\\Murloc\\SahauginskinBlue.blp" },
    };

    if (!model_path || !out || out_size == 0) {
        return false;
    }

    FOR_LOOP(i, sizeof(defaults) / sizeof(defaults[0])) {
        if (texture_type == defaults[i].texture_type &&
            !strcasecmp(model_path, defaults[i].model)) {
            snprintf(out, out_size, "%s", defaults[i].texture);
            return true;
        }
    }
    return false;
}

static BOOL M2_ArrayRange(m2Array_t array, DWORD elem_size, DWORD file_size, DWORD *offset, DWORD *bytes) {
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

static void *M2_ArrayPtr(BYTE const *base, DWORD file_size, m2Array_t array, DWORD elem_size) {
    DWORD offset;
    DWORD bytes;

    if (!M2_ArrayRange(array, elem_size, file_size, &offset, &bytes)) {
        return NULL;
    }
    return (void *)(base + offset);
}

static void *M2_ModelArrayPtr(m2Model_t const *model, m2Array_t array, DWORD elem_size) {
    if (!model || !model->data) {
        return NULL;
    }
    return M2_ArrayPtr(model->data, model->data_size, array, elem_size);
}

static LPCSTR M2_StringPtr(BYTE const *base, DWORD file_size, m2Array_t array) {
    DWORD offset;
    DWORD bytes;

    if (!M2_ArrayRange(array, 1, file_size, &offset, &bytes) || bytes == 0) {
        return NULL;
    }
    if (memchr(base + offset, '\0', bytes) == NULL) {
        return NULL;
    }
    return (LPCSTR)(base + offset);
}

static DWORD M2_SequenceStart(m2Model_t const *model, DWORD sequence_index) {
    if (!model || !model->sequences || sequence_index >= model->sequence_count) {
        return 0;
    }
    if (model->classic_sequences) {
        m2SequenceClassic_t const *sequence = (m2SequenceClassic_t const *)(model->sequences + sequence_index * model->sequence_stride);
        return sequence->start_timestamp;
    }
    return 0;
}

static DWORD M2_SequenceDuration(m2Model_t const *model, DWORD sequence_index) {
    if (!model || !model->sequences || sequence_index >= model->sequence_count) {
        return 0;
    }
    if (model->classic_sequences) {
        m2SequenceClassic_t const *sequence = (m2SequenceClassic_t const *)(model->sequences + sequence_index * model->sequence_stride);
        return sequence->end_timestamp > sequence->start_timestamp
            ? sequence->end_timestamp - sequence->start_timestamp
            : 0;
    } else {
        m2SequenceModern_t const *sequence = (m2SequenceModern_t const *)(model->sequences + sequence_index * model->sequence_stride);
        return sequence->duration;
    }
}

static DWORD M2_SequenceFlags(m2Model_t const *model, DWORD sequence_index) {
    if (!model || !model->sequences || sequence_index >= model->sequence_count) {
        return 0;
    }
    if (model->classic_sequences) {
        m2SequenceClassic_t const *sequence = (m2SequenceClassic_t const *)(model->sequences + sequence_index * model->sequence_stride);
        return sequence->flags;
    } else {
        m2SequenceModern_t const *sequence = (m2SequenceModern_t const *)(model->sequences + sequence_index * model->sequence_stride);
        return sequence->flags;
    }
}

typedef struct {
    DWORD sequence_index;
    DWORD sequence_time;
} m2PoseTime_t;

static BOOL M2_FrameToPoseTime(m2Model_t const *model, DWORD frame, m2PoseTime_t *pose) {
    DWORD range_start = 0;

    if (pose) {
        pose->sequence_index = 0;
        pose->sequence_time = frame;
    }
    if (!model || !model->sequences || model->sequence_count == 0 || !pose) {
        return false;
    }

    FOR_LOOP(i, model->sequence_count) {
        DWORD duration = M2_SequenceDuration(model, i);
        DWORD range_length = MAX(duration, 1);

        if (frame >= range_start && frame < range_start + range_length) {
            DWORD local_time = frame - range_start;

            pose->sequence_index = i;
            pose->sequence_time = duration
                ? M2_SequenceStart(model, i) + (local_time % duration)
                : M2_SequenceStart(model, i);
            return true;
        }
        range_start += range_length;
    }

    if (model->sequence_count > 0) {
        DWORD duration = M2_SequenceDuration(model, 0);

        pose->sequence_index = 0;
        pose->sequence_time = duration
            ? M2_SequenceStart(model, 0) + (frame % duration)
            : M2_SequenceStart(model, 0);
        return true;
    }
    return false;
}

static DWORD M2_AnimationTime(m2Model_t const *model, renderEntity_t const *entity, DWORD *sequence_index) {
    DWORD frame = entity ? entity->frame : tr.viewDef.time;
    m2PoseTime_t pose;
    m2PoseTime_t old_pose;

    if (sequence_index) {
        *sequence_index = 0;
    }
    if (!M2_FrameToPoseTime(model, frame, &pose)) {
        return frame;
    }

    if (entity &&
        M2_FrameToPoseTime(model, entity->oldframe, &old_pose) &&
        old_pose.sequence_index == pose.sequence_index) {
        DWORD duration = M2_SequenceDuration(model, pose.sequence_index);
        DWORD start_time = M2_SequenceStart(model, pose.sequence_index);
        DWORD old_time = old_pose.sequence_time - start_time;
        DWORD local_time = pose.sequence_time - start_time;
        FLOAT end_time = (FLOAT)local_time;
        FLOAT lerped;

        if (duration > 0 && old_time > local_time && !(M2_SequenceFlags(model, pose.sequence_index) & 0x1)) {
            end_time += (FLOAT)duration;
        }
        lerped = LerpNumber((FLOAT)old_time, end_time, tr.viewDef.lerpfrac);
        if (duration > 0 && lerped >= (FLOAT)duration) {
            lerped -= (FLOAT)duration * floorf(lerped / (FLOAT)duration);
        }
        pose.sequence_time = start_time + (DWORD)MAX(0.0f, lerped);
    }

    if (sequence_index) {
        *sequence_index = pose.sequence_index;
    }
    return pose.sequence_time;
}

static DWORD M2_TrackTime(m2Model_t const *model,
                          m2TrackView_t const *track,
                          DWORD sequence_index,
                          DWORD sequence_time) {
    DWORD const *loops;

    if (!model || !track || !model->header || track->loop_index == 0xFFFF) {
        return sequence_time;
    }

    loops = M2_ModelArrayPtr(model, model->global_loops, sizeof(DWORD));
    if (!loops || track->loop_index >= (WORD)model->global_loops.size || loops[track->loop_index] == 0) {
        return sequence_time;
    }

    (void)sequence_index;
    return tr.viewDef.time % loops[track->loop_index];
}

static BOOL M2_FindTrackKeys(m2Model_t const *model,
                             m2TrackView_t const *track,
                             DWORD sequence_index,
                             DWORD sequence_time,
                             DWORD elem_size,
                             void const **left,
                             void const **right,
                             float *ratio) {
    DWORD const *times;
    BYTE const *keys;
    DWORD count;

    if (!model || !track || !left || !right || !ratio) {
        return false;
    }

    if (track->classic) {
        m2Range_t const *ranges = M2_ModelArrayPtr(model, track->ranges, sizeof(*ranges));
        m2Range_t range;

        if (!ranges || track->ranges.size == 0) {
            return false;
        }
        if (sequence_index >= (DWORD)track->ranges.size) {
            sequence_index = 0;
        }

        range = ranges[sequence_index];
        if (range.end < range.start) {
            return false;
        }

        times = M2_ModelArrayPtr(model, track->sequence_times, sizeof(DWORD));
        keys = M2_ModelArrayPtr(model, track->sequence_keys, elem_size);
        if (!times || !keys ||
            range.start >= (DWORD)track->sequence_times.size ||
            range.start >= (DWORD)track->sequence_keys.size) {
            return false;
        }

        count = range.end - range.start + 1;
        count = MIN(count, (DWORD)track->sequence_times.size - range.start);
        count = MIN(count, (DWORD)track->sequence_keys.size - range.start);
        times += range.start;
        keys += range.start * elem_size;
        if (count == 0) {
            return false;
        }
    } else {
        m2SequenceTimes_t const *sequence_times = M2_ModelArrayPtr(model, track->sequence_times, sizeof(*sequence_times));
        m2SequenceKeys_t const *sequence_keys = M2_ModelArrayPtr(model, track->sequence_keys, sizeof(*sequence_keys));
        if (!sequence_times || !sequence_keys || track->sequence_times.size == 0 || track->sequence_keys.size == 0) {
            return false;
        }

        if (sequence_index >= (DWORD)track->sequence_times.size || sequence_index >= (DWORD)track->sequence_keys.size) {
            sequence_index = 0;
        }

        times = M2_ModelArrayPtr(model, sequence_times[sequence_index].times, sizeof(DWORD));
        keys = M2_ModelArrayPtr(model, sequence_keys[sequence_index].keys, elem_size);
        count = MIN((DWORD)sequence_times[sequence_index].times.size, (DWORD)sequence_keys[sequence_index].keys.size);
        if (!times || !keys || count == 0) {
            return false;
        }
    }

    if (count == 1 || track->track_type == TRACK_NO_INTERP || sequence_time <= times[0]) {
        *left = keys;
        *right = keys;
        *ratio = 0.0f;
        return true;
    }

    for (DWORD i = 1; i < count; i++) {
        if (sequence_time <= times[i]) {
            DWORD left_time = times[i - 1];
            DWORD right_time = times[i];
            *left = keys + ((i - 1) * elem_size);
            *right = keys + (i * elem_size);
            *ratio = right_time > left_time
                ? (float)(sequence_time - left_time) / (float)(right_time - left_time)
                : 0.0f;
            return true;
        }
    }

    *left = keys + ((count - 1) * elem_size);
    *right = *left;
    *ratio = 0.0f;
    return true;
}

static VECTOR3 M2_EvaluateVectorTrack(m2Model_t const *model,
                                      m2TrackView_t const *track,
                                      DWORD sequence_index,
                                      DWORD sequence_time,
                                      VECTOR3 default_value) {
    void const *left;
    void const *right;
    float ratio;
    DWORD track_time = M2_TrackTime(model, track, sequence_index, sequence_time);

    if (!M2_FindTrackKeys(model, track, sequence_index, track_time, sizeof(VECTOR3), &left, &right, &ratio)) {
        return default_value;
    }
    if (left == right) {
        return *(VECTOR3 const *)left;
    }
    return Vector3_lerp((LPCVECTOR3)left, (LPCVECTOR3)right, ratio);
}

static QUATERNION M2_DecodeCompQuat(m2CompQuat_t const *source) {
    return (QUATERNION) {
        .x = (float)(source->auCompQ[0] & 0xFFFF) * 0.000030518044f - 1.0f,
        .y = (float)(source->auCompQ[0] >> 16)    * 0.000030518044f - 1.0f,
        .z = (float)(source->auCompQ[1] & 0xFFFF) * 0.000030518044f - 1.0f,
        .w = (float)(source->auCompQ[1] >> 16)    * 0.000030518044f - 1.0f,
    };
}

static QUATERNION M2_QuaternionNlerp(LPCQUATERNION q1, LPCQUATERNION q2, float ratio) {
    QUATERNION out = {
        .x = (q2->x - q1->x) * ratio + q1->x,
        .y = (q2->y - q1->y) * ratio + q1->y,
        .z = (q2->z - q1->z) * ratio + q1->z,
        .w = (q2->w - q1->w) * ratio + q1->w,
    };
    float m = out.x * out.x + out.y * out.y + out.z * out.z + out.w * out.w;
    float v = ((m - 0.95906597f) * -0.532516f) + 1.021435f;

    if (m <= 0.91521198f) {
        v *= (((v * v * m) - 0.95906597f) * -0.532516f) + 1.021435f;
        if (m <= 0.6521197f) {
            v *= (((v * v * m) - 0.95906597f) * -0.532516f) + 1.021435f;
        }
    }

    out.x *= v;
    out.y *= v;
    out.z *= v;
    out.w *= v;
    return out;
}

static QUATERNION M2_EvaluateRotationTrack(m2Model_t const *model,
                                           m2TrackView_t const *track,
                                           DWORD sequence_index,
                                           DWORD sequence_time,
                                           QUATERNION default_value) {
    void const *left;
    void const *right;
    float ratio;
    DWORD track_time = M2_TrackTime(model, track, sequence_index, sequence_time);
    DWORD elem_size = track && track->classic ? sizeof(QUATERNION) : sizeof(m2CompQuat_t);

    if (!M2_FindTrackKeys(model, track, sequence_index, track_time, elem_size, &left, &right, &ratio)) {
        return default_value;
    }
    if (track->classic) {
        if (left == right) {
            return *(QUATERNION const *)left;
        }

        QUATERNION q1 = *(QUATERNION const *)left;
        QUATERNION q2 = *(QUATERNION const *)right;
        return M2_QuaternionNlerp(&q1, &q2, ratio);
    }
    if (left == right) {
        return M2_DecodeCompQuat((m2CompQuat_t const *)left);
    }

    QUATERNION q1 = M2_DecodeCompQuat((m2CompQuat_t const *)left);
    QUATERNION q2 = M2_DecodeCompQuat((m2CompQuat_t const *)right);
    return M2_QuaternionNlerp(&q1, &q2, ratio);
}

static BOOL M2_TrackHasKeys(m2TrackView_t const *track) {
    if (!track) {
        return false;
    }
    if (track->classic) {
        return track->ranges.size > 0 && track->sequence_times.size > 0 && track->sequence_keys.size > 0;
    }
    return track->sequence_times.size > 0 && track->sequence_keys.size > 0;
}

static void const *M2_BonePtr(m2Model_t const *model, DWORD bone_index) {
    if (!model || !model->bones || bone_index >= model->bone_count || model->bone_stride == 0) {
        return NULL;
    }
    return model->bones + (bone_index * model->bone_stride);
}

static DWORD M2_BoneFlags(m2Model_t const *model, DWORD bone_index) {
    void const *bone = M2_BonePtr(model, bone_index);

    if (!bone) {
        return 0;
    }
    if (model->classic_bones) {
        return ((m2CompBoneClassic_t const *)bone)->flags;
    }
    return ((m2CompBoneModern_t const *)bone)->flags;
}

static WORD M2_BoneParentIndex(m2Model_t const *model, DWORD bone_index) {
    void const *bone = M2_BonePtr(model, bone_index);

    if (!bone) {
        return 0xFFFF;
    }
    if (model->classic_bones) {
        return ((m2CompBoneClassic_t const *)bone)->parent_index;
    }
    return ((m2CompBoneModern_t const *)bone)->parent_index;
}

static VECTOR3 M2_BonePivot(m2Model_t const *model, DWORD bone_index) {
    void const *bone = M2_BonePtr(model, bone_index);

    if (!bone) {
        return (VECTOR3){ 0.0f, 0.0f, 0.0f };
    }
    if (model->classic_bones) {
        return ((m2CompBoneClassic_t const *)bone)->pivot;
    }
    return ((m2CompBoneModern_t const *)bone)->pivot;
}

static m2TrackView_t M2_ModernTrackView(m2Track_t const *track) {
    return (m2TrackView_t) {
        .track_type = track ? track->track_type : 0,
        .loop_index = track ? track->loop_index : 0xFFFF,
        .classic = false,
        .ranges = { 0, 0 },
        .sequence_times = track ? track->sequence_times : (m2Array_t){ 0, 0 },
        .sequence_keys = track ? track->sequence_keys : (m2Array_t){ 0, 0 },
    };
}

static m2TrackView_t M2_ClassicTrackView(m2TrackClassic_t const *track) {
    return (m2TrackView_t) {
        .track_type = track ? track->track_type : 0,
        .loop_index = track ? track->loop_index : 0xFFFF,
        .classic = true,
        .ranges = track ? track->ranges : (m2Array_t){ 0, 0 },
        .sequence_times = track ? track->times : (m2Array_t){ 0, 0 },
        .sequence_keys = track ? track->keys : (m2Array_t){ 0, 0 },
    };
}

static m2TrackView_t M2_BoneTranslationTrack(m2Model_t const *model, DWORD bone_index) {
    void const *bone = M2_BonePtr(model, bone_index);

    if (!bone) {
        return M2_ModernTrackView(NULL);
    }
    if (model->classic_bones) {
        return M2_ClassicTrackView(&((m2CompBoneClassic_t const *)bone)->translation_track);
    }
    return M2_ModernTrackView(&((m2CompBoneModern_t const *)bone)->translation_track);
}

static m2TrackView_t M2_BoneRotationTrack(m2Model_t const *model, DWORD bone_index) {
    void const *bone = M2_BonePtr(model, bone_index);

    if (!bone) {
        return M2_ModernTrackView(NULL);
    }
    if (model->classic_bones) {
        return M2_ClassicTrackView(&((m2CompBoneClassic_t const *)bone)->rotation_track);
    }
    return M2_ModernTrackView(&((m2CompBoneModern_t const *)bone)->rotation_track);
}

static m2TrackView_t M2_BoneScaleTrack(m2Model_t const *model, DWORD bone_index) {
    void const *bone = M2_BonePtr(model, bone_index);

    if (!bone) {
        return M2_ModernTrackView(NULL);
    }
    if (model->classic_bones) {
        return M2_ClassicTrackView(&((m2CompBoneClassic_t const *)bone)->scale_track);
    }
    return M2_ModernTrackView(&((m2CompBoneModern_t const *)bone)->scale_track);
}

static void M2_CalculateBoneMatrices(m2Model_t const *model, renderEntity_t const *entity) {
    MATRIX4 identity;
    m2PoseTime_t current_pose;
    m2PoseTime_t old_pose;
    FLOAT pose_lerp = 1.0f;

    if (!model || !model->bone_matrices || !model->bones) {
        return;
    }

    Matrix4_identity(&identity);
    M2_FrameToPoseTime(model, entity ? entity->frame : tr.viewDef.time, &current_pose);
    old_pose = current_pose;
    if (entity &&
        entity->oldframe != entity->frame &&
        M2_FrameToPoseTime(model, entity->oldframe, &old_pose) &&
        old_pose.sequence_index != current_pose.sequence_index) {
        pose_lerp = MAX(0.0f, MIN(1.0f, tr.viewDef.lerpfrac));
    } else {
        current_pose.sequence_time = M2_AnimationTime(model, entity, &current_pose.sequence_index);
        old_pose = current_pose;
    }

    FOR_LOOP(i, model->bone_count) {
        WORD parent_index = M2_BoneParentIndex(model, i);
        DWORD flags = M2_BoneFlags(model, i);
        VECTOR3 pivot = M2_BonePivot(model, i);
        m2TrackView_t translation_track = M2_BoneTranslationTrack(model, i);
        m2TrackView_t rotation_track = M2_BoneRotationTrack(model, i);
        m2TrackView_t scale_track = M2_BoneScaleTrack(model, i);
        BOOL has_keys = M2_TrackHasKeys(&translation_track) ||
                        M2_TrackHasKeys(&rotation_track) ||
                        M2_TrackHasKeys(&scale_track);
        LPCMATRIX4 parent = &identity;

        if (parent_index != 0xFFFF && parent_index < i) {
            parent = &model->bone_matrices[parent_index];
        }

        if ((flags & (0x80 | 0x200)) || has_keys) {
            MATRIX4 local;
            VECTOR3 translation = M2_EvaluateVectorTrack(model,
                                                         &translation_track,
                                                         current_pose.sequence_index,
                                                         current_pose.sequence_time,
                                                         (VECTOR3){ 0.0f, 0.0f, 0.0f });
            QUATERNION rotation = M2_EvaluateRotationTrack(model,
                                                           &rotation_track,
                                                           current_pose.sequence_index,
                                                           current_pose.sequence_time,
                                                           (QUATERNION){ 0.0f, 0.0f, 0.0f, 1.0f });
            VECTOR3 scale = M2_EvaluateVectorTrack(model,
                                                   &scale_track,
                                                   current_pose.sequence_index,
                                                   current_pose.sequence_time,
                                                   (VECTOR3){ 1.0f, 1.0f, 1.0f });

            if (pose_lerp < 1.0f) {
                VECTOR3 old_translation = M2_EvaluateVectorTrack(model,
                                                                 &translation_track,
                                                                 old_pose.sequence_index,
                                                                 old_pose.sequence_time,
                                                                 (VECTOR3){ 0.0f, 0.0f, 0.0f });
                QUATERNION old_rotation = M2_EvaluateRotationTrack(model,
                                                                   &rotation_track,
                                                                   old_pose.sequence_index,
                                                                   old_pose.sequence_time,
                                                                   (QUATERNION){ 0.0f, 0.0f, 0.0f, 1.0f });
                VECTOR3 old_scale = M2_EvaluateVectorTrack(model,
                                                           &scale_track,
                                                           old_pose.sequence_index,
                                                           old_pose.sequence_time,
                                                           (VECTOR3){ 1.0f, 1.0f, 1.0f });

                translation = Vector3_lerp(&old_translation, &translation, pose_lerp);
                rotation = Quaternion_slerp(&old_rotation, &rotation, pose_lerp);
                scale = Vector3_lerp(&old_scale, &scale, pose_lerp);
            }

            Matrix4_from_rotation_translation_scale_origin(&local,
                                                           &rotation,
                                                           &translation,
                                                           &scale,
                                                           &pivot);
            Matrix4_multiply(parent, &local, &model->bone_matrices[i]);
        } else {
            model->bone_matrices[i] = *parent;
        }
    }
}

static void M2_UploadBatchBones(m2Model_t const *model, m2ModelBatch_t const *batch, LPSHADER shader) {
    MATRIX4 palette[M2_MAX_BONES_PER_BATCH];

    FOR_LOOP(i, M2_MAX_BONES_PER_BATCH) {
        Matrix4_identity(&palette[i]);
    }

    if (model && batch && model->bone_matrices && model->bone_lookup_table) {
        DWORD count = MIN((DWORD)batch->bone_count, (DWORD)M2_MAX_BONES_PER_BATCH);
        FOR_LOOP(i, count) {
            DWORD lookup = (DWORD)batch->bone_combo_index + i;
            if (lookup < model->bone_lookup_count) {
                WORD bone_index = model->bone_lookup_table[lookup];
                if (bone_index < model->bone_count) {
                    palette[i] = model->bone_matrices[bone_index];
                }
            }
        }
    }

    R_Call(glUniformMatrix4fv, shader->uBones, M2_MAX_BONES_PER_BATCH, GL_FALSE, palette[0].v);
}

static BOOL M2_CommonOrcMaleSectionTexture(WORD section_id, LPSTR out, DWORD out_size);

static LPTEXTURE M2_TextureForBatch(BYTE const *m2_data,
                                    DWORD m2_size,
                                    m2GeometryInfo_t const *geom,
                                    m2Batch_t const *batch,
                                    LPCSTR modelFilename,
                                    WORD section_id,
                                    BOOL use_texture_lookup) {
    SHORT const *texture_lookup;
    m2TextureDisk_t const *texture;
    SHORT texture_index;
    LPCSTR texture_path;
    PATHSTR replacement_path;

    if (!geom || !batch || batch->texture_count == 0) {
        return tr.texture[TEX_WHITE];
    }

    if (M2_CommonOrcMaleSectionTexture(section_id, replacement_path, sizeof(replacement_path))) {
        return R_LoadTexture(replacement_path);
    }

    if (use_texture_lookup) {
        texture_lookup = M2_ArrayPtr(m2_data, m2_size, geom->texture_lookup_table, sizeof(SHORT));
        if (!texture_lookup || batch->texture_combo_index >= (WORD)geom->texture_lookup_table.size) {
            return tr.texture[TEX_WHITE];
        }
        texture_index = texture_lookup[batch->texture_combo_index];
    } else {
        texture_index = (SHORT)batch->texture_combo_index;
    }
    if (texture_index < 0 || texture_index >= geom->textures.size) {
        return tr.texture[TEX_WHITE];
    }

    texture = M2_ArrayPtr(m2_data, m2_size, geom->textures, sizeof(*texture));
    if (!texture) {
        return tr.texture[TEX_WHITE];
    }

    texture_path = M2_StringPtr(m2_data, m2_size, texture[texture_index].filename);
    if (!texture_path || !*texture_path) {
        if (M2_DefaultCharacterTexturePath(modelFilename, texture[texture_index].type, replacement_path, sizeof(replacement_path))) {
            return R_LoadTexture(replacement_path);
        }
        if (M2_DefaultObjectComponentTexturePath(modelFilename, texture[texture_index].type, replacement_path, sizeof(replacement_path))) {
            return R_LoadTexture(replacement_path);
        }
        if (M2_DefaultCreatureTexturePath(modelFilename, texture[texture_index].type, replacement_path, sizeof(replacement_path))) {
            return R_LoadTexture(replacement_path);
        }
        return tr.texture[TEX_WHITE];
    }
    return R_LoadTexture(texture_path);
}

static BOOL M2_CommonOrcMaleSectionTexture(WORD section_id, LPSTR out, DWORD out_size) {
    LPCSTR texture_path = NULL;

    switch (section_id) {
        case 401:
        case 402:
        case 403:
        case 404:
            texture_path = "Item\\TextureComponents\\HandTexture\\Cloth_A_01Brown_Glove_HA_U.blp";
            break;
        case 501:
        case 502:
        case 503:
        case 504:
            texture_path = "Item\\TextureComponents\\FootTexture\\Cloth_A_01Brown_Boot_FO_M.blp";
            break;
        case 802:
        case 803:
            texture_path = "Item\\TextureComponents\\ArmLowerTexture\\Cloth_A_01Brown_Sleeve_AL_U.blp";
            break;
        case 902:
        case 903:
            texture_path = "Item\\TextureComponents\\LegLowerTexture\\Cloth_A_01Brown_Pant_LL_M.blp";
            break;
        case 1002:
            texture_path = "Item\\TextureComponents\\TorsoUpperTexture\\Cloth_A_01Brown_Chest_TU_M.blp";
            break;
        case 1102:
        case 1202:
            texture_path = "Item\\TextureComponents\\TorsoLowerTexture\\Cloth_A_01Brown_Chest_TL_M.blp";
            break;
        case 1302:
            texture_path = "Item\\TextureComponents\\LegUpperTexture\\Cloth_A_01Brown_Pant_LU_M.blp";
            break;
        default:
            break;
    }

    if (!texture_path || !out || out_size == 0) {
        return false;
    }
    snprintf(out, out_size, "%s", texture_path);
    return true;
}

static BOOL M2_SkinPath(LPCSTR model_path, LPSTR out, DWORD out_size) {
    if (!M2_CopyWithExtension(model_path, "00.skin", out, out_size)) {
        return false;
    }
    return true;
}

static VERTEX M2_MakeVertex(m2VertexDisk_t const *src) {
    VERTEX out;
    memset(&out, 0, sizeof(out));
    out.position = src->pos;
    out.normal = src->normal;
    out.texcoord = src->tex_coords[0];
    out.color = COLOR32_WHITE;
    memcpy(out.skin, src->bone_indices, sizeof(src->bone_indices));
    memcpy(out.boneWeight, src->bone_weights, sizeof(src->bone_weights));
    return out;
}

static BOOL M2_CalculateGeometryBounds(m2VertexDisk_t const *vertices, DWORD num_vertices, BOX3 *bounds) {
    if (!vertices || !num_vertices || !bounds) {
        return false;
    }

    bounds->min = vertices[0].pos;
    bounds->max = vertices[0].pos;
    for (DWORD i = 1; i < num_vertices; i++) {
        VECTOR3 p = vertices[i].pos;
        bounds->min.x = MIN(bounds->min.x, p.x);
        bounds->min.y = MIN(bounds->min.y, p.y);
        bounds->min.z = MIN(bounds->min.z, p.z);
        bounds->max.x = MAX(bounds->max.x, p.x);
        bounds->max.y = MAX(bounds->max.y, p.y);
        bounds->max.z = MAX(bounds->max.z, p.z);
    }
    return true;
}

static void M2_AddBatch(m2Model_t *model,
                        BYTE const *m2_data,
                        DWORD m2_size,
                        m2GeometryInfo_t const *geom,
                        m2VertexDisk_t const *m2_vertices,
                        WORD const *skin_vertices,
                        DWORD skin_vertex_count,
                        WORD const *skin_indices,
                        DWORD skin_index_count,
                        m2Ubyte4_t const *skin_bones,
                        DWORD index_start,
                        DWORD index_count,
                        WORD bone_count,
                        WORD bone_combo_index,
                        m2Batch_t const *batch,
                        WORD section_id,
                        LPCSTR modelFilename,
                        BOOL use_texture_lookup) {
    VERTEX *vertices;
    m2ModelBatch_t *render_batch;

    if (!geom || !batch || index_count == 0) {
        return;
    }

    vertices = ri.MemAlloc(sizeof(*vertices) * index_count);
    if (!vertices) {
        return;
    }

    FOR_LOOP(i, index_count) {
        DWORD skin_index = index_start + i;
        DWORD vertex_lookup;
        DWORD vertex_index;

        if (skin_index >= skin_index_count) {
            memset(&vertices[i], 0, sizeof(vertices[i]));
            vertices[i].color = COLOR32_WHITE;
            continue;
        }
        vertex_lookup = skin_indices[skin_index];
        if (vertex_lookup >= skin_vertex_count) {
            memset(&vertices[i], 0, sizeof(vertices[i]));
            vertices[i].color = COLOR32_WHITE;
            continue;
        }
        vertex_index = skin_vertices[vertex_lookup];
        if (vertex_index >= (DWORD)geom->vertices.size) {
            memset(&vertices[i], 0, sizeof(vertices[i]));
            vertices[i].color = COLOR32_WHITE;
            continue;
        }
        vertices[i] = M2_MakeVertex(&m2_vertices[vertex_index]);
        if (skin_bones) {
            memcpy(vertices[i].skin, skin_bones[vertex_lookup].v, sizeof(skin_bones[vertex_lookup].v));
        }
    }

    render_batch = ri.MemAlloc(sizeof(*render_batch));
    memset(render_batch, 0, sizeof(*render_batch));
    render_batch->buffer = R_MakeVertexArrayObject(vertices, index_count);
    render_batch->texture = M2_TextureForBatch(m2_data, m2_size, geom, batch, modelFilename, section_id, use_texture_lookup);
    render_batch->num_vertices = index_count;
    render_batch->bone_count = bone_count;
    render_batch->bone_combo_index = bone_combo_index;
    ADD_TO_LIST(render_batch, model->batches);
    model->num_batches++;
    ri.MemFree(vertices);
}

static BOOL M2_LoadSkinData(LPCSTR modelFilename,
                            LPBYTE *skin_data,
                            DWORD *skin_size,
                            PATHSTR skin_path) {
    int read_size;

    if (!skin_data || !skin_size || !M2_SkinPath(modelFilename, skin_path, sizeof(PATHSTR))) {
        return false;
    }

    read_size = ri.FS_ReadFile(skin_path, (void **)skin_data);
    if (read_size <= 0 && M2_PathHasExtension(modelFilename, ".mdx")) {
        PATHSTR m2_path;
        if (M2_CopyWithExtension(modelFilename, ".m2", m2_path, sizeof(m2_path)) &&
            M2_SkinPath(m2_path, skin_path, sizeof(PATHSTR))) {
            read_size = ri.FS_ReadFile(skin_path, (void **)skin_data);
        }
    }
    if (read_size <= 0 || !*skin_data) {
        *skin_size = 0;
        return false;
    }
    *skin_size = (DWORD)read_size;
    return true;
}

static BOOL M2_InitLegacyGeometry(BYTE const *m2_base,
                                  DWORD m2_size,
                                  m2GeometryInfo_t *geom,
                                  m2LegacyView_t const **view) {
    m2HeaderLegacy_t const *legacy;

    if (!m2_base || m2_size < sizeof(*legacy) || !geom || !view) {
        return false;
    }

    legacy = (m2HeaderLegacy_t const *)m2_base;
    *geom = (m2GeometryInfo_t){
        .vertices = legacy->vertices,
        .textures = legacy->textures,
        .texture_lookup_table = legacy->texture_lookup_table,
        .bounding_box = legacy->bounding_box,
    };
    *view = M2_ArrayPtr(m2_base, m2_size, legacy->views, sizeof(**view));
    return *view != NULL;
}

static BOOL M2_InitModernGeometry(BYTE const *m2_base,
                                  DWORD m2_size,
                                  m2GeometryInfo_t *geom,
                                  m2Header_t const **header) {
    m2Header_t const *modern;

    if (!m2_base || m2_size < sizeof(*modern) || !geom || !header) {
        return false;
    }

    modern = (m2Header_t const *)m2_base;
    *geom = (m2GeometryInfo_t){
        .vertices = modern->vertices,
        .textures = modern->textures,
        .texture_lookup_table = modern->texture_lookup_table,
        .bounding_box = modern->bounding_box,
    };
    *header = modern;
    return true;
}

static BOOL M2_GetSectionRange(void const *sections,
                               BOOL legacy_sections,
                               DWORD section_count,
                               WORD section_index,
                               WORD *section_id,
                               DWORD *index_start,
                               DWORD *index_count,
                               WORD *bone_count,
                               WORD *bone_combo_index) {
    if (!sections || !index_start || !index_count || !bone_count || !bone_combo_index || section_index >= section_count) {
        return false;
    }
    if (legacy_sections) {
        m2SkinSectionLegacy_t const *section = &((m2SkinSectionLegacy_t const *)sections)[section_index];
        if (section_id) {
            *section_id = section->skin_section_id;
        }
        *index_start = section->index_start;
        *index_count = section->index_count;
        *bone_count = section->bone_count;
        *bone_combo_index = section->bone_combo_index;
    } else {
        m2SkinSection_t const *section = &((m2SkinSection_t const *)sections)[section_index];
        if (section_id) {
            *section_id = section->skin_section_id;
        }
        *index_start = section->index_start;
        *index_count = section->index_count;
        *bone_count = section->bone_count;
        *bone_combo_index = section->bone_combo_index;
    }
    return true;
}

static BOOL M2_IsCharacterModelPath(LPCSTR model_path) {
    LPCSTR character;
    LPCSTR race_end;
    LPCSTR gender;
    size_t gender_len;

    if (!model_path) {
        return false;
    }

    character = strcasestr(model_path, "Character\\");
    if (!character) {
        character = strcasestr(model_path, "Character/");
    }
    if (!character) {
        return false;
    }

    race_end = strpbrk(character + strlen("Character\\"), "\\/");
    if (!race_end || !race_end[1]) {
        return false;
    }

    gender = race_end + 1;
    gender_len = strcspn(gender, "\\/.");
    return (gender_len == 4 && !strncasecmp(gender, "Male", 4)) ||
           (gender_len == 6 && !strncasecmp(gender, "Female", 6));
}

static BOOL M2_DefaultCharacterGeosetVisible(WORD section_id) {
    static WORD const common_orc_male_outfit[] = {
        0,    /* base body */
        401,  /* simple gloves */
        501,  /* simple boots */
        702,  /* ears */
        802,  /* bracers */
        902,  /* lower-leg clothing */
        1002, /* chest */
        1102, /* lower chest */
        1202, /* shirt/tabard layer */
        1302, /* belt/upper-leg layer */
        1501  /* normal pelvis shape */
    };

    if (section_id < 400) {
        return true;
    }

    FOR_LOOP(i, sizeof(common_orc_male_outfit) / sizeof(common_orc_male_outfit[0])) {
        if (section_id == common_orc_male_outfit[i]) {
            return true;
        }
    }
    return false;
}

static void M2_FreeModelData(m2Model_t *model) {
    if (!model) {
        return;
    }
    if (model->bone_matrices) {
        ri.MemFree(model->bone_matrices);
        model->bone_matrices = NULL;
    }
    if (model->data) {
        ri.MemFree(model->data);
        model->data = NULL;
    }
}

static BOOL M2_CopyModelData(m2Model_t *model, BYTE const *m2_base, DWORD m2_size, BOOL legacy_header) {
    m2Array_t bones;
    m2Array_t sequences;
    m2Array_t bone_lookup_table;

    if (!model || !m2_base || m2_size < sizeof(m2Header_t)) {
        return false;
    }

    model->data = ri.MemAlloc(m2_size);
    if (!model->data) {
        return false;
    }
    memcpy(model->data, m2_base, m2_size);
    model->data_size = m2_size;
    model->header = (m2Header_t *)model->data;
    model->classic_sequences = model->header->version <= 263;
    model->sequence_stride = model->classic_sequences ? sizeof(m2SequenceClassic_t) : sizeof(m2SequenceModern_t);
    model->classic_bones = model->header->version <= 263;
    model->bone_stride = model->classic_bones ? sizeof(m2CompBoneClassic_t) : sizeof(m2CompBoneModern_t);
    model->classic_attachments = model->header->version <= 263;
    model->attachment_stride = model->classic_attachments ? sizeof(m2AttachmentClassic_t) : sizeof(m2AttachmentModern_t);

    if (legacy_header) {
        m2HeaderLegacy_t *legacy = (m2HeaderLegacy_t *)model->data;
        model->global_loops = legacy->global_loops;
        bones = legacy->bones;
        sequences = legacy->sequences;
        bone_lookup_table = legacy->bone_lookup_table;
        model->attachments = legacy->attachments;
        model->attachment_lookup = legacy->attachment_lookup;
    } else {
        model->global_loops = model->header->global_loops;
        bones = model->header->bones;
        sequences = model->header->sequences;
        bone_lookup_table = model->header->bone_lookup_table;
        model->attachments = model->header->attachments;
        model->attachment_lookup = model->header->attachment_lookup;
    }

    model->bones = M2_ModelArrayPtr(model, bones, model->bone_stride);
    model->sequences = M2_ModelArrayPtr(model, sequences, model->sequence_stride);
    model->bone_lookup_table = M2_ModelArrayPtr(model, bone_lookup_table, sizeof(*model->bone_lookup_table));
    model->bone_count = model->bones ? (DWORD)bones.size : 0;
    model->sequence_count = model->sequences ? (DWORD)sequences.size : 0;
    model->bone_lookup_count = model->bone_lookup_table ? (DWORD)bone_lookup_table.size : 0;

    if (model->bone_count) {
        model->bone_matrices = ri.MemAlloc(sizeof(*model->bone_matrices) * model->bone_count);
        if (!model->bone_matrices) {
            M2_FreeModelData(model);
            return false;
        }
        FOR_LOOP(i, model->bone_count) {
            Matrix4_identity(&model->bone_matrices[i]);
        }
    }

    return true;
}

m2Model_t *R_LoadModelM2(LPCSTR modelFilename, void *buffer, DWORD size) {
    m2Header_t const *modern_header = NULL;
    m2GeometryInfo_t geom;
    BYTE const *m2_base = buffer;
    DWORD m2_size = size;
    m2VertexDisk_t const *m2_vertices;
    LPBYTE skin_data = NULL;
    DWORD skin_size = 0;
    PATHSTR skin_path;
    m2SkinHeader_t const *skin;
    m2LegacyView_t const *legacy_view = NULL;
    WORD const *skin_vertices;
    WORD const *skin_indices;
    void const *sections;
    m2Batch_t const *batches;
    m2Ubyte4_t const *skin_bones;
    m2Model_t *model;
    DWORD batch_count;
    DWORD section_count;
    DWORD skin_vertex_count;
    DWORD skin_index_count;
    BOOL using_legacy_view = false;
    BOOL filter_character_geosets;

    if (!buffer || size < sizeof(DWORD)) {
        return M2_CreateFallbackModel(modelFilename, "missing model data");
    }

    if (*(DWORD *)buffer == ID_MD21) {
        m2_base = M2_FindChunk(buffer, size, "MD21", &m2_size);
        if (!m2_base) {
            m2_base = buffer;
            m2_size = size;
        }
    } else if (*(DWORD *)buffer == ID_12DM) {
        m2_base = M2_FindChunk(buffer, size, "12DM", &m2_size);
    }
    if (!m2_base || m2_size < sizeof(DWORD) * 2) {
        return M2_CreateFallbackModel(modelFilename, "truncated header");
    }

    if (*(DWORD *)m2_base != ID_MD20) {
        return M2_CreateFallbackModel(modelFilename, "bad MD20 header");
    }

    if (!M2_InitModernGeometry(m2_base, m2_size, &geom, &modern_header)) {
        return M2_CreateFallbackModel(modelFilename, "invalid modern header");
    }

    if (M2_LoadSkinData(modelFilename, &skin_data, &skin_size, skin_path) &&
        skin_size >= sizeof(*skin)) {
        skin = (m2SkinHeader_t const *)skin_data;
        if (skin->magic == MAKEFOURCC('S', 'K', 'I', 'N')) {
            skin_vertices = M2_ArrayPtr(skin_data, skin_size, skin->vertices, sizeof(*skin_vertices));
            skin_indices = M2_ArrayPtr(skin_data, skin_size, skin->indices, sizeof(*skin_indices));
            skin_bones = M2_ArrayPtr(skin_data, skin_size, skin->bones, sizeof(*skin_bones));
            sections = M2_ArrayPtr(skin_data, skin_size, skin->sections, sizeof(m2SkinSection_t));
            batches = M2_ArrayPtr(skin_data, skin_size, skin->batches, sizeof(*batches));
            batch_count = (DWORD)skin->batches.size;
            section_count = (DWORD)skin->sections.size;
            skin_vertex_count = (DWORD)skin->vertices.size;
            skin_index_count = (DWORD)skin->indices.size;
        } else {
            M2_LogFallback(modelFilename, "bad skin magic; trying legacy embedded view");
            ri.FS_FreeFile(skin_data);
            skin_data = NULL;
            skin_vertices = NULL;
            skin_indices = NULL;
            skin_bones = NULL;
            sections = NULL;
            batches = NULL;
            batch_count = section_count = skin_vertex_count = skin_index_count = 0;
        }
    } else {
        if (skin_data) {
            ri.FS_FreeFile(skin_data);
            skin_data = NULL;
        }
        skin_vertices = NULL;
        skin_indices = NULL;
        skin_bones = NULL;
        sections = NULL;
        batches = NULL;
        batch_count = section_count = skin_vertex_count = skin_index_count = 0;
    }

    if (!skin_vertices || !skin_indices || !sections || !batches) {
        DWORD version = modern_header ? modern_header->version : 0;

        if (version <= 260 && M2_InitLegacyGeometry(m2_base, m2_size, &geom, &legacy_view)) {
            skin_vertices = M2_ArrayPtr(m2_base, m2_size, legacy_view->vertices, sizeof(*skin_vertices));
            skin_indices = M2_ArrayPtr(m2_base, m2_size, legacy_view->indices, sizeof(*skin_indices));
            skin_bones = M2_ArrayPtr(m2_base, m2_size, legacy_view->bones, sizeof(*skin_bones));
            sections = M2_ArrayPtr(m2_base, m2_size, legacy_view->sections, sizeof(m2SkinSectionLegacy_t));
            batches = M2_ArrayPtr(m2_base, m2_size, legacy_view->batches, sizeof(*batches));
            batch_count = (DWORD)legacy_view->batches.size;
            section_count = (DWORD)legacy_view->sections.size;
            skin_vertex_count = (DWORD)legacy_view->vertices.size;
            skin_index_count = (DWORD)legacy_view->indices.size;
            using_legacy_view = true;
        }
    }

    m2_vertices = M2_ArrayPtr(m2_base, m2_size, geom.vertices, sizeof(*m2_vertices));
    if (!m2_vertices || !skin_vertices || !skin_indices || !sections || !batches) {
        if (skin_data) {
            ri.FS_FreeFile(skin_data);
        }
        return M2_CreateFallbackModel(modelFilename,
                                      using_legacy_view ? "invalid legacy embedded view" : "missing skin profile");
    }

    model = ri.MemAlloc(sizeof(*model));
    memset(model, 0, sizeof(*model));
    model->bounds = (BOX3){ geom.bounding_box.min, geom.bounding_box.max };
    model->has_geometry_bounds = M2_CalculateGeometryBounds(m2_vertices,
                                                            (DWORD)geom.vertices.size,
                                                            &model->geometry_bounds);
    filter_character_geosets = M2_IsCharacterModelPath(modelFilename);
    if (!M2_CopyModelData(model, m2_base, m2_size, using_legacy_view)) {
        if (skin_data) {
            ri.FS_FreeFile(skin_data);
        }
        M2_FreeModelData(model);
        ri.MemFree(model);
        return M2_CreateFallbackModel(modelFilename, "failed to copy animation data");
    }

    FOR_LOOP(i, batch_count) {
        m2Batch_t const *batch = &batches[i];
        DWORD index_start;
        DWORD index_count;
        WORD bone_count;
        WORD bone_combo_index;
        WORD section_id = 0;
        if (!M2_GetSectionRange(sections,
                                using_legacy_view,
                                section_count,
                                batch->skin_section_index,
                                &section_id,
                                &index_start,
                                &index_count,
                                &bone_count,
                                &bone_combo_index)) {
            continue;
        }
        if (filter_character_geosets && !M2_DefaultCharacterGeosetVisible(section_id)) {
            continue;
        }
        M2_AddBatch(model,
                    m2_base,
                    m2_size,
                    &geom,
                    m2_vertices,
                    skin_vertices,
                    skin_vertex_count,
                    skin_indices,
                    skin_index_count,
                    skin_bones,
                    index_start,
                    index_count,
                    bone_count,
                    bone_combo_index,
                    batch,
                    section_id,
                    modelFilename,
                    true);
    }

    if (skin_data) {
        ri.FS_FreeFile(skin_data);
    }
    if (!model->batches) {
        M2_FreeModelData(model);
        ri.MemFree(model);
        return M2_CreateFallbackModel(modelFilename, using_legacy_view ? "legacy view produced no batches" : "skin produced no batches");
    }
    return model;
}

void M2_RenderModel(renderEntity_t const *entity, m2Model_t const *model, LPCMATRIX4 transform) {
    MATRIX3 normal_matrix;
    m2ModelBatch_t *batch;
    LPSHADER shader;

    if (!entity || !model || !transform) {
        return;
    }
    if (!Frustum_ContainsBox(&tr.viewDef.frustum, &model->bounds, transform)) {
        return;
    }

    shader = M2_Shader();
    M2_CalculateBoneMatrices(model, entity);
    Matrix3_normal(&normal_matrix, transform);
    R_Call(glUseProgram, shader->progid);
    R_Call(glUniformMatrix4fv, shader->uViewProjectionMatrix, 1, GL_FALSE, tr.viewDef.viewProjectionMatrix.v);
    R_Call(glUniformMatrix4fv, shader->uTextureMatrix, 1, GL_FALSE, tr.viewDef.textureMatrix.v);
    R_Call(glUniformMatrix4fv, shader->uModelMatrix, 1, GL_FALSE, transform->v);
    R_Call(glUniformMatrix4fv, shader->uLightMatrix, 1, GL_FALSE, tr.viewDef.lightMatrix.v);
    R_Call(glUniformMatrix3fv, shader->uNormalMatrix, 1, GL_TRUE, normal_matrix.v);
    R_Call(glEnable, GL_DEPTH_TEST);
    R_Call(glDepthMask, GL_TRUE);
    R_Call(glDisable, GL_BLEND);

    for (batch = model->batches; batch; batch = batch->next) {
        M2_UploadBatchBones(model, batch, shader);
        R_BindTexture(batch->texture ? batch->texture : tr.texture[TEX_WHITE], 0);
#ifdef USE_SHADOWMAPS
        R_BindTexture(tr.texture[TEX_SHADOWMAP], 1);
#endif
        R_BindTexture(tr.texture[TEX_WHITE], 2);
        R_DrawBuffer(batch->buffer, batch->num_vertices);
    }
}

BOOL M2_AttachmentMatrix(m2Model_t const *model,
                         DWORD attachment_id,
                         LPCMATRIX4 model_matrix,
                         LPMATRIX4 out) {
    BYTE const *attachments;
    WORD const *lookup;
    DWORD attachment_index = 0xFFFFu;
    WORD bone_index;
    VECTOR3 position;
    MATRIX4 local;

    if (!model || !model_matrix || !out || !model->bone_matrices || !model->bones) {
        return false;
    }

    attachments = M2_ModelArrayPtr(model, model->attachments, model->attachment_stride);
    if (!attachments || model->attachments.size <= 0) {
        return false;
    }

    lookup = M2_ModelArrayPtr(model, model->attachment_lookup, sizeof(*lookup));
    if (lookup && attachment_id < (DWORD)model->attachment_lookup.size) {
        attachment_index = lookup[attachment_id];
    }
    if (attachment_index >= (DWORD)model->attachments.size) {
        FOR_LOOP(i, (DWORD)model->attachments.size) {
            DWORD id = model->classic_attachments
                ? ((m2AttachmentClassic_t const *)(attachments + i * model->attachment_stride))->attachment_id
                : ((m2AttachmentModern_t const *)(attachments + i * model->attachment_stride))->attachment_id;
            if (id == attachment_id) {
                attachment_index = i;
                break;
            }
        }
    }
    if (attachment_index >= (DWORD)model->attachments.size) {
        return false;
    }

    if (model->classic_attachments) {
        m2AttachmentClassic_t const *attachment = (m2AttachmentClassic_t const *)(attachments + attachment_index * model->attachment_stride);
        bone_index = attachment->bone_index;
        position = attachment->position;
    } else {
        m2AttachmentModern_t const *attachment = (m2AttachmentModern_t const *)(attachments + attachment_index * model->attachment_stride);
        bone_index = attachment->bone_index;
        position = attachment->position;
    }

    if (bone_index >= model->bone_count) {
        return false;
    }

    local = model->bone_matrices[bone_index];
    Matrix4_translate(&local, &position);
    Matrix4_multiply(model_matrix, &local, out);
    return true;
}

FLOAT M2_GroundOffset(m2Model_t const *model) {
    if (!model || !model->has_geometry_bounds || model->geometry_bounds.min.z >= 0.0f) {
        return 0.0f;
    }
    return -model->geometry_bounds.min.z;
}

void M2_Release(m2Model_t *model) {
    m2ModelBatch_t *batch;

    if (!model) {
        return;
    }
    batch = model->batches;
    while (batch) {
        m2ModelBatch_t *next = batch->next;
        R_ReleaseVertexArrayObject(batch->buffer);
        ri.MemFree(batch);
        batch = next;
    }
    M2_FreeModelData(model);
    ri.MemFree(model);
}
