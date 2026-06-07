#include "r_sc2map.h"

#include "games/starcraft-2/common/sc2_map.c"
#include "games/starcraft-2/renderer/m3/r_m3.h"
#define SC2_REUSE_WAR3_CLIFF_BAKER
#include "games/warcraft-3/renderer/w3m/r_war3map_cliffs.c"
#undef SC2_REUSE_WAR3_CLIFF_BAKER
#include "games/warcraft-3/renderer/w3m/r_terrain_layers.c"

#define SC2_TERRAIN_BLEND_LAYERS    8
#define SC2_M3_MAX_BONES            128
#define SC2_CLIFF_BLOCK_SPAN        2u      /* cliff cells are 2x2 grid units */
#define SC2_CLIFF_BLOCK_ORIGIN(X)   ((X) & ~(SC2_CLIFF_BLOCK_SPAN - 1u))
#define SC2_CLIFF_BLOCK_CENTER(X)   ((X) + 1u)  /* centre grid coord within a 2-unit block */
#define SC2_CLIFF_LEVEL_PACKED_MIN  0x40u   /* packed format threshold; values >= this are shifted */
#define SC2_CLIFF_LEVEL_SHIFT       6
#define SC2_CELL_FLAG_TERRAIN_MASK  0x0fu
#define SC2_CELL_FLAG_CLIFF         0x03u
#define SC2_GROUND_VERTICES_PER_CELL 6u
#define SC2_TERRAIN_UV_SCALE        8.0f
#define SC2_CLIFF_VARIANT_MASK      3u
#define SC2_M3_UV_SCALE             2048.0f /* M3 UV coords are stored as 1/2048 fixed-point */
#define SC2_M3_NORMAL_SCALE         (2.0f / 255.0f)  /* byte-packed snorm: val/255*2-1 */
#define SC2_EPSILON                 0.001f  /* near-zero threshold for spans and scale guards */
#define SC2_TRACE_EPSILON           0.0001f /* near-zero direction threshold in ray-slab test */
#define SC2_TRACE_INF               1.0e30f /* infinity sentinel for axis-aligned ray marching */
#define SC2_CLIFF_MIN_SPAN          1.0f    /* minimum z-span fallback when terrain is flat */
#define SC2_CLIFF_WIDTH(MAP)        (MAX(1, ((MAP)->width + 1) / 2))

/* BL=0, BR=1, TR=2, TL=3 — matches SC2 cliff model corner convention */
#define SC2_CLIFF_BLOCK_LEVELS(LEVEL, MAP, X, Y) \
do { \
    (LEVEL)[0] = r_sc2_cliff_level_at_grid((MAP), (X),                          (Y)); \
    (LEVEL)[1] = r_sc2_cliff_level_at_grid((MAP), (X) + SC2_CLIFF_BLOCK_SPAN,   (Y)); \
    (LEVEL)[2] = r_sc2_cliff_level_at_grid((MAP), (X) + SC2_CLIFF_BLOCK_SPAN,   (Y) + SC2_CLIFF_BLOCK_SPAN); \
    (LEVEL)[3] = r_sc2_cliff_level_at_grid((MAP), (X),                          (Y) + SC2_CLIFF_BLOCK_SPAN); \
} while (0)

/* BL=0, BR=1, TL=2, TR=3 — all four corners; order only matters for min/max sweeps */
#define SC2_CLIFF_BLOCK_HEIGHTS(HEIGHT, MAP, X, Y) \
do { \
    (HEIGHT)[0] = r_sc2_height_at_grid((MAP), (X),                        (Y)); \
    (HEIGHT)[1] = r_sc2_height_at_grid((MAP), (X) + SC2_CLIFF_BLOCK_SPAN, (Y)); \
    (HEIGHT)[2] = r_sc2_height_at_grid((MAP), (X),                        (Y) + SC2_CLIFF_BLOCK_SPAN); \
    (HEIGHT)[3] = r_sc2_height_at_grid((MAP), (X) + SC2_CLIFF_BLOCK_SPAN, (Y) + SC2_CLIFF_BLOCK_SPAN); \
} while (0)

extern LPCSTR vs_default;
extern m3SequenceTimeline_t const *M3_FindAnimationAtTime(m3Model_t const *model, DWORD time, DWORD *localtime);
extern VECTOR3 M3_GetVector3AnimValue(m3Model_t const *model,
                                      m3SequenceTimeline_t const *timeline,
                                      m3Vector3AnimRef_t const *animref,
                                      DWORD time);
extern VECTOR4 M3_GetVector4AnimValue(m3Model_t const *model,
                                      m3SequenceTimeline_t const *timeline,
                                      m3Vector4AnimRef_t const *animref,
                                      DWORD time);

static LPMAPSEGMENT sc2_terrain_segment;
static LPSHADER sc2_terrain_shader;
static GLint sc2_u_layer[SC2_TERRAIN_BLEND_LAYERS];
static GLint sc2_u_mask[SC2_TERRAIN_BLEND_LAYERS];
static GLint sc2_u_layer_count = -1;
static LPTEXTURE sc2_terrain_textures[SC2_TERRAIN_BLEND_LAYERS];
static LPTEXTURE sc2_terrain_masks[SC2_TERRAIN_BLEND_LAYERS];
static DWORD sc2_num_terrain_layers;

typedef struct sc2CliffModel_s {
    PATHSTR path;
    LPCMODEL model;
    struct sc2CliffModel_s *next;
} sc2CliffModel_t;

static sc2CliffModel_t *sc2_cliff_models;

static void r_sc2_release_cliff_models(void);

