#include "../r_local.h"
#include <strings.h>

typedef struct {
    int32_t size;
    int32_t offset;
} m2Array_t;

typedef struct {
    VECTOR3 min;
    VECTOR3 max;
} m2Box_t;

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
    struct m2ModelBatch_s *next;
} m2ModelBatch_t;

struct m2Model_s {
    m2ModelBatch_t *batches;
    DWORD num_batches;
    BOX3 bounds;
    BOX3 geometry_bounds;
    BOOL has_geometry_bounds;
};

typedef struct {
    m2Array_t vertices;
    m2Array_t textures;
    m2Array_t texture_lookup_table;
    m2Box_t bounding_box;
} m2GeometryInfo_t;

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

static LPTEXTURE M2_TextureForBatch(BYTE const *m2_data,
                                    DWORD m2_size,
                                    m2GeometryInfo_t const *geom,
                                    m2Batch_t const *batch,
                                    LPCSTR modelFilename,
                                    BOOL use_texture_lookup) {
    SHORT const *texture_lookup;
    m2TextureDisk_t const *texture;
    SHORT texture_index;
    LPCSTR texture_path;
    PATHSTR replacement_path;

    if (!geom || !batch || batch->texture_count == 0) {
        return tr.texture[TEX_WHITE];
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
        return tr.texture[TEX_WHITE];
    }
    return R_LoadTexture(texture_path);
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
                        DWORD index_start,
                        DWORD index_count,
                        m2Batch_t const *batch,
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
    }

    render_batch = ri.MemAlloc(sizeof(*render_batch));
    memset(render_batch, 0, sizeof(*render_batch));
    render_batch->buffer = R_MakeVertexArrayObject(vertices, index_count);
    render_batch->texture = M2_TextureForBatch(m2_data, m2_size, geom, batch, modelFilename, use_texture_lookup);
    render_batch->num_vertices = index_count;
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
                               DWORD *index_start,
                               DWORD *index_count) {
    if (!sections || !index_start || !index_count || section_index >= section_count) {
        return false;
    }
    if (legacy_sections) {
        m2SkinSectionLegacy_t const *section = &((m2SkinSectionLegacy_t const *)sections)[section_index];
        *index_start = section->index_start;
        *index_count = section->index_count;
    } else {
        m2SkinSection_t const *section = &((m2SkinSection_t const *)sections)[section_index];
        *index_start = section->index_start;
        *index_count = section->index_count;
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
    m2Model_t *model;
    DWORD batch_count;
    DWORD section_count;
    DWORD skin_vertex_count;
    DWORD skin_index_count;
    BOOL using_legacy_view = false;

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
        sections = NULL;
        batches = NULL;
        batch_count = section_count = skin_vertex_count = skin_index_count = 0;
    }

    if (!skin_vertices || !skin_indices || !sections || !batches) {
        DWORD version = modern_header ? modern_header->version : 0;

        if (version <= 260 && M2_InitLegacyGeometry(m2_base, m2_size, &geom, &legacy_view)) {
            skin_vertices = M2_ArrayPtr(m2_base, m2_size, legacy_view->vertices, sizeof(*skin_vertices));
            skin_indices = M2_ArrayPtr(m2_base, m2_size, legacy_view->indices, sizeof(*skin_indices));
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

    FOR_LOOP(i, batch_count) {
        m2Batch_t const *batch = &batches[i];
        DWORD index_start;
        DWORD index_count;
        if (!M2_GetSectionRange(sections,
                                using_legacy_view,
                                section_count,
                                batch->skin_section_index,
                                &index_start,
                                &index_count)) {
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
                    index_start,
                    index_count,
                    batch,
                    modelFilename,
                    !using_legacy_view);
    }

    if (skin_data) {
        ri.FS_FreeFile(skin_data);
    }
    if (!model->batches) {
        ri.MemFree(model);
        return M2_CreateFallbackModel(modelFilename, using_legacy_view ? "legacy view produced no batches" : "skin produced no batches");
    }
    return model;
}

void M2_RenderModel(renderEntity_t const *entity, m2Model_t const *model, LPCMATRIX4 transform) {
    MATRIX3 normal_matrix;
    m2ModelBatch_t *batch;

    if (!entity || !model || !transform) {
        return;
    }
    if (!Frustum_ContainsBox(&tr.viewDef.frustum, &model->bounds, transform)) {
        return;
    }

    Matrix3_normal(&normal_matrix, transform);
    R_Call(glUseProgram, tr.shader[SHADER_DEFAULT]->progid);
    R_Call(glUniformMatrix4fv, tr.shader[SHADER_DEFAULT]->uViewProjectionMatrix, 1, GL_FALSE, tr.viewDef.viewProjectionMatrix.v);
    R_Call(glUniformMatrix4fv, tr.shader[SHADER_DEFAULT]->uTextureMatrix, 1, GL_FALSE, tr.viewDef.textureMatrix.v);
    R_Call(glUniformMatrix4fv, tr.shader[SHADER_DEFAULT]->uModelMatrix, 1, GL_FALSE, transform->v);
    R_Call(glUniformMatrix4fv, tr.shader[SHADER_DEFAULT]->uLightMatrix, 1, GL_FALSE, tr.viewDef.lightMatrix.v);
    R_Call(glUniformMatrix3fv, tr.shader[SHADER_DEFAULT]->uNormalMatrix, 1, GL_TRUE, normal_matrix.v);
    R_Call(glEnable, GL_DEPTH_TEST);
    R_Call(glDepthMask, GL_TRUE);
    R_Call(glDisable, GL_BLEND);

    for (batch = model->batches; batch; batch = batch->next) {
        R_BindTexture(batch->texture ? batch->texture : tr.texture[TEX_WHITE], 0);
        R_DrawBuffer(batch->buffer, batch->num_vertices);
    }
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
    ri.MemFree(model);
}
