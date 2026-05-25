#include "../r_local.h"
#include <strings.h>
#include <stdlib.h>

#define WOW_WDT_TILES 64
#define WOW_MCVT_COUNT (9 * 9 + 8 * 8)
#define WOW_ADT_RADIUS 1
#define WOW_ALPHA_TEXELS (64 * 64)
#define WOW_ALPHA_CHUNK_SIZE 64
#define WOW_ALPHA_ATLAS_CHUNKS ((WOW_ADT_RADIUS * 2 + 1) * 16)
#define WOW_ALPHA_ATLAS_SIZE (WOW_ALPHA_CHUNK_SIZE * WOW_ALPHA_ATLAS_CHUNKS)
#define WOW_IGNORE_TERRAIN_HOLES 1
#define WOW_DEBUG_OBJECT_MARKERS 0
#define WOW_DEBUG_DOODAD_ERROR_MESHES 0
#define WOW_M2_BOUNDING_RADIUS_OFFSET 184

typedef struct wowWdtTile_s {
    BOOL present;
} wowWdtTile_t;

typedef struct wowTextureCache_s {
    PATHSTR path;
    LPTEXTURE texture;
    struct wowTextureCache_s *next;
} wowTextureCache_t;

typedef struct wowM2BoundsCache_s {
    PATHSTR path;
    float radius;
    struct wowM2BoundsCache_s *next;
} wowM2BoundsCache_t;

typedef struct wowDoodadModel_s {
    PATHSTR path;
    LPMODEL model;
    struct wowDoodadModel_s *next;
} wowDoodadModel_t;

typedef struct wowDoodadInstance_s {
    renderEntity_t entity;
    struct wowDoodadInstance_s *next;
} wowDoodadInstance_t;

typedef struct wowWmoBatch_s {
    LPBUFFER buffer;
    LPTEXTURE texture;
    DWORD num_vertices;
    struct wowWmoBatch_s *next;
} wowWmoBatch_t;

typedef struct wowWmoGroup_s {
    wowWmoBatch_t *batches;
} wowWmoGroup_t;

typedef struct wowWmoModel_s {
    PATHSTR path;
    wowWmoGroup_t *groups;
    DWORD num_groups;
    BOOL loaded;
    struct wowWmoModel_s *next;
} wowWmoModel_t;

typedef struct wowWmoInstance_s {
    wowWmoModel_t *model;
    MATRIX4 matrix;
    struct wowWmoInstance_s *next;
} wowWmoInstance_t;

typedef struct wowAdtChunk_s {
    LPBUFFER buffer;
    LPTEXTURE textures[4];
    LPTEXTURE alpha_texture;
    DWORD alpha_index_x;
    DWORD alpha_index_y;
    DWORD num_vertices;
    DWORD layer_count;
    BOX3 bounds;
    struct wowAdtChunk_s *next;
} wowAdtChunk_t;

typedef struct wowMap_s {
    wowWdtTile_t tiles[WOW_WDT_TILES][WOW_WDT_TILES];
    wowAdtChunk_t *chunks;
    wowTextureCache_t *textures;
    wowM2BoundsCache_t *m2_bounds;
    wowDoodadModel_t *doodad_models;
    wowDoodadInstance_t *doodads;
    wowWmoModel_t *wmo_models;
    wowWmoInstance_t *wmos;
    LPTEXTURE alpha_atlas_texture;
    LPBUFFER object_buffer;
    DWORD num_object_vertices;
    DWORD num_adts;
    DWORD num_chunks;
    DWORD num_doodads;
    DWORD num_doodad_instances;
    DWORD num_doodad_models;
    DWORD num_missing_doodad_models;
    DWORD num_filedata_doodads;
    DWORD num_wmos;
    DWORD num_wmo_models;
    DWORD num_wmo_batches;
    DWORD num_missing_wmos;
    DWORD wdt_flags;
    BOOL use_weighted_blend;
    DWORD layer_histogram[5];
    PATHSTR map_dir;
    char map_name[128];
} wowMap_t;

static LPSHADER wow_terrain_shader;
static GLint wow_uTexture0 = -1;
static GLint wow_uTexture1 = -1;
static GLint wow_uTexture2 = -1;
static GLint wow_uTexture3 = -1;
static GLint wow_uAlphaTexture = -1;
static GLint wow_uUseWeightedBlend = -1;
static GLint wow_uAlphaOrigin = -1;
static GLint wow_uAlphaAtlasChunks = -1;

typedef struct {
    DWORD flags;
    DWORD async_id;
} wowWdtMainEntry_t;

typedef struct {
    float x, y, z;
} wowVec3_t;

typedef struct {
    DWORD texture_id;
    DWORD flags;
    DWORD offset_in_mcal;
    DWORD effect_id;
} wowLayer_t;

typedef struct {
    DWORD name_id;
    DWORD unique_id;
    wowVec3_t position;
    wowVec3_t rotation;
    WORD scale;
    WORD flags;
} wowDoodadDef_t;

typedef struct {
    wowVec3_t min;
    wowVec3_t max;
} wowBox_t;

typedef struct {
    DWORD name_id;
    DWORD unique_id;
    wowVec3_t position;
    wowVec3_t rotation;
    wowBox_t extents;
    WORD flags;
    WORD doodad_set;
    WORD name_set;
    WORD unk;
} wowMapObjDef_t;

typedef struct {
    BYTE flags;
    BYTE material_id;
} wowWmoPoly_t;

typedef struct {
    SHORT box_min[3];
    SHORT box_max[3];
    DWORD first_index;
    WORD num_indices;
    WORD first_vertex;
    WORD last_vertex;
    BYTE flags;
    BYTE material_id;
} wowWmoBatchDef_t;

typedef struct {
    float u, v;
} wowVec2_t;

static wowMap_t wow_world;

static BOOL Wow_PathHasExtension(LPCSTR path, LPCSTR extension) {
    size_t path_len;
    size_t ext_len;

    if (!path || !extension) {
        return false;
    }
    path_len = strlen(path);
    ext_len = strlen(extension);
    if (path_len < ext_len) {
        return false;
    }
    return strcasecmp(path + path_len - ext_len, extension) == 0;
}

static void Wow_NormalizeMapPath(LPCSTR mapFileName, LPSTR out, DWORD out_size) {
    if (!mapFileName || !*mapFileName) {
        snprintf(out, out_size, "World/Maps/Azeroth/Azeroth.wdt");
    } else if (Wow_PathHasExtension(mapFileName, ".wdt")) {
        snprintf(out, out_size, "%s", mapFileName);
    } else {
        snprintf(out, out_size, "World/Maps/%s/%s.wdt", mapFileName, mapFileName);
    }
}

static void Wow_SetMapNames(LPCSTR path) {
    char const *slash = strrchr(path, '/');
    char const *backslash = strrchr(path, '\\');
    char const *base = slash > backslash ? slash : backslash;
    size_t dir_len;
    size_t name_len;

    base = base ? base + 1 : path;
    dir_len = (size_t)(base - path);
    if (dir_len > 0 && dir_len < sizeof(wow_world.map_dir)) {
        memcpy(wow_world.map_dir, path, dir_len);
        wow_world.map_dir[dir_len] = '\0';
        if (wow_world.map_dir[dir_len - 1] == '/' || wow_world.map_dir[dir_len - 1] == '\\') {
            wow_world.map_dir[dir_len - 1] = '\0';
        }
    }

    name_len = strlen(base);
    if (name_len > 4 && Wow_PathHasExtension(base, ".wdt")) {
        name_len -= 4;
    }
    name_len = MIN(name_len, sizeof(wow_world.map_name) - 1);
    memcpy(wow_world.map_name, base, name_len);
    wow_world.map_name[name_len] = '\0';
}

static DWORD Wow_Read32(BYTE const *p) {
    DWORD v;
    memcpy(&v, p, sizeof(v));
    return v;
}

static WORD Wow_Read16(BYTE const *p) {
    WORD v;
    memcpy(&v, p, sizeof(v));
    return v;
}

static BOOL Wow_TagEquals(BYTE const *tag, LPCSTR reversed) {
    return memcmp(tag, reversed, 4) == 0;
}

static void Wow_FreeChunks(void) {
    wowAdtChunk_t *chunk = wow_world.chunks;
    while (chunk) {
        wowAdtChunk_t *next = chunk->next;
        R_ReleaseVertexArrayObject(chunk->buffer);
        if (chunk->alpha_texture && chunk->alpha_texture != wow_world.alpha_atlas_texture) {
            R_ReleaseTexture(chunk->alpha_texture);
        }
        ri.MemFree(chunk);
        chunk = next;
    }
    wow_world.chunks = NULL;
    if (wow_world.alpha_atlas_texture) {
        R_ReleaseTexture(wow_world.alpha_atlas_texture);
        wow_world.alpha_atlas_texture = NULL;
    }
    if (wow_world.object_buffer) {
        R_ReleaseVertexArrayObject(wow_world.object_buffer);
        wow_world.object_buffer = NULL;
    }
    wow_world.num_object_vertices = 0;
}

static void Wow_FreeWmoModels(void) {
    wowWmoModel_t *model = wow_world.wmo_models;
    while (model) {
        wowWmoModel_t *next_model = model->next;
        if (model->groups) {
            FOR_LOOP(i, model->num_groups) {
                wowWmoBatch_t *batch = model->groups[i].batches;
                while (batch) {
                    wowWmoBatch_t *next_batch = batch->next;
                    R_ReleaseVertexArrayObject(batch->buffer);
                    ri.MemFree(batch);
                    batch = next_batch;
                }
            }
            ri.MemFree(model->groups);
        }
        ri.MemFree(model);
        model = next_model;
    }
    wow_world.wmo_models = NULL;
}