static LPCSTR sc2_fs_terrain =
"#version 140\n"
"in vec2 v_texcoord;\n"
"in vec2 v_texcoord2;\n"
"in vec4 v_color;\n"
"out vec4 o_color;\n"
"uniform sampler2D uLayer0;\n"
"uniform sampler2D uLayer1;\n"
"uniform sampler2D uLayer2;\n"
"uniform sampler2D uLayer3;\n"
"uniform sampler2D uLayer4;\n"
"uniform sampler2D uLayer5;\n"
"uniform sampler2D uLayer6;\n"
"uniform sampler2D uLayer7;\n"
"uniform sampler2D uMask0;\n"
"uniform sampler2D uMask1;\n"
"uniform sampler2D uMask2;\n"
"uniform sampler2D uMask3;\n"
"uniform sampler2D uMask4;\n"
"uniform sampler2D uMask5;\n"
"uniform sampler2D uMask6;\n"
"uniform sampler2D uMask7;\n"
"uniform int uLayerCount;\n"
"vec2 get_mask_coord() {\n"
"    return clamp(v_texcoord2 * 0.5 + vec2(0.5), vec2(0.0), vec2(1.0));\n"
"}\n"
"void main() {\n"
"    vec2 mc = get_mask_coord();\n"
"    float w0 = texture(uMask0, mc).r;\n"
"    float w1 = texture(uMask1, mc).r;\n"
"    float w2 = texture(uMask2, mc).r;\n"
"    float w3 = texture(uMask3, mc).r;\n"
"    float w4 = texture(uMask4, mc).r;\n"
"    float w5 = texture(uMask5, mc).r;\n"
"    float w6 = texture(uMask6, mc).r;\n"
"    float w7 = texture(uMask7, mc).r;\n"
"    vec4 color = texture(uLayer0, v_texcoord) * w0;\n"
"    float total = w0;\n"
"    if (uLayerCount > 1) { color += texture(uLayer1, v_texcoord) * w1; total += w1; }\n"
"    if (uLayerCount > 2) { color += texture(uLayer2, v_texcoord) * w2; total += w2; }\n"
"    if (uLayerCount > 3) { color += texture(uLayer3, v_texcoord) * w3; total += w3; }\n"
"    if (uLayerCount > 4) { color += texture(uLayer4, v_texcoord) * w4; total += w4; }\n"
"    if (uLayerCount > 5) { color += texture(uLayer5, v_texcoord) * w5; total += w5; }\n"
"    if (uLayerCount > 6) { color += texture(uLayer6, v_texcoord) * w6; total += w6; }\n"
"    if (uLayerCount > 7) { color += texture(uLayer7, v_texcoord) * w7; total += w7; }\n"
"    color = total > 0.001 ? color / total : texture(uLayer0, v_texcoord);\n"
"    color.rgb *= v_color.rgb;\n"
"    color.a = 1.0;\n"
"    o_color = color;\n"
"}\n";

static void r_sc2_init_terrain_shader(void) {
    static LPCSTR layer_names[SC2_TERRAIN_BLEND_LAYERS] = {
        "uLayer0", "uLayer1", "uLayer2", "uLayer3",
        "uLayer4", "uLayer5", "uLayer6", "uLayer7",
    };
    static LPCSTR mask_names[SC2_TERRAIN_BLEND_LAYERS] = {
        "uMask0", "uMask1", "uMask2", "uMask3", "uMask4", "uMask5", "uMask6", "uMask7",
    };

    if (sc2_terrain_shader) {
        return;
    }
    sc2_terrain_shader = R_InitShader(vs_default, sc2_fs_terrain);
    if (!sc2_terrain_shader) {
        return;
    }
    R_Call(glUseProgram, sc2_terrain_shader->progid);
    FOR_LOOP(i, SC2_TERRAIN_BLEND_LAYERS) {
        sc2_u_layer[i] = glGetUniformLocation(sc2_terrain_shader->progid, layer_names[i]);
        R_Call(glUniform1i, sc2_u_layer[i], (GLint)i);
        sc2_u_mask[i] = glGetUniformLocation(sc2_terrain_shader->progid, mask_names[i]);
        R_Call(glUniform1i, sc2_u_mask[i], (GLint)(SC2_TERRAIN_BLEND_LAYERS + i));
    }
    sc2_u_layer_count = glGetUniformLocation(sc2_terrain_shader->progid, "uLayerCount");
}

static HANDLE r_sc2_read_file(LPCSTR filename, LPDWORD size) {
    void *buffer = NULL;
    int result = ri.FS_ReadFile(filename, &buffer);
    if (result < 0) {
        if (size) *size = 0;
        return NULL;
    }
    if (size) *size = (DWORD)result;
    return buffer;
}

static void r_sc2_free_file(HANDLE file) {
    ri.FS_FreeFile(file);
}

static void r_sc2_push_vertex_normal(VERTEX *v,
                                     FLOAT x,
                                     FLOAT y,
                                     FLOAT z,
                                     FLOAT u,
                                     FLOAT t,
                                     BYTE alpha,
                                     VECTOR3 normal) {
    v->position = (VECTOR3){ x, y, z };
    v->texcoord = (VECTOR2){ u, t };
    v->normal = normal;
    v->color = (COLOR32){ 255, 255, 255, alpha };
}

static void r_sc2_push_vertex(VERTEX *v, FLOAT x, FLOAT y, FLOAT z, FLOAT u, FLOAT t, BYTE alpha) {
    r_sc2_push_vertex_normal(v, x, y, z, u, t, alpha, (VECTOR3){ 0.0f, 0.0f, 1.0f });
}

static FLOAT r_sc2_height_at_grid(sc2Map_t const *map, DWORD x, DWORD y) {
    if (!map->height_map || !map->height_map_width || !map->height_map_height) {
        return 0.0f;
    }
    x = MIN(map->height_map_width - 1, x);
    y = MIN(map->height_map_height - 1, y);
    return map->height_map[x + y * map->height_map_width];
}

static USHORT r_sc2_cliff_level_at_grid(sc2Map_t const *map, DWORD x, DWORD y) {
    USHORT value;

    if (!map->cliff_levels || !map->cliff_level_width || !map->cliff_level_height) {
        return 0;
    }
    x = MIN(map->cliff_level_width - 1, x * map->cliff_level_width / MAX(1, map->width));
    y = MIN(map->cliff_level_height - 1, y * map->cliff_level_height / MAX(1, map->height));
    value = map->cliff_levels[x + y * map->cliff_level_width];
    return value >= SC2_CLIFF_LEVEL_PACKED_MIN ? value >> SC2_CLIFF_LEVEL_SHIFT : value;
}

static BYTE r_sc2_cell_flag_at_grid(sc2Map_t const *map, DWORD x, DWORD y) {
    if (!map->cell_flags || !map->cell_flags_width || !map->cell_flags_height) {
        return 0;
    }
    x = MIN(map->cell_flags_width - 1, x * map->cell_flags_width / MAX(1, map->width));
    y = MIN(map->cell_flags_height - 1, y * map->cell_flags_height / MAX(1, map->height));
    return map->cell_flags[x + y * map->cell_flags_width];
}

static BOOL r_sc2_cliff_block_is_flat(sc2Map_t const *map, DWORD x, DWORD y) {
    USHORT level[4];

    SC2_CLIFF_BLOCK_LEVELS(level, map, x, y);
    return level[1] == level[0] && level[2] == level[0] && level[3] == level[0];
}

static BOOL r_sc2_skip_ground_cell(sc2Map_t const *map, DWORD x, DWORD y) {
    if ((r_sc2_cell_flag_at_grid(map, x, y) & SC2_CELL_FLAG_TERRAIN_MASK) != SC2_CELL_FLAG_CLIFF)
        return false;
    return !r_sc2_cliff_block_is_flat(map, SC2_CLIFF_BLOCK_ORIGIN(x), SC2_CLIFF_BLOCK_ORIGIN(y));
}

static void r_sc2_release_layer(LPMAPLAYER layer) {
    while (layer) {
        LPMAPLAYER next = layer->next;
        R_ReleaseVertexArrayObject((LPBUFFER)layer->buffer);
        R_ReleaseTexture((LPTEXTURE)layer->texture);
        ri.MemFree(layer);
        layer = next;
    }
}

static void r_sc2_release_terrain(void) {
    while (sc2_terrain_segment) {
        LPMAPSEGMENT next = sc2_terrain_segment->next;
        r_sc2_release_layer(sc2_terrain_segment->layers);
        ri.MemFree(sc2_terrain_segment);
        sc2_terrain_segment = next;
    }
    FOR_LOOP(i, SC2_TERRAIN_BLEND_LAYERS) {
        SAFE_DELETE(sc2_terrain_textures[i], R_ReleaseTexture);
        SAFE_DELETE(sc2_terrain_masks[i], R_ReleaseTexture);
    }
    r_sc2_release_cliff_models();
    sc2_num_terrain_layers = 0;
}

static void r_sc2_release_cliff_models(void) {
    while (sc2_cliff_models) {
        sc2CliffModel_t *next = sc2_cliff_models->next;
        R_ReleaseModel((LPMODEL)sc2_cliff_models->model);
        ri.MemFree(sc2_cliff_models);
        sc2_cliff_models = next;
    }
}

static void r_sc2_add_layer(LPMAPLAYER *list, LPMAPLAYER layer) {
    LPMAPLAYER *tail = list;

    if (!layer) {
        return;
    }
    while (*tail) {
        tail = &(*tail)->next;
    }
    *tail = layer;
}

static LPTEXTURE r_sc2_build_mask_texture(sc2Map_t const *map, DWORD layer) {
    COLOR32 *pixels;
    LPTEXTURE texture;

    if (!map->texture_masks[layer] || !map->texture_mask_width || !map->texture_mask_height) {
        return NULL;
    }
    pixels = ri.MemAlloc(map->texture_mask_width * map->texture_mask_height * sizeof(*pixels));
    FOR_LOOP(i, map->texture_mask_width * map->texture_mask_height) {
        BYTE value = (BYTE)(map->texture_masks[layer][i] * 17);
        pixels[i] = (COLOR32){ value, value, value, 255 };
    }
    texture = R_AllocateTexture(map->texture_mask_width, map->texture_mask_height);
    R_LoadTextureMipLevel(texture, 0, pixels, map->texture_mask_width, map->texture_mask_height);
    R_SetTextureWrap(texture, false, false);
    ri.MemFree(pixels);
    return texture;
}

static void r_sc2_load_terrain_textures(sc2Map_t const *map) {
    sc2_num_terrain_layers = MIN(MAX(1, map->num_terrain_textures), SC2_TERRAIN_BLEND_LAYERS);
    FOR_LOOP(i, sc2_num_terrain_layers) {
        if (map->num_terrain_textures > i) {
            sc2_terrain_textures[i] = R_LoadTexture(map->terrain_textures[i].diffuse);
            R_SetTextureWrap(sc2_terrain_textures[i], true, true);
        }
        if (map->num_texture_masks > i) {
            sc2_terrain_masks[i] = r_sc2_build_mask_texture(map, i);
        }
    }
}