static void Wow_FreeWmoInstances(void) {
    wowWmoInstance_t *instance = wow_world.wmos;
    while (instance) {
        wowWmoInstance_t *next = instance->next;
        ri.MemFree(instance);
        instance = next;
    }
    wow_world.wmos = NULL;
}

static void Wow_FreeWorld(void) {
    wowM2BoundsCache_t *bounds;
    wowDoodadModel_t *doodad_model;
    wowDoodadInstance_t *doodad;

    Wow_FreeChunks();
    Wow_FreeWmoInstances();
    Wow_FreeWmoModels();
    wowTextureCache_t *texture = wow_world.textures;
    while (texture) {
        wowTextureCache_t *next = texture->next;
        R_ReleaseTexture(texture->texture);
        ri.MemFree(texture);
        texture = next;
    }
    bounds = wow_world.m2_bounds;
    while (bounds) {
        wowM2BoundsCache_t *next = bounds->next;
        ri.MemFree(bounds);
        bounds = next;
    }
    doodad_model = wow_world.doodad_models;
    while (doodad_model) {
        wowDoodadModel_t *next = doodad_model->next;
        SAFE_DELETE(doodad_model->model, R_ReleaseModel);
        ri.MemFree(doodad_model);
        doodad_model = next;
    }
    doodad = wow_world.doodads;
    while (doodad) {
        wowDoodadInstance_t *next = doodad->next;
        ri.MemFree(doodad);
        doodad = next;
    }
    memset(&wow_world, 0, sizeof(wow_world));
}

static LPTEXTURE Wow_LoadTexture(LPCSTR path) {
    wowTextureCache_t *entry;

    for (entry = wow_world.textures; entry; entry = entry->next) {
        if (!strcasecmp(entry->path, path)) {
            return entry->texture;
        }
    }

    entry = ri.MemAlloc(sizeof(*entry));
    memset(entry, 0, sizeof(*entry));
    snprintf(entry->path, sizeof(entry->path), "%s", path);
    entry->texture = R_LoadTexture(path);
    if (entry->texture) {
        R_SetTextureWrap(entry->texture, true, true);
        if (entry->texture != tr.texture[TEX_WHITE]) {
            /* Keep cache hit path silent in normal runs. */
        }
    } else {
        entry->texture = tr.texture[TEX_WHITE];
    }
    entry->next = wow_world.textures;
    wow_world.textures = entry;
    return entry->texture;
}

static BOOL Wow_ReadM2RadiusFromPath(LPCSTR path, float *radius) {
    LPBYTE data = NULL;
    int size;

    size = ri.FS_ReadFile(path, (void **)&data);
    if (size < WOW_M2_BOUNDING_RADIUS_OFFSET + (int)sizeof(float) || !data) {
        if (data) {
            ri.FS_FreeFile(data);
        }
        return false;
    }

    if (*(DWORD *)data != MAKEFOURCC('M', 'D', '2', '0')) {
        ri.FS_FreeFile(data);
        return false;
    }

    memcpy(radius, data + WOW_M2_BOUNDING_RADIUS_OFFSET, sizeof(*radius));
    ri.FS_FreeFile(data);
    return true;
}

static BOOL Wow_CopyModelPathFallback(LPCSTR path, LPSTR out, DWORD out_size) {
    size_t len;

    if (!path || !out || out_size == 0 || !Wow_PathHasExtension(path, ".mdx")) {
        return false;
    }

    len = strlen(path);
    if (len + 1 > out_size) {
        return false;
    }

    snprintf(out, out_size, "%s", path);
    out[len - 3] = 'm';
    out[len - 2] = '2';
    out[len - 1] = '\0';
    return true;
}

static float Wow_LoadM2BoundsRadius(LPCSTR path) {
    wowM2BoundsCache_t *entry;
    PATHSTR fallback_path;
    float radius = 0.0f;

    if (!path || !*path) {
        return 0.0f;
    }

    for (entry = wow_world.m2_bounds; entry; entry = entry->next) {
        if (!strcasecmp(entry->path, path)) {
            return entry->radius;
        }
    }

    if (!Wow_ReadM2RadiusFromPath(path, &radius) &&
        Wow_CopyModelPathFallback(path, fallback_path, sizeof(fallback_path))) {
        (void)Wow_ReadM2RadiusFromPath(fallback_path, &radius);
    }

    entry = ri.MemAlloc(sizeof(*entry));
    memset(entry, 0, sizeof(*entry));
    snprintf(entry->path, sizeof(entry->path), "%s", path);
    entry->radius = radius;
    entry->next = wow_world.m2_bounds;
    wow_world.m2_bounds = entry;
    if (radius > 0.0f) {
        wow_world.num_doodad_models++;
    }
    return radius;
}