static LPMAPLAYER r_sc2_build_ground_layer(sc2Map_t const *map) {
    BOX2 bounds;
    DWORD w, h, num_vertices;
    VERTEX *out;
    VERTEX *vertices;
    LPMAPLAYER map_layer;

    if (!map || !map->width || !map->height)
        return NULL;

    w = map->width;
    h = map->height;
    num_vertices = w * h * SC2_GROUND_VERTICES_PER_CELL;
    vertices = ri.MemAlloc(num_vertices * sizeof(*vertices));
    out = vertices;
    bounds = SC2_MapBounds();

    FOR_LOOP(y, h) {
        FOR_LOOP(x, w) {
            if (r_sc2_skip_ground_cell(map, x, y)) {
                continue;
            }
            FLOAT x0 = bounds.min.x + x * map->cell_size;
            FLOAT y0 = bounds.min.y + y * map->cell_size;
            FLOAT x1 = x0 + map->cell_size;
            FLOAT y1 = y0 + map->cell_size;
            FLOAT u0 = x0 / SC2_TERRAIN_UV_SCALE;
            FLOAT v0 = y0 / SC2_TERRAIN_UV_SCALE;
            FLOAT u1 = x1 / SC2_TERRAIN_UV_SCALE;
            FLOAT v1 = y1 / SC2_TERRAIN_UV_SCALE;
            FLOAT z00 = r_sc2_height_at_grid(map, x, y);
            FLOAT z10 = r_sc2_height_at_grid(map, x + 1, y);
            FLOAT z11 = r_sc2_height_at_grid(map, x + 1, y + 1);
            FLOAT z01 = r_sc2_height_at_grid(map, x, y + 1);
            r_sc2_push_vertex(out++, x0, y0, z00, u0, v0, 255);
            r_sc2_push_vertex(out++, x1, y0, z10, u1, v0, 255);
            r_sc2_push_vertex(out++, x1, y1, z11, u1, v1, 255);
            r_sc2_push_vertex(out++, x0, y0, z00, u0, v0, 255);
            r_sc2_push_vertex(out++, x1, y1, z11, u1, v1, 255);
            r_sc2_push_vertex(out++, x0, y1, z01, u0, v1, 255);
        }
    }

    map_layer = ri.MemAlloc(sizeof(*map_layer));
    memset(map_layer, 0, sizeof(*map_layer));
    map_layer->type = MAPLAYERTYPE_GROUND;
    map_layer->buffer = R_MakeVertexArrayObject(vertices, (DWORD)(out - vertices));
    map_layer->num_vertices = (DWORD)(out - vertices);
    ri.MemFree(vertices);
    return map_layer;
}

static BOOL r_sc2_file_exists(LPCSTR path) {
    void *buffer = NULL;
    int size;

    size = ri.FS_ReadFile(path, &buffer);
    if (size < 0 || !buffer)
        return false;
    ri.FS_FreeFile(buffer);
    return true;
}

static LPCMODEL r_sc2_load_cliff_model(LPCSTR path) {
    sc2CliffModel_t *cliff;

    for (cliff = sc2_cliff_models; cliff; cliff = cliff->next) {
        if (!strcmp(cliff->path, path))
            return cliff->model;
    }
    cliff = ri.MemAlloc(sizeof(*cliff));
    memset(cliff, 0, sizeof(*cliff));
    snprintf(cliff->path, sizeof(cliff->path), "%s", path);
    cliff->model = R_LoadModel(path);
    ADD_TO_LIST(cliff, sc2_cliff_models);
    return cliff->model;
}

static BOOL r_sc2_cliff_model_path(LPSTR path, size_t size, LPCSTR mesh, char const config[5], DWORD variant) {
    snprintf(path, size, "Assets\\Cliffs\\%s\\%s_%s_%02u.m3", mesh, mesh, config, variant & SC2_CLIFF_VARIANT_MASK);
    if (r_sc2_file_exists(path))
        return true;
    snprintf(path, size, "Assets\\Cliffs\\%s\\%s_%s_00.m3", mesh, mesh, config);
    if (r_sc2_file_exists(path))
        return true;
    return false;
}

static BOOL r_sc2_cliff_config(sc2Map_t const *map,
                               DWORD x,
                               DWORD y,
                               char config[5],
                               USHORT *baselevel,
                               USHORT *toplevel,
                               int *rotation) {
    USHORT level[4];
    USHORT base;
    USHORT top;
    char raw[5];
    int best;

    /* SC2 cliff models use BL, BR, TR, TL corner order. */
    SC2_CLIFF_BLOCK_LEVELS(level, map, x, y);
    base = MIN(MIN(level[0], level[1]), MIN(level[2], level[3]));
    top = MAX(MAX(level[0], level[1]), MAX(level[2], level[3]));
    FOR_LOOP(i, 4) {
        USHORT diff = level[i] - base;
        raw[i] = (char)('A' + MIN(diff, 25));
    }
    raw[4] = 0;

    best = 0;
    FOR_LOOP(r, 4) {
        FOR_LOOP(k, 4) {
            char a = raw[(best + k) & 3];
            char b = raw[(r + k) & 3];
            if (b < a) { best = r; break; }
            if (b > a) break;
        }
    }
    FOR_LOOP(i, 4) {
        config[i] = raw[(best + i) & 3];
    }
    config[4] = 0;

    if (baselevel) {
        *baselevel = base;
    }
    if (toplevel) {
        *toplevel = top;
    }
    if (rotation) {
        *rotation = best;
    }
    return config[0] != config[1] || config[1] != config[2] || config[2] != config[3];
}

static VECTOR3 r_sc2_rotate_cliff_vec(VECTOR3 pos, int rotation) {
    switch (rotation & 3) {
        case 1: return (VECTOR3){ -pos.y,  pos.x, pos.z };
        case 2: return (VECTOR3){ -pos.x, -pos.y, pos.z };
        case 3: return (VECTOR3){  pos.y, -pos.x, pos.z };
        default: return pos;
    }
}

static VECTOR3 r_sc2_m3_raw_normal(m3Vertex_t const *vertex) {
    VECTOR3 normal = {
        (FLOAT)vertex->normal[0] * SC2_M3_NORMAL_SCALE - 1.0f,
        (FLOAT)vertex->normal[1] * SC2_M3_NORMAL_SCALE - 1.0f,
        (FLOAT)vertex->normal[2] * SC2_M3_NORMAL_SCALE - 1.0f,
    };
    Vector3_normalize(&normal);
    return normal;
}

static VECTOR3 r_sc2_matrix_transform(LPCMATRIX4 matrix, LPCVECTOR3 v, FLOAT w) {
    return (VECTOR3){
        matrix->v[0] * v->x + matrix->v[4] * v->y + matrix->v[8]  * v->z + matrix->v[12] * w,
        matrix->v[1] * v->x + matrix->v[5] * v->y + matrix->v[9]  * v->z + matrix->v[13] * w,
        matrix->v[2] * v->x + matrix->v[6] * v->y + matrix->v[10] * v->z + matrix->v[14] * w,
    };
}

static void r_sc2_m3_make_bone_matrix(LPCVECTOR3 position,
                                      LPCVECTOR4 rotation,
                                      LPCVECTOR3 scale,
                                      LPCMATRIX4 parent,
                                      LPMATRIX4 matrix) {
    MATRIX4 local;

    Matrix4_identity(&local);
    Matrix4_translate(&local, position);
    Matrix4_rotate4(&local, rotation);
    Matrix4_scale(&local, scale);
    Matrix4_multiply(parent, &local, matrix);
}

static void r_sc2_m3_build_cliff_bones(m3Model_t const *m3, MATRIX4 bones[SC2_M3_MAX_BONES]) {
    MATRIX4 tmp[SC2_M3_MAX_BONES];
    MATRIX4 identity;
    m3SequenceTimeline_t const *timeline;
    DWORD localtime = 0;

    Matrix4_identity(&identity);
    FOR_LOOP(i, SC2_M3_MAX_BONES) {
        Matrix4_identity(&tmp[i]);
        Matrix4_identity(&bones[i]);
    }
    if (!m3 || !m3->bones || !m3->boneLookup || !m3->absoluteInverseBoneRestPositions)
        return;
    timeline = M3_FindAnimationAtTime(m3, 0, &localtime);
    FOR_LOOP(i, MIN(m3->bonesNum, SC2_M3_MAX_BONES)) {
        m3Bone_t const *bone = &m3->bones[i];
        VECTOR3 position = M3_GetVector3AnimValue(m3, timeline, &bone->position, localtime);
        VECTOR4 rotation = M3_GetVector4AnimValue(m3, timeline, &bone->rotation, localtime);
        VECTOR3 scale = M3_GetVector3AnimValue(m3, timeline, &bone->scale, localtime);
        LPCMATRIX4 parent = bone->parent >= 0 && bone->parent < (SHORT)m3->bonesNum ? &tmp[bone->parent] : &identity;

        r_sc2_m3_make_bone_matrix(&position, &rotation, &scale, parent, &tmp[i]);
    }
    FOR_LOOP(i, MIN(m3->boneLookupNum, SC2_M3_MAX_BONES)) {
        m3Uint16_t bone_index = m3->boneLookup[i];
        if (bone_index >= m3->bonesNum || bone_index >= m3->absoluteInverseBoneRestPositionsNum ||
            bone_index >= SC2_M3_MAX_BONES)
            continue;
        Matrix4_multiply(&tmp[bone_index], &m3->absoluteInverseBoneRestPositions[bone_index], &bones[i]);
    }
}

static VECTOR3 r_sc2_m3_skin_vertex(m3Vertex_t const *vertex,
                                    m3Region_t const *region,
                                    MATRIX4 const bones[SC2_M3_MAX_BONES],
                                    FLOAT w) {
    VECTOR3 out = { 0 };
    VECTOR3 raw = w == 0.0f ? r_sc2_m3_raw_normal(vertex) : vertex->pos;

    FOR_LOOP(i, 4) {
        DWORD bone = region->firstBoneLookupIndex + vertex->boneIndex[i];
        FLOAT weight = (FLOAT)vertex->boneWeight[i] / 255.0f;
        VECTOR3 part;

        if (bone >= SC2_M3_MAX_BONES || weight <= 0.0f)
            continue;
        part = r_sc2_matrix_transform(&bones[bone], &raw, w);
        out.x += part.x * weight;
        out.y += part.y * weight;
        out.z += part.z * weight;
    }
    return out;
}

static BOOL r_sc2_cliff_model_bounds(LPCMODEL model, LPBOX3 bounds) {
    m3Model_t const *m3;
    MATRIX4 bones[SC2_M3_MAX_BONES];
    BOOL have_vertex = false;

    if (!model || model->modeltype != ID_43DM || !model->m3 || !model->m3->verticesNum)
        return false;
    m3 = model->m3;
    r_sc2_m3_build_cliff_bones(m3, bones);
    FOR_LOOP(div_i, m3->divisionsNum) {
        m3Divisions_t const *div = &m3->divisions[div_i];
        FOR_LOOP(region_i, div->regionsNum) {
            m3Region_t const *region = &div->regions[region_i];
            for (DWORD index_i = 0; index_i + 2 < region->triangleIndicesCount; index_i += 3) {
                DWORD fi[3], vi[3];
                BOOL valid = true;
                FOR_LOOP(k, 3) {
                    fi[k] = region->firstTriangleIndex + index_i + k;
                    if (fi[k] >= div->facesNum) { valid = false; break; }
                    vi[k] = div->faces[fi[k]] + region->firstVertexIndex;
                    if (vi[k] >= m3->verticesNum) { valid = false; break; }
                }
                if (!valid)
                    continue;
                FOR_LOOP(k, 3) {
                    VECTOR3 pos = r_sc2_m3_skin_vertex(&m3->vertices[vi[k]], region, bones, 1.0f);
                    if (!have_vertex) {
                        bounds->min = pos;
                        bounds->max = pos;
                        have_vertex = true;
                        continue;
                    }
                    bounds->min.x = MIN(bounds->min.x, pos.x);
                    bounds->min.y = MIN(bounds->min.y, pos.y);
                    bounds->min.z = MIN(bounds->min.z, pos.z);
                    bounds->max.x = MAX(bounds->max.x, pos.x);
                    bounds->max.y = MAX(bounds->max.y, pos.y);
                    bounds->max.z = MAX(bounds->max.z, pos.z);
                }
            }
        }
    }
    return have_vertex;
}

static void r_sc2_cliff_terrain_z_span(sc2Map_t const *map,
                                       DWORD grid_x,
                                       DWORD grid_y,
                                       USHORT baselevel,
                                       USHORT toplevel,
                                       LPFLOAT min_z,
                                       LPFLOAT span_z) {
    FLOAT h[4];

    SC2_CLIFF_BLOCK_HEIGHTS(h, map, grid_x, grid_y);
    *min_z = MIN(MIN(h[0], h[1]), MIN(h[2], h[3]));
    *span_z = MAX(MAX(h[0], h[1]), MAX(h[2], h[3])) - *min_z;
    if (*span_z <= SC2_EPSILON)
        *span_z = MAX(SC2_CLIFF_MIN_SPAN, (FLOAT)(toplevel - baselevel)) * map->cell_size;
}