static LPTEXTURE Wow_CreateAlphaTexture(BYTE const alpha[4][WOW_ALPHA_TEXELS]) {
    BYTE pixels[WOW_ALPHA_TEXELS * 4];
    LPTEXTURE texture = R_AllocateTexture(64, 64);

    FOR_LOOP(i, WOW_ALPHA_TEXELS) {
        pixels[i * 4 + 0] = alpha[0][i];
        pixels[i * 4 + 1] = alpha[1][i];
        pixels[i * 4 + 2] = alpha[2][i];
        pixels[i * 4 + 3] = alpha[3][i];
    }

    R_Call(glBindTexture, GL_TEXTURE_2D, texture->texid);
    R_Call(glTexImage2D, GL_TEXTURE_2D, 0, GL_RGBA, 64, 64, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    R_Call(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    R_Call(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    R_Call(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
    R_Call(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
    R_Call(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    R_Call(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    return texture;
}

static void Wow_EnsureAlphaAtlasTexture(void) {
    if (wow_world.alpha_atlas_texture) {
        return;
    }

    wow_world.alpha_atlas_texture = R_AllocateTexture(WOW_ALPHA_ATLAS_SIZE, WOW_ALPHA_ATLAS_SIZE);
    R_Call(glBindTexture, GL_TEXTURE_2D, wow_world.alpha_atlas_texture->texid);
    R_Call(glTexImage2D,
           GL_TEXTURE_2D,
           0,
           GL_RGBA,
           WOW_ALPHA_ATLAS_SIZE,
           WOW_ALPHA_ATLAS_SIZE,
           0,
           GL_RGBA,
           GL_UNSIGNED_BYTE,
           NULL);
    R_Call(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    R_Call(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    R_Call(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
    R_Call(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
    R_Call(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    R_Call(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

static void Wow_UploadAlphaAtlasChunk(DWORD index_x, DWORD index_y, BYTE const alpha[4][WOW_ALPHA_TEXELS]) {
    BYTE pixels[WOW_ALPHA_TEXELS * 4];

    if (index_x >= WOW_ALPHA_ATLAS_CHUNKS || index_y >= WOW_ALPHA_ATLAS_CHUNKS) {
        return;
    }

    Wow_EnsureAlphaAtlasTexture();
    FOR_LOOP(i, WOW_ALPHA_TEXELS) {
        pixels[i * 4 + 0] = alpha[0][i];
        pixels[i * 4 + 1] = alpha[1][i];
        pixels[i * 4 + 2] = alpha[2][i];
        pixels[i * 4 + 3] = alpha[3][i];
    }

    R_Call(glBindTexture, GL_TEXTURE_2D, wow_world.alpha_atlas_texture->texid);
    R_Call(glTexSubImage2D,
           GL_TEXTURE_2D,
           0,
           index_x * WOW_ALPHA_CHUNK_SIZE,
           index_y * WOW_ALPHA_CHUNK_SIZE,
           WOW_ALPHA_CHUNK_SIZE,
           WOW_ALPHA_CHUNK_SIZE,
           GL_RGBA,
           GL_UNSIGNED_BYTE,
           pixels);
}

static void Wow_InitTerrainShader(void) {
    static LPCSTR vs_wow_terrain =
    "#version 140\n"
    "in vec3 i_position;\n"
    "in vec2 i_texcoord;\n"
    "in vec4 i_color;\n"
    "out vec2 v_texcoord;\n"
    "out vec4 v_color;\n"
    "uniform mat4 uViewProjectionMatrix;\n"
    "uniform mat4 uModelMatrix;\n"
    "void main() {\n"
    "    v_texcoord = i_texcoord;\n"
    "    v_color = i_color;\n"
    "    gl_Position = uViewProjectionMatrix * uModelMatrix * vec4(i_position, 1.0);\n"
    "}\n";
    static LPCSTR fs_wow_terrain =
    "#version 140\n"
    "in vec2 v_texcoord;\n"
    "in vec4 v_color;\n"
    "out vec4 o_color;\n"
    "uniform sampler2D uTexture0;\n"
    "uniform sampler2D uTexture1;\n"
    "uniform sampler2D uTexture2;\n"
    "uniform sampler2D uTexture3;\n"
    "uniform sampler2D uAlphaTexture;\n"
    "uniform int uUseWeightedBlend;\n"
    "uniform vec2 uAlphaOrigin;\n"
    "uniform float uAlphaAtlasChunks;\n"
    "vec2 adtAlphaCoord(vec2 chunkCoord) {\n"
    "    const float alphaTexelsPerChunk = 64.0;\n"
    "    float alphaAtlasSize = alphaTexelsPerChunk * uAlphaAtlasChunks;\n"
    "    chunkCoord = clamp(chunkCoord, vec2(0.0), vec2(1.0));\n"
    "    vec2 atlasTexel = uAlphaOrigin * alphaTexelsPerChunk + chunkCoord * (alphaTexelsPerChunk - 1.0) + vec2(0.5);\n"
    "    return atlasTexel / alphaAtlasSize;\n"
    "}\n"
    "void main() {\n"
    "    vec2 alphaCoord = adtAlphaCoord(v_texcoord * 0.125);\n"
    "    vec3 alphaBlend = texture(uAlphaTexture, alphaCoord).gba;\n"
    "    vec4 tex1 = texture(uTexture0, v_texcoord);\n"
    "    vec4 tex2 = texture(uTexture1, v_texcoord);\n"
    "    vec4 tex3 = texture(uTexture2, v_texcoord);\n"
    "    vec4 tex4 = texture(uTexture3, v_texcoord);\n"
    "    vec4 color;\n"
    "    if (uUseWeightedBlend != 0) {\n"
    "        float baseWeight = 1.0 - clamp(dot(alphaBlend, vec3(1.0)), 0.0, 1.0);\n"
    "        vec4 weights = vec4(baseWeight, alphaBlend);\n"
    "        color = tex1 * weights.r + tex2 * weights.g + tex3 * weights.b + tex4 * weights.a;\n"
    "    } else {\n"
    "        color = mix(mix(mix(tex1, tex2, alphaBlend.r), tex3, alphaBlend.g), tex4, alphaBlend.b);\n"
    "    }\n"
    "    color.rgb *= 2.0 * v_color.rgb;\n"
    "    color.a = 1.0;\n"
    "    o_color = color;\n"
    "}\n";

    if (wow_terrain_shader) {
        return;
    }

    wow_terrain_shader = R_InitShader(vs_wow_terrain, fs_wow_terrain);
    if (!wow_terrain_shader) {
        return;
    }

    wow_uTexture0 = glGetUniformLocation(wow_terrain_shader->progid, "uTexture0");
    wow_uTexture1 = glGetUniformLocation(wow_terrain_shader->progid, "uTexture1");
    wow_uTexture2 = glGetUniformLocation(wow_terrain_shader->progid, "uTexture2");
    wow_uTexture3 = glGetUniformLocation(wow_terrain_shader->progid, "uTexture3");
    wow_uAlphaTexture = glGetUniformLocation(wow_terrain_shader->progid, "uAlphaTexture");
    wow_uUseWeightedBlend = glGetUniformLocation(wow_terrain_shader->progid, "uUseWeightedBlend");
    wow_uAlphaOrigin = glGetUniformLocation(wow_terrain_shader->progid, "uAlphaOrigin");
    wow_uAlphaAtlasChunks = glGetUniformLocation(wow_terrain_shader->progid, "uAlphaAtlasChunks");
    R_Call(glUseProgram, wow_terrain_shader->progid);
    R_Call(glUniform1i, wow_uTexture0, 0);
    R_Call(glUniform1i, wow_uTexture1, 1);
    R_Call(glUniform1i, wow_uTexture2, 2);
    R_Call(glUniform1i, wow_uTexture3, 3);
    R_Call(glUniform1i, wow_uAlphaTexture, 4);
    R_Call(glUniform1f, wow_uAlphaAtlasChunks, (GLfloat)WOW_ALPHA_ATLAS_CHUNKS);
}

static COLOR32 Wow_Color(BYTE r, BYTE g, BYTE b, BYTE a) {
    return (COLOR32){ b, g, r, a };
}

static VERTEX Wow_Vertex(float x, float y, float z, float u, float v, COLOR32 color) {
    VERTEX vertex;
    memset(&vertex, 0, sizeof(vertex));
    vertex.position = (VECTOR3){ x, y, z };
    vertex.texcoord = (VECTOR2){ u, v };
    vertex.normal = (VECTOR3){ 0.0f, 0.0f, 1.0f };
    vertex.color = color;
    return vertex;
}

static void Wow_AddBoundsPoint(LPBOX3 bounds, LPCVECTOR3 p) {
    bounds->min.x = MIN(bounds->min.x, p->x);
    bounds->min.y = MIN(bounds->min.y, p->y);
    bounds->min.z = MIN(bounds->min.z, p->z);
    bounds->max.x = MAX(bounds->max.x, p->x);
    bounds->max.y = MAX(bounds->max.y, p->y);
    bounds->max.z = MAX(bounds->max.z, p->z);
}

static VECTOR3 Wow_ScalePoint(float x, float y, float z) {
    return (VECTOR3){
        x + WOW_TILE_SIZE * 0.5f,
        y + WOW_TILE_SIZE * 0.5f,
        z,
    };
}

static VECTOR2 Wow_McvtCoords(int index) {
    int row = index / 17;
    int col = index % 17;
    VECTOR2 coords;

    if (col < 9) {
        coords.x = col * WOW_UNIT_SIZE;
        coords.y = row * WOW_UNIT_SIZE;
    } else {
        coords.x = (col - 8.5f) * WOW_UNIT_SIZE;
        coords.y = (row + 0.5f) * WOW_UNIT_SIZE;
    }
    return coords;
}

static VECTOR3 Wow_McvtPoint(wowVec3_t pos, float const *heights, int index) {
    VECTOR2 coords = Wow_McvtCoords(index);

    return Wow_ScalePoint(
        pos.x - coords.y,
        pos.y - coords.x,
        pos.z + heights[index]);
}

static VECTOR3 Wow_NormalAtHeightIndex(BYTE const *normals, int height_index) {
    if (!normals) {
        return (VECTOR3){ 0.0f, 0.0f, 1.0f };
    }
    return (VECTOR3){
        ((signed char const *)normals)[height_index * 3 + 0] / 127.0f,
        ((signed char const *)normals)[height_index * 3 + 2] / 127.0f,
        ((signed char const *)normals)[height_index * 3 + 1] / 127.0f,
    };
}

static void Wow_PushTerrainVertex(VERTEX *vertices,
                                  LPDWORD index,
                                  wowVec3_t pos,
                                  float const *heights,
                                  BYTE const *normals,
                                  int height_index,
                                  COLOR32 color) {
    VECTOR3 p = Wow_McvtPoint(pos, heights, height_index);
    VECTOR2 coords = Wow_McvtCoords(height_index);
    float u = coords.x / WOW_UNIT_SIZE;
    float v = coords.y / WOW_UNIT_SIZE;
    VERTEX vertex = Wow_Vertex(p.x, p.y, p.z, u, v, color);
    vertex.normal = Wow_NormalAtHeightIndex(normals, height_index);
    vertices[(*index)++] = vertex;
}

static BOOL Wow_IsHole(WORD holes, int x, int y) {
    static int holetab_h[4] = { 0x1111, 0x2222, 0x4444, 0x8888 };
    static int holetab_v[4] = { 0x000f, 0x00f0, 0x0f00, 0xf000 };
    x >>= 1;
    y >>= 1;
    return (holes & holetab_h[x] & holetab_v[y]) != 0;
}

static void Wow_AddTerrainCell(VERTEX *vertices,
                               LPDWORD index,
                               wowVec3_t pos,
                               float const *heights,
                               BYTE const *normals,
                               int x,
                               int y,
                               COLOR32 color) {
    static BYTE const tri[] = { 9, 0, 17, 9, 1, 0, 9, 18, 1, 9, 17, 18 };
    int base = y * 17 + x;
    FOR_LOOP(i, sizeof(tri) / sizeof(tri[0])) {
        Wow_PushTerrainVertex(vertices, index, pos, heights, normals, base + tri[i], color);
    }
}

static DWORD Wow_PredictedLayer(WORD const pred_tex[8], DWORD layer_count, int x, int y) {
    DWORD layer;

    if (!pred_tex || layer_count <= 1) {
        return 0;
    }
    layer = (pred_tex[y] >> (x * 2)) & 3;
    if (layer >= layer_count) {
        layer = 0;
    }
    return layer;
}

static DWORD Wow_AlphaSlotForTexture(DWORD unique_texture_ids[4], DWORD *unique_count, DWORD texture_id) {
    FOR_LOOP(i, *unique_count) {
        if (unique_texture_ids[i] == texture_id) {
            return i;
        }
    }
    if (*unique_count < 4) {
        unique_texture_ids[*unique_count] = texture_id;
        return (*unique_count)++;
    }
    return 0;
}

static DWORD Wow_BuildUniqueTextureSlots(wowLayer_t const *layers,
                                         DWORD layer_count,
                                         DWORD slot_texture_ids[4]) {
    DWORD unique_texture_ids[4] = { 0, 0, 0, 0 };
    DWORD unique_count = 0;

    memset(slot_texture_ids, 0, sizeof(DWORD) * 4);
    if (!layers || layer_count == 0) {
        return 0;
    }

    FOR_LOOP(layer_index, MIN(layer_count, 4)) {
        DWORD texture_id = layers[layer_index].texture_id;
        DWORD slot = Wow_AlphaSlotForTexture(unique_texture_ids, &unique_count, texture_id);
        if (slot < 4) {
            slot_texture_ids[slot] = texture_id;
        }
    }

    return unique_count;
}

static void Wow_DecodeAlphaLayer(BYTE const *src,
                                 BYTE const *src_end,
                                 DWORD flags,
                                 DWORD mcnk_flags,
                                 BOOL big_alpha,
                                 BYTE out[WOW_ALPHA_TEXELS]) {
    DWORD read_count = 0;

    if (!src || src >= src_end) {
        return;
    }

    if (flags & 0x200) {
        while (read_count < WOW_ALPHA_TEXELS && src < src_end) {
            BYTE code = *src++;
            DWORD count = code & 0x7f;
            if (code & 0x80) {
                BYTE value;
                if (src >= src_end) {
                    break;
                }
                value = *src++;
                while (count-- && read_count < WOW_ALPHA_TEXELS) {
                    out[read_count++] = value;
                }
            } else {
                while (count-- && read_count < WOW_ALPHA_TEXELS && src < src_end) {
                    out[read_count++] = *src++;
                }
            }
        }
    } else if (big_alpha) {
        while (read_count < WOW_ALPHA_TEXELS && src < src_end) {
            out[read_count++] = *src++;
        }
    } else {
        BOOL do_not_fix_alpha_map = (mcnk_flags & 0x8000) != 0;
        size_t nibble_count = (size_t)(src_end - src) * 2;

        if (do_not_fix_alpha_map) {
            while (read_count < WOW_ALPHA_TEXELS && src < src_end) {
                BYTE value = *src++;
                out[read_count++] = (BYTE)((value & 0x0f) * 17);
                if (read_count < WOW_ALPHA_TEXELS) {
                    out[read_count++] = (BYTE)(((value >> 4) & 0x0f) * 17);
                }
            }
        } else {
            FOR_LOOP(y, 63) {
                FOR_LOOP(x, 63) {
                    size_t nibble_index = (size_t)y * 64 + x;
                    BYTE packed;
                    BYTE nibble;

                    if (nibble_index >= nibble_count) {
                        continue;
                    }
                    packed = src[nibble_index >> 1];
                    nibble = (nibble_index & 1) ? ((packed >> 4) & 0x0f) : (packed & 0x0f);
                    out[y * 64 + x] = (BYTE)(nibble * 17);
                }
                out[y * 64 + 63] = out[y * 64 + 62];
            }
            FOR_LOOP(x, 64) {
                out[63 * 64 + x] = out[62 * 64 + x];
            }
        }
    }
}

static void Wow_DecodeAlphaMaps(BYTE const *mcal,
                                DWORD mcal_size,
                                wowLayer_t const *layers,
                                DWORD layer_count,
                                DWORD mcnk_flags,
                                BYTE alpha[4][WOW_ALPHA_TEXELS]) {
    BOOL big_alpha = (wow_world.wdt_flags & (0x4 | 0x80)) != 0;
    DWORD unique_texture_ids[4] = { 0, 0, 0, 0 };
    DWORD unique_count = 0;
    DWORD uncompressed_index = 0;
    BOOL has_uncompressed = false;
    memset(alpha, 0, sizeof(BYTE) * 4 * WOW_ALPHA_TEXELS);

    if (!mcal || !layers || layer_count <= 1) {
        return;
    }

    FOR_LOOP(layer_index, MIN(layer_count, 4)) {
        DWORD slot = Wow_AlphaSlotForTexture(unique_texture_ids, &unique_count, layers[layer_index].texture_id);
        BYTE const *src;
        DWORD offset;

        if (!(layers[layer_index].flags & 0x100)) {
            uncompressed_index = slot;
            has_uncompressed = true;
            continue;
        }

        offset = layers[layer_index].offset_in_mcal;
        if (offset >= mcal_size) {
            continue;
        }

        src = mcal + offset;
        Wow_DecodeAlphaLayer(src, mcal + mcal_size, layers[layer_index].flags, mcnk_flags, big_alpha, alpha[slot]);
    }

    if (has_uncompressed) {
        FOR_LOOP(i, WOW_ALPHA_TEXELS) {
            BYTE value = 255;
            FOR_LOOP(layer_index, 4) {
                value = (BYTE)(value - alpha[layer_index][i]);
            }
            alpha[uncompressed_index][i] = value;
        }
    }

}

static void Wow_AddAdtChunk(wowVec3_t pos,
                            DWORD alpha_index_x,
                            DWORD alpha_index_y,
                            WORD holes,
                            BYTE const alpha[4][WOW_ALPHA_TEXELS],
                            wowLayer_t const *layers,
                            DWORD layer_count,
                            char **textures,
                            DWORD num_textures,
                            float const *heights,
                            BYTE const *normals) {
    enum { MAX_VERTICES = 8 * 8 * 12 };
    COLOR32 color = Wow_Color(127, 127, 127, 127);
    DWORD slot_texture_ids[4] = { 0, 0, 0, 0 };
    DWORD unique_layer_count = Wow_BuildUniqueTextureSlots(layers, layer_count, slot_texture_ids);
    DWORD effective_layers = MAX(1, MIN(unique_layer_count ? unique_layer_count : layer_count, 4));
    VERTEX *vertices;
    DWORD num_vertices = 0;
    wowAdtChunk_t *chunk;

    if (!heights) {
        return;
    }

    vertices = ri.MemAlloc(sizeof(VERTEX) * MAX_VERTICES);
    if (!vertices) {
        return;
    }

    FOR_LOOP(y, 8) {
        FOR_LOOP(x, 8) {
            if (WOW_IGNORE_TERRAIN_HOLES || !Wow_IsHole(holes, x, y)) {
                Wow_AddTerrainCell(vertices, &num_vertices, pos, heights, normals, x, y, color);
            }
        }
    }

    if (!num_vertices) {
        ri.MemFree(vertices);
        return;
    }

    chunk = ri.MemAlloc(sizeof(*chunk));
    memset(chunk, 0, sizeof(*chunk));
    chunk->bounds = (BOX3){
        .min = { FLT_MAX, FLT_MAX, FLT_MAX },
        .max = { -FLT_MAX, -FLT_MAX, -FLT_MAX },
    };
    FOR_LOOP(i, num_vertices) {
        Wow_AddBoundsPoint(&chunk->bounds, &vertices[i].position);
    }
    chunk->buffer = R_MakeVertexArrayObject(vertices, num_vertices);
    chunk->num_vertices = num_vertices;
    chunk->layer_count = effective_layers;
    chunk->alpha_texture = wow_world.alpha_atlas_texture ? wow_world.alpha_atlas_texture : Wow_CreateAlphaTexture(alpha);
    chunk->alpha_index_x = alpha_index_x;
    chunk->alpha_index_y = alpha_index_y;
    FOR_LOOP(layer_index, 4) {
        DWORD texture_id = 0;
        if (layer_index < unique_layer_count) {
            texture_id = slot_texture_ids[layer_index];
        } else if (unique_layer_count > 0) {
            texture_id = slot_texture_ids[0];
        }
        if (texture_id < num_textures && textures[texture_id]) {
            chunk->textures[layer_index] = Wow_LoadTexture(textures[texture_id]);
        } else {
            chunk->textures[layer_index] = layer_index > 0 ? chunk->textures[0] : tr.texture[TEX_WHITE];
        }
    }
    ADD_TO_LIST(chunk, wow_world.chunks);
    wow_world.num_chunks++;
    wow_world.layer_histogram[MIN(effective_layers, 4)]++;
    ri.MemFree(vertices);
}

static void Wow_FreeStringList(char **strings, DWORD count) {
    if (!strings) {
        return;
    }
    FOR_LOOP(i, count) {
        SAFE_DELETE(strings[i], ri.MemFree);
    }
    ri.MemFree(strings);
}

static char **Wow_ParseStringBlock(BYTE const *data, DWORD size, LPDWORD out_count) {
    DWORD count = 0;
    DWORD offset = 0;
    char **strings;

    while (offset < size) {
        size_t len = strlen((LPCSTR)(data + offset));
        count++;
        offset += (DWORD)len + 1;
    }

    strings = ri.MemAlloc(sizeof(char *) * MAX(count, 1));
    memset(strings, 0, sizeof(char *) * MAX(count, 1));
    offset = 0;
    count = 0;
    while (offset < size) {
        LPCSTR value = (LPCSTR)(data + offset);
        size_t len = strlen(value);
        strings[count] = ri.MemAlloc(len + 1);
        memcpy(strings[count], value, len + 1);
        count++;
        offset += (DWORD)len + 1;
    }

    *out_count = count;
    return strings;
}

static LPCSTR Wow_StringRefFromOffsets(BYTE const *blob, DWORD blob_size, DWORD const *offsets, DWORD offset_count, DWORD id) {
    DWORD offset;

    if (!blob || !offsets || id >= offset_count) {
        return NULL;
    }
    offset = offsets[id];
    if (offset >= blob_size || memchr(blob + offset, '\0', blob_size - offset) == NULL) {
        return NULL;
    }
    return (LPCSTR)(blob + offset);
}

static VECTOR3 Wow_ObjectPoint(wowVec3_t p) {
    return Wow_ScalePoint(
        32.0f * WOW_TILE_SIZE - p.z,
        32.0f * WOW_TILE_SIZE - p.x,
        p.y);
}

static void Wow_InstanceMatrix(wowMapObjDef_t const *def, LPMATRIX4 matrix) {
    MATRIX4 basis;
    MATRIX4 tmp;
    VECTOR3 origin;

    Matrix4_identity(matrix);
    origin = Wow_ObjectPoint(def->position);
    Matrix4_translate(matrix, &origin);

    Matrix4_identity(&basis);
    basis.v[0] = 0.0f;
    basis.v[1] = -1.0f;
    basis.v[2] = 0.0f;
    basis.v[4] = 0.0f;
    basis.v[5] = 0.0f;
    basis.v[6] = 1.0f;
    basis.v[8] = -1.0f;
    basis.v[9] = 0.0f;
    basis.v[10] = 0.0f;
    Matrix4_multiply(matrix, &basis, &tmp);
    *matrix = tmp;

    Matrix4_scale(matrix, &(VECTOR3){ -1.0f, 1.0f, -1.0f });
    Matrix4_rotate(matrix, &(VECTOR3){ 0.0f, def->rotation.y - 270.0f, 0.0f }, ROTATE_XYZ);
    Matrix4_rotate(matrix, &(VECTOR3){ 0.0f, 0.0f, -def->rotation.x }, ROTATE_XYZ);
    Matrix4_rotate(matrix, &(VECTOR3){ def->rotation.z - 90.0f, 0.0f, 0.0f }, ROTATE_XYZ);
    if (def->unk) {
        float scale = def->unk / 1024.0f;
        Matrix4_scale(matrix, &(VECTOR3){ scale, scale, scale });
    }
}

static void Wow_GroupPath(LPCSTR root_path, DWORD group_index, LPSTR out, DWORD out_size) {
    size_t len = strlen(root_path);
    if (len > 4 && Wow_PathHasExtension(root_path, ".wmo")) {
        snprintf(out, out_size, "%.*s_%03u.wmo", (int)(len - 4), root_path, (unsigned)group_index);
    } else {
        snprintf(out, out_size, "%s_%03u.wmo", root_path, (unsigned)group_index);
    }
}

static LPCSTR Wow_StringAt(LPCSTR blob, DWORD blob_size, DWORD offset) {
    if (!blob || offset >= blob_size) {
        return NULL;
    }
    if (!memchr(blob + offset, '\0', blob_size - offset)) {
        return NULL;
    }
    return blob + offset;
}

static BOOL Wow_LoadWmoGroup(wowWmoModel_t *model,
                             DWORD group_index,
                             LPTEXTURE const *materials,
                             DWORD material_count) {
    PATHSTR group_path;
    LPBYTE data = NULL;
    int size;
    DWORD offset = 0;
    BYTE const *mopy = NULL;
    DWORD mopy_count = 0;
    WORD const *indices = NULL;
    DWORD index_count = 0;
    wowVec3_t const *vertices = NULL;
    DWORD vertex_count = 0;
    wowVec2_t const *uvs = NULL;
    DWORD uv_count = 0;
    wowWmoBatchDef_t const *batches = NULL;
    DWORD batch_count = 0;

    Wow_GroupPath(model->path, group_index, group_path, sizeof(group_path));
    size = ri.FS_ReadFile(group_path, (void **)&data);
    if (size <= 0 || !data) {
        fprintf(stderr, "WoW WMO: missing group %s\n", group_path);
        return false;
    }

    while (offset + 8 <= (DWORD)size) {
        BYTE const *tag = data + offset;
        DWORD chunk_size = Wow_Read32(data + offset + 4);
        BYTE const *chunk = data + offset + 8;
        offset += 8;
        if (offset + chunk_size > (DWORD)size) {
            break;
        }

        if (Wow_TagEquals(tag, "PGOM")) {
            DWORD sub = 0x44;
            if (chunk_size < sub) {
                break;
            }
            while (sub + 8 <= chunk_size) {
                BYTE const *subtag = chunk + sub;
                DWORD sub_size = Wow_Read32(chunk + sub + 4);
                BYTE const *subchunk = chunk + sub + 8;
                sub += 8;
                if (sub + sub_size > chunk_size) {
                    break;
                }
                if (Wow_TagEquals(subtag, "YPOM")) {
                    mopy = subchunk;
                    mopy_count = sub_size / sizeof(wowWmoPoly_t);
                } else if (Wow_TagEquals(subtag, "IVOM")) {
                    indices = (WORD const *)subchunk;
                    index_count = sub_size / sizeof(WORD);
                } else if (Wow_TagEquals(subtag, "TVOM")) {
                    vertices = (wowVec3_t const *)subchunk;
                    vertex_count = sub_size / sizeof(wowVec3_t);
                } else if (Wow_TagEquals(subtag, "VTOM")) {
                    uvs = (wowVec2_t const *)subchunk;
                    uv_count = sub_size / sizeof(wowVec2_t);
                } else if (Wow_TagEquals(subtag, "ABOM")) {
                    batches = (wowWmoBatchDef_t const *)subchunk;
                    batch_count = sub_size / sizeof(wowWmoBatchDef_t);
                }
                sub += sub_size;
            }
        }
        offset += chunk_size;
    }

    if (!vertices || !indices || !vertex_count || !index_count) {
        fprintf(stderr, "WoW WMO: group %s has no drawable geometry\n", group_path);
        ri.FS_FreeFile(data);
        return false;
    }

    if (batch_count) {
        FOR_LOOP(batch_index, batch_count) {
            wowWmoBatchDef_t const *batch = batches + batch_index;
            DWORD first_index = batch->first_index;
            DWORD num_indices = batch->num_indices;
            DWORD material_id = batch->material_id;
            VERTEX *out_vertices;
            DWORD out_count = 0;
            wowWmoBatch_t *out_batch;

            if (first_index >= index_count || first_index + num_indices > index_count || !num_indices) {
                continue;
            }
            out_vertices = ri.MemAlloc(sizeof(VERTEX) * num_indices);
            FOR_LOOP(i, num_indices) {
                WORD vertex_index = indices[first_index + i];
                wowVec3_t p;
                wowVec2_t uv = { 0.0f, 0.0f };
                if (vertex_index >= vertex_count) {
                    continue;
                }
                p = vertices[vertex_index];
                if (uvs && vertex_index < uv_count) {
                    uv = uvs[vertex_index];
                }
                out_vertices[out_count++] = Wow_Vertex(p.x, p.y, p.z, uv.u, uv.v, COLOR32_WHITE);
            }
            if (!out_count) {
                ri.MemFree(out_vertices);
                continue;
            }

            out_batch = ri.MemAlloc(sizeof(*out_batch));
            memset(out_batch, 0, sizeof(*out_batch));
            out_batch->buffer = R_MakeVertexArrayObject(out_vertices, out_count);
            out_batch->num_vertices = out_count;
            out_batch->texture = material_id < material_count ? materials[material_id] : tr.texture[TEX_WHITE];
            out_batch->next = model->groups[group_index].batches;
            model->groups[group_index].batches = out_batch;
            wow_world.num_wmo_batches++;
            ri.MemFree(out_vertices);
        }
    } else {
        VERTEX *out_vertices = ri.MemAlloc(sizeof(VERTEX) * index_count);
        DWORD out_count = 0;
        FOR_LOOP(i, index_count) {
            WORD vertex_index = indices[i];
            DWORD poly_index = i / 3;
            DWORD material_id = 0;
            wowVec3_t p;
            wowVec2_t uv = { 0.0f, 0.0f };
            if (vertex_index >= vertex_count) {
                continue;
            }
            if (mopy && poly_index < mopy_count) {
                material_id = ((wowWmoPoly_t const *)mopy)[poly_index].material_id;
            }
            p = vertices[vertex_index];
            if (uvs && vertex_index < uv_count) {
                uv = uvs[vertex_index];
            }
            out_vertices[out_count++] = Wow_Vertex(p.x, p.y, p.z, uv.u, uv.v, COLOR32_WHITE);
            if ((i % 3) == 2) {
                wowWmoBatch_t *out_batch = ri.MemAlloc(sizeof(*out_batch));
                memset(out_batch, 0, sizeof(*out_batch));
                out_batch->buffer = R_MakeVertexArrayObject(out_vertices + out_count - 3, 3);
                out_batch->num_vertices = 3;
                out_batch->texture = material_id < material_count ? materials[material_id] : tr.texture[TEX_WHITE];
                out_batch->next = model->groups[group_index].batches;
                model->groups[group_index].batches = out_batch;
                wow_world.num_wmo_batches++;
            }
        }
        ri.MemFree(out_vertices);
    }

    ri.FS_FreeFile(data);
    return true;
}

static BOOL Wow_LoadWmoModel(wowWmoModel_t *model) {
    LPBYTE data = NULL;
    int size;
    DWORD offset = 0;
    DWORD group_count = 0;
    LPCSTR texture_blob = NULL;
    DWORD texture_blob_size = 0;
    BYTE const *materials_blob = NULL;
    DWORD material_count = 0;
    LPTEXTURE *materials = NULL;

    size = ri.FS_ReadFile(model->path, (void **)&data);
    if (size <= 0 || !data) {
        fprintf(stderr, "WoW WMO: missing root %s\n", model->path);
        return false;
    }

    while (offset + 8 <= (DWORD)size) {
        BYTE const *tag = data + offset;
        DWORD chunk_size = Wow_Read32(data + offset + 4);
        BYTE const *chunk = data + offset + 8;
        offset += 8;
        if (offset + chunk_size > (DWORD)size) {
            break;
        }
        if (Wow_TagEquals(tag, "DHOM") && chunk_size >= 8) {
            group_count = Wow_Read32(chunk + 4);
        } else if (Wow_TagEquals(tag, "XTOM")) {
            texture_blob = (LPCSTR)chunk;
            texture_blob_size = chunk_size;
        } else if (Wow_TagEquals(tag, "TMOM")) {
            materials_blob = chunk;
            material_count = chunk_size / 64;
        }
        offset += chunk_size;
    }

    if (!group_count) {
        fprintf(stderr, "WoW WMO: %s has no groups\n", model->path);
        ri.FS_FreeFile(data);
        return false;
    }

    if (material_count) {
        materials = ri.MemAlloc(sizeof(*materials) * material_count);
        memset(materials, 0, sizeof(*materials) * material_count);
        FOR_LOOP(i, material_count) {
            DWORD texture_offset = Wow_Read32(materials_blob + i * 64 + 0x0c);
            LPCSTR texture_path = Wow_StringAt(texture_blob, texture_blob_size, texture_offset);
            materials[i] = texture_path ? Wow_LoadTexture(texture_path) : tr.texture[TEX_WHITE];
        }
    }

    model->groups = ri.MemAlloc(sizeof(*model->groups) * group_count);
    memset(model->groups, 0, sizeof(*model->groups) * group_count);
    model->num_groups = group_count;
    FOR_LOOP(i, group_count) {
        Wow_LoadWmoGroup(model, i, materials, material_count);
    }

    if (materials) {
        ri.MemFree(materials);
    }
    ri.FS_FreeFile(data);
    model->loaded = true;
    return true;
}

static wowWmoModel_t *Wow_GetWmoModel(LPCSTR path) {
    wowWmoModel_t *model;

    if (!path || !*path) {
        return NULL;
    }
    for (model = wow_world.wmo_models; model; model = model->next) {
        if (!strcasecmp(model->path, path)) {
            return model->loaded ? model : NULL;
        }
    }

    model = ri.MemAlloc(sizeof(*model));
    memset(model, 0, sizeof(*model));
    snprintf(model->path, sizeof(model->path), "%s", path);
    model->next = wow_world.wmo_models;
    wow_world.wmo_models = model;
    wow_world.num_wmo_models++;
    if (!Wow_LoadWmoModel(model)) {
        wow_world.num_missing_wmos++;
        return NULL;
    }
    return model;
}

static void Wow_AddWmoInstance(LPCSTR path, wowMapObjDef_t const *def) {
    wowWmoModel_t *model = Wow_GetWmoModel(path);
    wowWmoInstance_t *instance;

    wow_world.num_wmos++;
    if (!model || !def) {
        return;
    }

    instance = ri.MemAlloc(sizeof(*instance));
    memset(instance, 0, sizeof(*instance));
    instance->model = model;
    Wow_InstanceMatrix(def, &instance->matrix);
    instance->next = wow_world.wmos;
    wow_world.wmos = instance;
}

static LPMODEL Wow_LoadDoodadModel(LPCSTR path) {
    wowDoodadModel_t *entry;

    if (!path || !*path) {
        return NULL;
    }
    for (entry = wow_world.doodad_models; entry; entry = entry->next) {
        if (!strcasecmp(entry->path, path)) {
            return entry->model;
        }
    }

    entry = ri.MemAlloc(sizeof(*entry));
    memset(entry, 0, sizeof(*entry));
    snprintf(entry->path, sizeof(entry->path), "%s", path);
    entry->model = R_LoadModel(path);
    entry->next = wow_world.doodad_models;
    wow_world.doodad_models = entry;
    wow_world.num_doodad_models++;
    if (!entry->model) {
        wow_world.num_missing_doodad_models++;
    }
    return entry->model;
}

static void Wow_AddDoodadInstance(LPCSTR model_path, wowDoodadDef_t const *def) {
    wowDoodadInstance_t *instance;
    LPMODEL model;

    if (!model_path || !*model_path || !def) {
        wow_world.num_missing_doodad_models++;
        return;
    }
    if (def->flags & 0x40) {
        wow_world.num_filedata_doodads++;
        return;
    }

    model = Wow_LoadDoodadModel(model_path);
    if (!model) {
        return;
    }

    instance = ri.MemAlloc(sizeof(*instance));
    memset(instance, 0, sizeof(*instance));
    instance->entity.origin = Wow_ObjectPoint(def->position);
    instance->entity.rotation = (VECTOR3){ def->rotation.x, def->rotation.y, def->rotation.z };
    instance->entity.scale = def->scale / 1024.0f;
    instance->entity.model = model;
    instance->entity.radius = 32.0f;
    instance->entity.flags = RF_NO_SHADOW;
    instance->next = wow_world.doodads;
    wow_world.doodads = instance;
    wow_world.num_doodad_instances++;
}

static void Wow_AddMarker(VERTEX *vertices, LPDWORD index, VECTOR3 p, float size, COLOR32 color) {
    VECTOR3 a = { p.x - size, p.y - size, p.z };
    VECTOR3 b = { p.x + size, p.y - size, p.z };
    VECTOR3 c = { p.x + size, p.y + size, p.z };
    VECTOR3 d = { p.x - size, p.y + size, p.z };
    VECTOR3 top = { p.x, p.y, p.z + size * 3.0f };
    vertices[(*index)++] = Wow_Vertex(a.x, a.y, a.z, 0, 0, color);
    vertices[(*index)++] = Wow_Vertex(b.x, b.y, b.z, 1, 0, color);
    vertices[(*index)++] = Wow_Vertex(top.x, top.y, top.z, 0.5f, 1, color);
    vertices[(*index)++] = Wow_Vertex(b.x, b.y, b.z, 0, 0, color);
    vertices[(*index)++] = Wow_Vertex(c.x, c.y, c.z, 1, 0, color);
    vertices[(*index)++] = Wow_Vertex(top.x, top.y, top.z, 0.5f, 1, color);
    vertices[(*index)++] = Wow_Vertex(c.x, c.y, c.z, 0, 0, color);
    vertices[(*index)++] = Wow_Vertex(d.x, d.y, d.z, 1, 0, color);
    vertices[(*index)++] = Wow_Vertex(top.x, top.y, top.z, 0.5f, 1, color);
    vertices[(*index)++] = Wow_Vertex(d.x, d.y, d.z, 0, 0, color);
    vertices[(*index)++] = Wow_Vertex(a.x, a.y, a.z, 1, 0, color);
    vertices[(*index)++] = Wow_Vertex(top.x, top.y, top.z, 0.5f, 1, color);
}

static VERTEX *Wow_AppendMarkers(VERTEX *old_vertices,
                                 LPDWORD old_count,
                                 BYTE const *chunk,
                                 DWORD size,
                                 BYTE const *name_blob,
                                 DWORD name_blob_size,
                                 DWORD const *name_offsets,
                                 DWORD name_offset_count,
                                 BOOL wmo) {
    DWORD record_size = wmo ? sizeof(wowMapObjDef_t) : sizeof(wowDoodadDef_t);
    DWORD count = size / record_size;
    DWORD new_count = *old_count + count * 12;
    VERTEX *vertices = ri.MemAlloc(sizeof(VERTEX) * MAX(new_count, 1));

    if (*old_count && old_vertices) {
        memcpy(vertices, old_vertices, sizeof(VERTEX) * *old_count);
        ri.MemFree(old_vertices);
    }

    FOR_LOOP(i, count) {
        VECTOR3 p;
        if (wmo) {
            wowMapObjDef_t const *def = (wowMapObjDef_t const *)(chunk + i * record_size);
            p = Wow_ObjectPoint(def->position);
            Wow_AddMarker(vertices, old_count, p, 18.0f, Wow_Color(90, 130, 255, 255));
            wow_world.num_wmos++;
        } else {
            wowDoodadDef_t const *def = (wowDoodadDef_t const *)(chunk + i * record_size);
            LPCSTR model_path = NULL;
            float model_scale = def->scale / 1024.0f;
            float radius = 0.0f;
            float marker_size;

            if (def->flags & 0x40) {
                wow_world.num_filedata_doodads++;
            } else {
                model_path = Wow_StringRefFromOffsets(name_blob, name_blob_size, name_offsets, name_offset_count, def->name_id);
                if (model_path) {
                    radius = Wow_LoadM2BoundsRadius(model_path);
                }
            }
            marker_size = radius > 0.0f ? radius * model_scale : model_scale * 8.0f;
            marker_size = MAX(5.0f, MIN(marker_size, 80.0f));
            p = Wow_ObjectPoint(def->position);
            Wow_AddMarker(vertices, old_count, p, marker_size, radius > 0.0f ? Wow_Color(90, 230, 130, 255) : Wow_Color(230, 210, 80, 255));
            wow_world.num_doodads++;
        }
    }

    return vertices;
}

static VERTEX *Wow_AppendDoodadErrorMarkers(VERTEX *old_vertices,
                                            LPDWORD old_count,
                                            BYTE const *chunk,
                                            DWORD size) {
    DWORD count = size / sizeof(wowDoodadDef_t);
    DWORD new_count = *old_count + count * 12;
    VERTEX *vertices = ri.MemAlloc(sizeof(VERTEX) * MAX(new_count, 1));

    if (*old_count && old_vertices) {
        memcpy(vertices, old_vertices, sizeof(VERTEX) * *old_count);
        ri.MemFree(old_vertices);
    }

    FOR_LOOP(i, count) {
        wowDoodadDef_t const *def = (wowDoodadDef_t const *)(chunk + i * sizeof(*def));
        if (def->flags & 0x40) {
            wow_world.num_filedata_doodads++;
            continue;
        }
        Wow_AddMarker(vertices,
                      old_count,
                      Wow_ObjectPoint(def->position),
                      6.0f,
                      Wow_Color(255, 255, 255, 255));
        wow_world.num_doodad_instances++;
    }

    return vertices;
}

static void Wow_LoadAdt(BYTE const *data, DWORD size, DWORD tile_x, DWORD tile_y) {
    DWORD offset = 0;
    char **textures = NULL;
    DWORD num_textures = 0;
    VERTEX *object_vertices = NULL;
    DWORD object_vertex_count = 0;
#if !WOW_DEBUG_DOODAD_ERROR_MESHES
    BYTE const *doodad_names = NULL;
    DWORD doodad_names_size = 0;
    DWORD const *doodad_offsets = NULL;
    DWORD doodad_offset_count = 0;
#endif
    BYTE const *wmo_names = NULL;
    DWORD wmo_names_size = 0;
    DWORD const *wmo_offsets = NULL;
    DWORD wmo_offset_count = 0;

    while (offset + 8 <= size) {
        BYTE const *tag = data + offset;
        DWORD chunk_size = Wow_Read32(data + offset + 4);
        BYTE const *chunk = data + offset + 8;

        offset += 8;
        if (offset + chunk_size > size) {
            break;
        }

        if (Wow_TagEquals(tag, "XETM")) {
            Wow_FreeStringList(textures, num_textures);
            textures = Wow_ParseStringBlock(chunk, chunk_size, &num_textures);
        } else if (Wow_TagEquals(tag, "XDMM")) {
#if !WOW_DEBUG_DOODAD_ERROR_MESHES
            doodad_names = chunk;
            doodad_names_size = chunk_size;
#endif
        } else if (Wow_TagEquals(tag, "DIMM")) {
#if !WOW_DEBUG_DOODAD_ERROR_MESHES
            doodad_offsets = (DWORD const *)chunk;
            doodad_offset_count = chunk_size / sizeof(DWORD);
#endif
        } else if (Wow_TagEquals(tag, "OMWM")) {
            wmo_names = chunk;
            wmo_names_size = chunk_size;
        } else if (Wow_TagEquals(tag, "DIWM")) {
            wmo_offsets = (DWORD const *)chunk;
            wmo_offset_count = chunk_size / sizeof(DWORD);
        } else if (Wow_TagEquals(tag, "FDDM")) {
            DWORD count = chunk_size / sizeof(wowDoodadDef_t);
            wow_world.num_doodads += count;
#if WOW_DEBUG_DOODAD_ERROR_MESHES
            object_vertices = Wow_AppendDoodadErrorMarkers(object_vertices,
                                                           &object_vertex_count,
                                                           chunk,
                                                           chunk_size);
#else
            FOR_LOOP(i, count) {
                wowDoodadDef_t const *def = (wowDoodadDef_t const *)(chunk + i * sizeof(*def));
                LPCSTR model_path = Wow_StringRefFromOffsets(doodad_names,
                                                              doodad_names_size,
                                                              doodad_offsets,
                                                              doodad_offset_count,
                                                              def->name_id);
                Wow_AddDoodadInstance(model_path, def);
            }
#endif
        } else if (Wow_TagEquals(tag, "FDOM")) {
#if WOW_DEBUG_OBJECT_MARKERS
            object_vertices = Wow_AppendMarkers(object_vertices,
                                                &object_vertex_count,
                                                chunk,
                                                chunk_size,
                                                wmo_names,
                                                wmo_names_size,
                                                wmo_offsets,
                                                wmo_offset_count,
                                                true);
#else
            DWORD count = chunk_size / sizeof(wowMapObjDef_t);
            FOR_LOOP(i, count) {
                wowMapObjDef_t const *def = (wowMapObjDef_t const *)(chunk + i * sizeof(*def));
                LPCSTR wmo_path = Wow_StringRefFromOffsets(wmo_names,
                                                           wmo_names_size,
                                                           wmo_offsets,
                                                           wmo_offset_count,
                                                           def->name_id);
                Wow_AddWmoInstance(wmo_path, def);
            }
#endif
        } else if (Wow_TagEquals(tag, "KNCM") && chunk_size >= 0x80) {
            DWORD sub = 0x80;
            wowVec3_t pos;
            DWORD index_x;
            DWORD index_y;
            DWORD chunk_flags;
            WORD holes;
            WORD pred_tex[8];
            BYTE alpha[4][WOW_ALPHA_TEXELS];
            wowLayer_t layers[4];
            DWORD layer_count = 0;
            BYTE const *mcal = NULL;
            DWORD mcal_size = 0;
            float heights[WOW_MCVT_COUNT];
            BYTE normals[WOW_MCVT_COUNT * 3];
            BOOL has_heights = false;
            BOOL has_normals = false;
            memset(heights, 0, sizeof(heights));
            memset(normals, 0, sizeof(normals));
            memset(pred_tex, 0, sizeof(pred_tex));
            memset(layers, 0, sizeof(layers));
            memset(alpha, 0, sizeof(alpha));
            chunk_flags = Wow_Read32(chunk + 0x00);
            index_x = Wow_Read32(chunk + 0x04);
            index_y = Wow_Read32(chunk + 0x08);
            memcpy(&pos, chunk + 0x68, sizeof(pos));
            memcpy(&holes, chunk + 0x3c, sizeof(holes));
            memcpy(pred_tex, chunk + 0x40, sizeof(pred_tex));

            while (sub + 8 <= chunk_size) {
                BYTE const *subtag = chunk + sub;
                DWORD sub_size = Wow_Read32(chunk + sub + 4);
                BYTE const *subchunk = chunk + sub + 8;
                BOOL is_mcnr = Wow_TagEquals(subtag, "RNCM");
                sub += 8;
                if (sub + sub_size > chunk_size) {
                    break;
                }
                if (Wow_TagEquals(subtag, "TVCM") && sub_size >= sizeof(heights)) {
                    memcpy(heights, subchunk, sizeof(heights));
                    has_heights = true;
                } else if (is_mcnr && sub_size >= sizeof(normals)) {
                    memcpy(normals, subchunk, sizeof(normals));
                    has_normals = true;
                } else if (Wow_TagEquals(subtag, "YLCM") && sub_size >= sizeof(wowLayer_t)) {
                    layer_count = MIN(sub_size / sizeof(wowLayer_t), 4);
                    memcpy(layers, subchunk, sizeof(*layers) * layer_count);
                } else if (Wow_TagEquals(subtag, "LACM")) {
                    mcal = subchunk;
                    mcal_size = sub_size;
                }
                sub += sub_size;
                if (is_mcnr && sub_size == sizeof(normals) && sub + 13 <= chunk_size) {
                    sub += 13;
                }
            }

            if (has_heights) {
                DWORD atlas_index_x = (tile_x - (WOW_START_TILE_X - WOW_ADT_RADIUS)) * 16 + index_x;
                DWORD atlas_index_y = (tile_y - (WOW_START_TILE_Y - WOW_ADT_RADIUS)) * 16 + index_y;
                (void)pred_tex;
                Wow_DecodeAlphaMaps(mcal, mcal_size, layers, layer_count, chunk_flags, alpha);
                Wow_UploadAlphaAtlasChunk(atlas_index_x, atlas_index_y, alpha);
                Wow_AddAdtChunk(pos, atlas_index_x, atlas_index_y, holes, alpha, layers, layer_count, textures, num_textures, heights, has_normals ? normals : NULL);
            }
        }

        offset += chunk_size;
    }

    if (object_vertex_count) {
        wowAdtChunk_t *marker_chunk = ri.MemAlloc(sizeof(*marker_chunk));
        memset(marker_chunk, 0, sizeof(*marker_chunk));
        marker_chunk->buffer = R_MakeVertexArrayObject(object_vertices, object_vertex_count);
        marker_chunk->textures[0] = tr.texture[TEX_WHITE];
        marker_chunk->textures[1] = tr.texture[TEX_WHITE];
        marker_chunk->textures[2] = tr.texture[TEX_WHITE];
        marker_chunk->textures[3] = tr.texture[TEX_WHITE];
        marker_chunk->num_vertices = object_vertex_count;
        marker_chunk->next = wow_world.chunks;
        wow_world.chunks = marker_chunk;
        wow_world.num_object_vertices += object_vertex_count;
        SAFE_DELETE(object_vertices, ri.MemFree);
    }
    SAFE_DELETE(object_vertices, ri.MemFree);

    Wow_FreeStringList(textures, num_textures);
    wow_world.num_adts++;
}

static void Wow_LoadAdtFile(DWORD tile_x, DWORD tile_y) {
    PATHSTR path;
    LPBYTE data = NULL;
    int size;

    snprintf(path,
             sizeof(path),
             "%s/%s_%u_%u.adt",
             wow_world.map_dir,
             wow_world.map_name,
             (unsigned)tile_x,
             (unsigned)tile_y);

    size = ri.FS_ReadFile(path, (void **)&data);
    if (size <= 0 || !data) {
        return;
    }
    Wow_LoadAdt(data, (DWORD)size, tile_x, tile_y);
    ri.FS_FreeFile(data);
}

static BYTE const *Wow_FindMainChunk(BYTE const *data, DWORD size, LPDWORD main_size) {
    DWORD offset = 0;

    while (offset + 8 <= size) {
        BYTE const *tag = data + offset;
        DWORD chunk_size = Wow_Read32(data + offset + 4);
        offset += 8;
        if (offset + chunk_size > size) {
            break;
        }
        if (Wow_TagEquals(tag, "NIAM")) {
            *main_size = chunk_size;
            return data + offset;
        }
        offset += chunk_size;
    }
    return NULL;
}

static void Wow_LoadWdtFlags(BYTE const *data, DWORD size) {
    DWORD offset = 0;

    wow_world.wdt_flags = 0;
    while (offset + 8 <= size) {
        BYTE const *tag = data + offset;
        DWORD chunk_size = Wow_Read32(data + offset + 4);
        BYTE const *chunk = data + offset + 8;
        offset += 8;
        if (offset + chunk_size > size) {
            break;
        }
        if (Wow_TagEquals(tag, "DHPM") && chunk_size >= sizeof(DWORD)) {
            wow_world.wdt_flags = Wow_Read32(chunk);
            return;
        }
        offset += chunk_size;
    }
}

static BOOL Wow_LoadWdtTiles(BYTE const *data, DWORD size) {
    DWORD main_size = 0;
    BYTE const *main_chunk = Wow_FindMainChunk(data, size, &main_size);
    DWORD max_entries;

    if (!main_chunk || main_size < sizeof(wowWdtMainEntry_t)) {
        return false;
    }

    Wow_LoadWdtFlags(data, size);
    max_entries = main_size / sizeof(wowWdtMainEntry_t);
    FOR_LOOP(tile_y, WOW_WDT_TILES) {
        FOR_LOOP(tile_x, WOW_WDT_TILES) {
            DWORD i = tile_y * WOW_WDT_TILES + tile_x;
            wowWdtMainEntry_t const *entry;
            if (i >= max_entries) {
                continue;
            }
            entry = (wowWdtMainEntry_t const *)(main_chunk + i * sizeof(*entry));
            wow_world.tiles[tile_x][tile_y].present = (entry->flags & 1) != 0;
        }
    }
    return true;
}

static LPCSTR Wow_DbcString(BYTE const *string_block, DWORD string_size, DWORD offset) {
    if (offset >= string_size) {
        return NULL;
    }
    return (LPCSTR)(string_block + offset);
}

static void Wow_LoadMapDbcFlags(void) {
    LPBYTE data = NULL;
    int size = ri.FS_ReadFile("DBFilesClient\\Map.dbc", (void **)&data);
    DWORD records;
    DWORD fields;
    DWORD record_size;
    DWORD string_size;
    BYTE const *records_base;
    BYTE const *strings_base;

    wow_world.use_weighted_blend = false;
    if (size <= 20 || !data) {
        return;
    }

    if (memcmp(data, "WDBC", 4) != 0) {
        ri.FS_FreeFile(data);
        return;
    }

    records = Wow_Read32(data + 4);
    fields = Wow_Read32(data + 8);
    record_size = Wow_Read32(data + 12);
    string_size = Wow_Read32(data + 16);
    records_base = data + 20;
    strings_base = records_base + records * record_size;

    if (fields == 0 || record_size < fields * sizeof(DWORD) ||
        20 + records * record_size + string_size > (DWORD)size) {
        ri.FS_FreeFile(data);
        return;
    }

    FOR_LOOP(record_index, records) {
        BYTE const *record = records_base + record_index * record_size;
        BOOL directory_matches = false;
        DWORD flags0 = Wow_Read32(record + (fields - 1) * sizeof(DWORD));

        FOR_LOOP(field_index, fields) {
            DWORD string_offset = Wow_Read32(record + field_index * sizeof(DWORD));
            LPCSTR value = Wow_DbcString(strings_base, string_size, string_offset);
            if (value && *value && !strcasecmp(value, wow_world.map_name)) {
                directory_matches = true;
                break;
            }
        }

        if (directory_matches) {
            wow_world.use_weighted_blend = (flags0 & 0x4) != 0;
            ri.FS_FreeFile(data);
            return;
        }
    }
    ri.FS_FreeFile(data);
}

static void Wow_LoadNearbyAdts(void) {
    int center_x = WOW_START_TILE_X;
    int center_y = WOW_START_TILE_Y;
    for (int y = center_y - WOW_ADT_RADIUS; y <= center_y + WOW_ADT_RADIUS; y++) {
        for (int x = center_x - WOW_ADT_RADIUS; x <= center_x + WOW_ADT_RADIUS; x++) {
            if (x < 0 || x >= WOW_WDT_TILES || y < 0 || y >= WOW_WDT_TILES) {
                continue;
            }
            if (wow_world.tiles[x][y].present) {
                Wow_LoadAdtFile((DWORD)x, (DWORD)y);
            }
        }
    }
}

void R_RegisterMap(LPCSTR mapFileName) {
    PATHSTR path;
    LPBYTE data = NULL;
    int size;

    Wow_FreeWorld();
    Wow_NormalizeMapPath(mapFileName, path, sizeof(path));
    Wow_SetMapNames(path);
    Wow_LoadMapDbcFlags();

    size = ri.FS_ReadFile(path, (void **)&data);
    if (size <= 0 || !data) {
        fprintf(stderr, "R_RegisterMap: failed to read WoW WDT %s\n", path);
        return;
    }

    if (!Wow_LoadWdtTiles(data, (DWORD)size)) {
        fprintf(stderr, "R_RegisterMap: failed to parse WoW WDT tiles %s\n", path);
        ri.FS_FreeFile(data);
        return;
    }

    Wow_LoadNearbyAdts();
    fprintf(stderr,
            "R_RegisterMap: WoW map %s loaded chunks=%u doodads=%u rendered_doodads=%u doodad_models=%u missing_doodad_models=%u wmos=%u wmo_models=%u wmo_batches=%u missing_wmos=%u weighted_blend=%d doodad_error_meshes=%d\n",
            path,
            (unsigned)wow_world.num_chunks,
            (unsigned)wow_world.num_doodads,
            (unsigned)wow_world.num_doodad_instances,
            (unsigned)wow_world.num_doodad_models,
            (unsigned)wow_world.num_missing_doodad_models,
            (unsigned)wow_world.num_wmos,
            (unsigned)wow_world.num_wmo_models,
            (unsigned)wow_world.num_wmo_batches,
            (unsigned)wow_world.num_missing_wmos,
            wow_world.use_weighted_blend ? 1 : 0,
            WOW_DEBUG_DOODAD_ERROR_MESHES ? 1 : 0);
    ri.FS_FreeFile(data);
}

void R_DrawWorld(void) {
    static GLfloat const identity[16] = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f,
    };
    wowAdtChunk_t *chunk;

    if (tr.viewDef.rdflags & RDF_NOWORLDMODEL) {
        return;
    }

    if (!wow_world.chunks) {
        static BOOL logged_no_chunks = false;
        if (!logged_no_chunks) {
            fprintf(stderr, "R_DrawWorld: WoW world has no loaded terrain chunks\n");
            logged_no_chunks = true;
        }
        return;
    }

    Wow_InitTerrainShader();
    if (!wow_terrain_shader) {
        static BOOL logged_no_shader = false;
        if (!logged_no_shader) {
            fprintf(stderr, "R_DrawWorld: WoW terrain shader failed to initialize\n");
            logged_no_shader = true;
        }
        return;
    }

    R_Call(glUseProgram, wow_terrain_shader->progid);
    R_Call(glUniformMatrix4fv, wow_terrain_shader->uViewProjectionMatrix, 1, GL_FALSE, tr.viewDef.viewProjectionMatrix.v);
    R_Call(glUniformMatrix4fv, wow_terrain_shader->uModelMatrix, 1, GL_FALSE, identity);
    R_Call(glUniform1i, wow_uUseWeightedBlend, wow_world.use_weighted_blend ? 1 : 0);
    R_Call(glEnable, GL_DEPTH_TEST);
    R_Call(glDepthMask, GL_TRUE);
    R_Call(glDepthFunc, GL_LEQUAL);
    R_Call(glDisable, GL_CULL_FACE);
    R_Call(glDisable, GL_BLEND);

    for (chunk = wow_world.chunks; chunk; chunk = chunk->next) {
        if (!chunk->buffer || !chunk->num_vertices) {
            continue;
        }
        R_BindTexture(chunk->textures[0] ? chunk->textures[0] : tr.texture[TEX_WHITE], 0);
        R_BindTexture(chunk->textures[1] ? chunk->textures[1] : chunk->textures[0], 1);
        R_BindTexture(chunk->textures[2] ? chunk->textures[2] : chunk->textures[0], 2);
        R_BindTexture(chunk->textures[3] ? chunk->textures[3] : chunk->textures[0], 3);
        R_BindTexture(chunk->alpha_texture ? chunk->alpha_texture : tr.texture[TEX_WHITE], 4);
        R_Call(glUniform2f, wow_uAlphaOrigin, (GLfloat)chunk->alpha_index_x, (GLfloat)chunk->alpha_index_y);
        R_DrawBuffer(chunk->buffer, chunk->num_vertices);
    }

    for (wowWmoInstance_t *wmo = wow_world.wmos; wmo; wmo = wmo->next) {
        if (!wmo->model || !wmo->model->groups) {
            continue;
        }
        R_Call(glUniformMatrix4fv, wow_terrain_shader->uModelMatrix, 1, GL_FALSE, wmo->matrix.v);
        R_Call(glUniform1i, wow_uUseWeightedBlend, 0);
        R_Call(glUniform2f, wow_uAlphaOrigin, 0.0f, 0.0f);
        FOR_LOOP(group_index, wmo->model->num_groups) {
            for (wowWmoBatch_t *batch = wmo->model->groups[group_index].batches; batch; batch = batch->next) {
                if (!batch->buffer || !batch->num_vertices) {
                    continue;
                }
                R_BindTexture(batch->texture ? batch->texture : tr.texture[TEX_WHITE], 0);
                R_BindTexture(batch->texture ? batch->texture : tr.texture[TEX_WHITE], 1);
                R_BindTexture(batch->texture ? batch->texture : tr.texture[TEX_WHITE], 2);
                R_BindTexture(batch->texture ? batch->texture : tr.texture[TEX_WHITE], 3);
                R_BindTexture(tr.texture[TEX_WHITE], 4);
                R_DrawBuffer(batch->buffer, batch->num_vertices);
            }
        }
    }
    R_Call(glUniformMatrix4fv, wow_terrain_shader->uModelMatrix, 1, GL_FALSE, identity);
    R_Call(glUniform1i, wow_uUseWeightedBlend, wow_world.use_weighted_blend ? 1 : 0);

    for (wowDoodadInstance_t *doodad = wow_world.doodads; doodad; doodad = doodad->next) {
        R_RenderModel(&doodad->entity);
    }

    R_Call(glDepthMask, GL_TRUE);
    R_Call(glDisable, GL_BLEND);
    R_Call(glEnable, GL_CULL_FACE);

    if (wow_world.object_buffer && wow_world.num_object_vertices) {
        R_BindTexture(tr.texture[TEX_WHITE], 0);
        R_DrawBuffer(wow_world.object_buffer, wow_world.num_object_vertices);
    }
}

void R_DrawAlphaSurfaces(void) {
}

void R_DrawTerrainShadows(void) {
}

void R_RenderRectSplat(LPCVECTOR2 mins, LPCVECTOR2 maxs, LPCTEXTURE texture, LPCSHADER shader, COLOR32 color) {
    (void)mins;
    (void)maxs;
    (void)texture;
    (void)shader;
    (void)color;
}

void R_RenderSplat(LPCVECTOR2 position, float radius, LPCTEXTURE texture, LPCSHADER shader, COLOR32 color) {
    (void)position;
    (void)radius;
    (void)texture;
    (void)shader;
    (void)color;
}

VECTOR2 GetWar3MapSize(LPCWAR3MAP war3Map) {
    (void)war3Map;
    return (VECTOR2){ 0.0f, 0.0f };
}

float GetAccurateHeightAtPoint(float sx, float sy) {
    (void)sx;
    (void)sy;
    return 0.0f;
}

FLOAT R_GetHeightAtPoint(FLOAT x, FLOAT y) {
    (void)x;
    (void)y;
    return 0.0f;
}

bool R_TraceLocation(viewDef_t const *viewdef, FLOAT x, FLOAT y, LPVECTOR3 output) {
    LINE3 const line = R_LineForScreenPoint(viewdef, x, y);
    float const dz = line.b.z - line.a.z;
    float t;

    if (fabsf(dz) < 0.0001f || !output) {
        return false;
    }

    t = -line.a.z / dz;
    output->x = line.a.x + (line.b.x - line.a.x) * t;
    output->y = line.a.y + (line.b.y - line.a.y) * t;
    output->z = 0.0f;
    return true;
}