static void r_sc2_bake_cliff_region(rCliffBakeList_t *list,
                                    m3Model_t const *m3,
                                    m3Divisions_t const *div,
                                    m3Region_t const *region,
                                    MATRIX4 const bones[SC2_M3_MAX_BONES],
                                    LPCBOX3 bounds,
                                    LPCVECTOR2 offset,
                                    FLOAT terrain_min_z,
                                    FLOAT z_scale,
                                    int rotation) {
    for (DWORD index_i = 0; index_i + 2 < region->triangleIndicesCount; index_i += 3) {
        DWORD fi[3], vi[3];
        BOOL valid = true;

        FOR_LOOP(k, 3) {
            fi[k] = region->firstTriangleIndex + index_i + k;
            if (fi[k] >= div->facesNum) { valid = false; break; }
            vi[k] = div->faces[fi[k]] + region->firstVertexIndex;
            if (vi[k] >= m3->verticesNum) { valid = false; break; }
        }
        if (!valid)
            continue;
        FOR_LOOP(k, 3) {
            m3Vertex_t const *vertex = &m3->vertices[vi[k]];
            VERTEX *out = R_CliffBakeVertex(list);
            VECTOR3 local;
            VECTOR3 normal;
            VECTOR3 rotated;
            VECTOR3 position;
            VECTOR2 uv;

            local = r_sc2_m3_skin_vertex(vertex, region, bones, 1.0f);
            normal = r_sc2_m3_skin_vertex(vertex, region, bones, 0.0f);
            rotated = r_sc2_rotate_cliff_vec(local, rotation);
            position = (VECTOR3){
                offset->x + rotated.x,
                offset->y + rotated.y,
                terrain_min_z + (local.z - bounds->min.z) * z_scale,
            };
            uv = (VECTOR2){ vertex->uv[0][0] / SC2_M3_UV_SCALE, vertex->uv[0][1] / SC2_M3_UV_SCALE };
            normal = r_sc2_rotate_cliff_vec(normal, rotation);
            Vector3_normalize(&normal);
            r_sc2_push_vertex_normal(out,
                                     position.x,
                                     position.y,
                                     position.z,
                                     uv.x,
                                     uv.y,
                                     255,
                                     normal);
        }
    }
}

static void r_sc2_bake_cliff_model(rCliffBakeList_t *list,
                                   sc2Map_t const *map,
                                   LPCMODEL model,
                                   DWORD grid_x,
                                   DWORD grid_y,
                                   USHORT baselevel,
                                   USHORT toplevel,
                                   int rotation) {
    BOX2 map_bounds = SC2_MapBounds();
    BOX3 bounds;
    VECTOR2 offset;
    FLOAT terrain_min_z;
    FLOAT terrain_span_z;
    FLOAT model_span_z;
    FLOAT z_scale;
    m3Model_t const *m3;
    MATRIX4 bones[SC2_M3_MAX_BONES];

    if (!model || model->modeltype != ID_43DM || !model->m3 || !model->m3->verticesNum)
        return;
    m3 = model->m3;
    r_sc2_m3_build_cliff_bones(m3, bones);
    if (!r_sc2_cliff_model_bounds(model, &bounds)) {
        bounds.min = (VECTOR3){ -map->cell_size, -map->cell_size, 0.0f };
        bounds.max = (VECTOR3){  map->cell_size,  map->cell_size, 0.0f };
    }
    offset = (VECTOR2){
        map_bounds.min.x + SC2_CLIFF_BLOCK_CENTER(grid_x) * map->cell_size,
        map_bounds.min.y + SC2_CLIFF_BLOCK_CENTER(grid_y) * map->cell_size,
    };
    r_sc2_cliff_terrain_z_span(map, grid_x, grid_y, baselevel, toplevel, &terrain_min_z, &terrain_span_z);
    model_span_z = bounds.max.z - bounds.min.z;
    z_scale = model_span_z > SC2_EPSILON ? terrain_span_z / model_span_z : 1.0f;
    FOR_LOOP(div_i, m3->divisionsNum) {
        m3Divisions_t const *div = &m3->divisions[div_i];
        FOR_LOOP(region_i, div->regionsNum) {
            r_sc2_bake_cliff_region(list,
                                    m3,
                                    div,
                                    &div->regions[region_i],
                                    bones,
                                    &bounds,
                                    &offset,
                                    terrain_min_z,
                                    z_scale,
                                    rotation);
        }
    }
}

static LPCTEXTURE r_sc2_make_white_texture(void) {
    LPTEXTURE texture;
    COLOR32 white = COLOR32_WHITE;

    texture = R_AllocateTexture(1, 1);
    R_LoadTextureMipLevel(texture, 0, &white, 1, 1);
    R_SetTextureWrap(texture, true, true);
    return texture;
}

static LPMAPLAYER r_sc2_build_cliff_layer(sc2Map_t const *map) {
    rCliffBakeList_t list = {0};
    LPMAPLAYER map_layer;
    DWORD cliff_width;

    if (!map || !map->num_cliff_cells || !map->cliff_levels)
        return NULL;
    cliff_width = SC2_CLIFF_WIDTH(map);
    FOR_LOOP(i, map->num_cliff_cells) {
        sc2CliffCell_t const *cell = &map->cliff_cells[i];
        sc2CliffSet_t const *set;
        char config[5];
        PATHSTR path;
        DWORD grid_x;
        DWORD grid_y;
        USHORT baselevel;
        USHORT toplevel;
        int rotation;
        LPCMODEL model;

        if (cell->cliff_set >= map->num_cliff_sets)
            continue;
        set = &map->cliff_sets[cell->cliff_set];
        grid_x = (cell->index % cliff_width) * SC2_CLIFF_BLOCK_SPAN;
        grid_y = (cell->index / cliff_width) * SC2_CLIFF_BLOCK_SPAN;
        if (!r_sc2_cliff_config(map, grid_x, grid_y, config, &baselevel, &toplevel, &rotation))
            continue;
        if (!r_sc2_cliff_model_path(path, sizeof(path), set->mesh[0] ? set->mesh : set->name, config, cell->variant))
            continue;
        model = r_sc2_load_cliff_model(path);
        if (!model || model->modeltype != ID_43DM || !model->m3)
            continue;
        r_sc2_bake_cliff_model(&list, map, model, grid_x, grid_y, baselevel, toplevel, rotation);
    }
    if (!list.num_vertices) {
        ri.MemFree(list.vertices);
        return NULL;
    }
    map_layer = ri.MemAlloc(sizeof(*map_layer));
    memset(map_layer, 0, sizeof(*map_layer));
    map_layer->type = MAPLAYERTYPE_CLIFF;
    map_layer->texture = r_sc2_make_white_texture();
    map_layer->buffer = R_MakeVertexArrayObject(list.vertices, list.num_vertices);
    map_layer->num_vertices = list.num_vertices;
    ri.MemFree(list.vertices);
    return map_layer;
}

static void r_sc2_build_terrain(sc2Map_t const *map) {
    BOX2 bounds;
    FLOAT max_z = 1.0f;

    r_sc2_release_terrain();
    if (!map || !map->width || !map->height)
        return;

    r_sc2_init_terrain_shader();
    r_sc2_load_terrain_textures(map);
    sc2_terrain_segment = ri.MemAlloc(sizeof(*sc2_terrain_segment));
    memset(sc2_terrain_segment, 0, sizeof(*sc2_terrain_segment));
    r_sc2_add_layer(&sc2_terrain_segment->layers, r_sc2_build_ground_layer(map));
    r_sc2_add_layer(&sc2_terrain_segment->layers, r_sc2_build_cliff_layer(map));

    bounds = SC2_MapBounds();
    if (map->height_map) {
        FOR_LOOP(i, map->height_map_width * map->height_map_height) {
            max_z = MAX(max_z, map->height_map[i] + 1.0f);
        }
    }
    sc2_terrain_segment->bbox = (BOX3){
        .min = { bounds.min.x, bounds.min.y, -1.0f },
        .max = { bounds.max.x, bounds.max.y, max_z },
    };
}

static void r_sc2_draw_ground_layer(LPCMAPSEGMENT segment) {
    LPCMAPLAYER layer;
    MATRIX4 model_matrix;

    if (!sc2_terrain_shader || !segment)
        return;
    layer = segment->layers;
    while (layer && layer->type != MAPLAYERTYPE_GROUND) {
        layer = layer->next;
    }
    if (!layer)
        return;

    Matrix4_identity(&model_matrix);
    R_Call(glUseProgram, sc2_terrain_shader->progid);
    R_Call(glUniformMatrix4fv, sc2_terrain_shader->uViewProjectionMatrix, 1, GL_FALSE, tr.viewDef.viewProjectionMatrix.v);
    R_Call(glUniformMatrix4fv, sc2_terrain_shader->uModelMatrix, 1, GL_FALSE, model_matrix.v);
    R_Call(glUniformMatrix4fv, sc2_terrain_shader->uTextureMatrix, 1, GL_FALSE, tr.viewDef.textureMatrix.v);
    R_Call(glUniform1i, sc2_u_layer_count, (GLint)sc2_num_terrain_layers);
    FOR_LOOP(i, SC2_TERRAIN_BLEND_LAYERS) {
        R_BindTexture(sc2_terrain_textures[i] ? sc2_terrain_textures[i] : sc2_terrain_textures[0], i);
        R_BindTexture(sc2_terrain_masks[i] ? sc2_terrain_masks[i] : (i ? tr.texture[TEX_BLACK] : tr.texture[TEX_WHITE]),
                      SC2_TERRAIN_BLEND_LAYERS + i);
    }
    R_Call(glDisable, GL_BLEND);
    R_DrawBuffer(layer->buffer, layer->num_vertices);
}

void R_SC2RegisterMap(LPCSTR mapFileName) {
    SC2_MapSetHost(&(sc2MapHost_t){
        .read_file = r_sc2_read_file,
        .free_file = r_sc2_free_file,
        .mem_alloc = ri.MemAlloc,
        .mem_free = ri.MemFree,
    });
    SC2_MapLoad(mapFileName);
    r_sc2_build_terrain(SC2_MapCurrent());
}

void R_SC2DrawWorld(void) {
    MATRIX4 model_matrix;

    if (!sc2_terrain_segment || (tr.viewDef.rdflags & RDF_NOWORLDMODEL))
        return;

    Matrix4_identity(&model_matrix);
    R_Call(glUseProgram, tr.shader[SHADER_UI]->progid);
    R_Call(glUniformMatrix4fv, tr.shader[SHADER_UI]->uViewProjectionMatrix, 1, GL_FALSE, tr.viewDef.viewProjectionMatrix.v);
    R_Call(glUniformMatrix4fv, tr.shader[SHADER_UI]->uModelMatrix, 1, GL_FALSE, model_matrix.v);
    R_Call(glDisable, GL_CULL_FACE);
    R_Call(glEnable, GL_DEPTH_TEST);
    R_Call(glDepthMask, GL_TRUE);
    R_Call(glDepthFunc, GL_LEQUAL);
    R_Call(glColorMask, GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE);
    r_sc2_draw_ground_layer(sc2_terrain_segment);

    R_Call(glUseProgram, tr.shader[SHADER_UI]->progid);
    R_Call(glUniformMatrix4fv, tr.shader[SHADER_UI]->uViewProjectionMatrix, 1, GL_FALSE, tr.viewDef.viewProjectionMatrix.v);
    R_Call(glUniformMatrix4fv, tr.shader[SHADER_UI]->uModelMatrix, 1, GL_FALSE, model_matrix.v);
    R_DrawTerrainSegment(sc2_terrain_segment, (1 << MAPLAYERTYPE_CLIFF));
}

static BOOL r_sc2_clip_trace_to_bounds(LPCLINE3 line, LPCBOX2 bounds, LPFLOAT t0, LPFLOAT t1) {
    FLOAT const bounds_min[2] = { bounds->min.x, bounds->min.y };
    FLOAT const bounds_max[2] = { bounds->max.x, bounds->max.y };
    FLOAT const start[2] = { line->a.x, line->a.y };
    FLOAT const finish[2] = { line->b.x, line->b.y };

    *t0 = 0.0f;
    *t1 = 1.0f;
    FOR_LOOP(axis, 2) {
        FLOAT const dir = finish[axis] - start[axis];
        FLOAT near_t;
        FLOAT far_t;

        if (fabsf(dir) < SC2_TRACE_EPSILON) {
            if (start[axis] < bounds_min[axis] || start[axis] > bounds_max[axis])
                return false;
            continue;
        }

        near_t = (bounds_min[axis] - start[axis]) / dir;
        far_t = (bounds_max[axis] - start[axis]) / dir;
        if (near_t > far_t) {
            FLOAT const swap = near_t;
            near_t = far_t;
            far_t = swap;
        }
        *t0 = MAX(*t0, near_t);
        *t1 = MIN(*t1, far_t);
        if (*t0 > *t1)
            return false;
    }
    return true;
}

static BOOL r_sc2_trace_heightmap_tile(sc2Map_t const *map, DWORD x, DWORD y, LPCLINE3 line, LPVECTOR3 output) {
    BOX2 bounds = SC2_MapBounds();
    FLOAT x0 = bounds.min.x + x * map->cell_size;
    FLOAT y0 = bounds.min.y + y * map->cell_size;
    FLOAT x1 = x0 + map->cell_size;
    FLOAT y1 = y0 + map->cell_size;
    TRIANGLE3 const tri1 = {
        { x0, y0, SC2_MapHeightAtPoint(x0, y0) },
        { x1, y0, SC2_MapHeightAtPoint(x1, y0) },
        { x1, y1, SC2_MapHeightAtPoint(x1, y1) },
    };
    TRIANGLE3 const tri2 = {
        { x1, y1, SC2_MapHeightAtPoint(x1, y1) },
        { x0, y1, SC2_MapHeightAtPoint(x0, y1) },
        { x0, y0, SC2_MapHeightAtPoint(x0, y0) },
    };

    if (Line3_intersect_triangle(line, &tri1, output))
        return true;
    return Line3_intersect_triangle(line, &tri2, output);
}

bool R_SC2TraceLocation(viewDef_t const *viewdef, FLOAT x, FLOAT y, LPVECTOR3 output) {
    sc2Map_t const *map = SC2_MapCurrent();
    BOX2 bounds;
    LINE3 line;
    FLOAT t0, t1;
    FLOAT dir_x, dir_y;
    int tile_x, tile_y;
    int step_x, step_y;
    FLOAT t_max_x, t_max_y;
    FLOAT t_delta_x, t_delta_y;

    if (!viewdef || !output || !map || !map->width || !map->height)
        return false;
    line = R_LineForScreenPoint(viewdef, x, y);
    bounds = SC2_MapBounds();
    if (!r_sc2_clip_trace_to_bounds(&line, &bounds, &t0, &t1))
        return false;

    dir_x = line.b.x - line.a.x;
    dir_y = line.b.y - line.a.y;
    tile_x = (int)floorf((line.a.x + dir_x * t0 - bounds.min.x) / map->cell_size);
    tile_y = (int)floorf((line.a.y + dir_y * t0 - bounds.min.y) / map->cell_size);
    tile_x = MAX(0, MIN((int)map->width - 1, tile_x));
    tile_y = MAX(0, MIN((int)map->height - 1, tile_y));

    if (fabsf(dir_x) < SC2_TRACE_EPSILON) {
        step_x = 0;
        t_max_x = SC2_TRACE_INF;
        t_delta_x = SC2_TRACE_INF;
    } else {
        step_x = dir_x > 0.0f ? 1 : -1;
        t_max_x = (bounds.min.x + (tile_x + (step_x > 0 ? 1.0f : 0.0f)) * map->cell_size - line.a.x) / dir_x;
        t_delta_x = fabsf(map->cell_size / dir_x);
    }

    if (fabsf(dir_y) < SC2_TRACE_EPSILON) {
        step_y = 0;
        t_max_y = SC2_TRACE_INF;
        t_delta_y = SC2_TRACE_INF;
    } else {
        step_y = dir_y > 0.0f ? 1 : -1;
        t_max_y = (bounds.min.y + (tile_y + (step_y > 0 ? 1.0f : 0.0f)) * map->cell_size - line.a.y) / dir_y;
        t_delta_y = fabsf(map->cell_size / dir_y);
    }

    while (tile_x >= 0 && tile_y >= 0 && tile_x < (int)map->width && tile_y < (int)map->height) {
        if (r_sc2_trace_heightmap_tile(map, (DWORD)tile_x, (DWORD)tile_y, &line, output))
            return true;
        if (t_max_x < t_max_y) {
            if (t_max_x > t1)
                break;
            tile_x += step_x;
            t_max_x += t_delta_x;
        } else {
            if (t_max_y > t1)
                break;
            tile_y += step_y;
            t_max_y += t_delta_y;
        }
    }
    return false;
}

FLOAT R_SC2GetHeightAtPoint(FLOAT x, FLOAT y) {
    return SC2_MapHeightAtPoint(x, y);
}

VECTOR2 R_SC2WorldSize(void) {
    sc2Map_t const *map = SC2_MapCurrent();
    return (VECTOR2){ map->width * map->cell_size, map->height * map->cell_size };
}
