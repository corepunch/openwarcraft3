#include "r_sc2map.h"

#include "games/starcraft-2/common/sc2_map.c"
#include "games/warcraft-3/renderer/w3m/r_terrain_layers.c"

static LPMAPSEGMENT sc2_terrain_segment;

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

static void r_sc2_push_vertex(VERTEX *v, FLOAT x, FLOAT y, FLOAT z, FLOAT u, FLOAT t, BYTE alpha) {
    v->position = (VECTOR3){ x, y, z };
    v->texcoord = (VECTOR2){ u, t };
    v->normal = (VECTOR3){ 0.0f, 0.0f, 1.0f };
    v->color = (COLOR32){ 255, 255, 255, alpha };
}

static BYTE r_sc2_texture_weight(sc2Map_t const *map, DWORD texture, DWORD x, DWORD y) {
    LPBYTE mask;
    DWORD mx, my;

    if (!map->num_texture_masks || texture == 0) {
        return texture == 0 ? 255 : 0;
    }
    if (texture >= map->num_texture_masks || !map->texture_masks[texture]) {
        return 0;
    }
    mask = map->texture_masks[texture];
    mx = MIN(map->texture_mask_width - 1, x * map->texture_mask_width / MAX(1, map->width));
    my = MIN(map->texture_mask_height - 1, y * map->texture_mask_height / MAX(1, map->height));
    return (BYTE)(mask[mx + my * map->texture_mask_width] * 17);
}

static FLOAT r_sc2_height_at_grid(sc2Map_t const *map, DWORD x, DWORD y) {
    if (!map->cliff_levels || !map->cliff_level_width || !map->cliff_level_height) {
        return 0.0f;
    }
    x = MIN(map->cliff_level_width - 1, x * map->cliff_level_width / MAX(1, map->width));
    y = MIN(map->cliff_level_height - 1, y * map->cliff_level_height / MAX(1, map->height));
    return map->cliff_levels[x + y * map->cliff_level_width];
}

static BOOL r_sc2_tile_hole(sc2Map_t const *map, DWORD x, DWORD y) {
    DWORD fx, fy;

    if (!map->cell_flags || !map->cell_flags_width || !map->cell_flags_height) {
        return false;
    }
    fx = MIN(map->cell_flags_width - 1, x * map->cell_flags_width / MAX(1, map->width));
    fy = MIN(map->cell_flags_height - 1, y * map->cell_flags_height / MAX(1, map->height));
    return map->cell_flags[fx + fy * map->cell_flags_width] == 0x03;
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

static LPMAPLAYER r_sc2_build_terrain_layer(sc2Map_t const *map, DWORD layer) {
    BOX2 bounds;
    DWORD w, h, num_vertices;
    VERTEX *out;
    VERTEX *vertices;
    LPMAPLAYER map_layer;

    if (!map || !map->width || !map->height)
        return NULL;

    w = MIN(map->width, 192);
    h = MIN(map->height, 192);
    num_vertices = w * h * 6;
    vertices = ri.MemAlloc(num_vertices * sizeof(*vertices));
    out = vertices;
    bounds = SC2_MapBounds();

    FOR_LOOP(y, h) {
        FOR_LOOP(x, w) {
            FLOAT x0 = bounds.min.x + x * map->cell_size;
            FLOAT y0 = bounds.min.y + y * map->cell_size;
            FLOAT x1 = x0 + map->cell_size;
            FLOAT y1 = y0 + map->cell_size;
            BYTE a = r_sc2_texture_weight(map, layer, x, y);
            FLOAT u0 = x0 / 8.0f;
            FLOAT v0 = y0 / 8.0f;
            FLOAT u1 = x1 / 8.0f;
            FLOAT v1 = y1 / 8.0f;
            FLOAT z00 = r_sc2_height_at_grid(map, x, y);
            FLOAT z10 = r_sc2_height_at_grid(map, x + 1, y);
            FLOAT z11 = r_sc2_height_at_grid(map, x + 1, y + 1);
            FLOAT z01 = r_sc2_height_at_grid(map, x, y + 1);
            if (r_sc2_tile_hole(map, x, y)) {
                memset(out, 0, 6 * sizeof(*out));
                out += 6;
                continue;
            }
            r_sc2_push_vertex(out++, x0, y0, z00, u0, v0, a);
            r_sc2_push_vertex(out++, x1, y0, z10, u1, v0, a);
            r_sc2_push_vertex(out++, x1, y1, z11, u1, v1, a);
            r_sc2_push_vertex(out++, x0, y0, z00, u0, v0, a);
            r_sc2_push_vertex(out++, x1, y1, z11, u1, v1, a);
            r_sc2_push_vertex(out++, x0, y1, z01, u0, v1, a);
        }
    }

    map_layer = ri.MemAlloc(sizeof(*map_layer));
    memset(map_layer, 0, sizeof(*map_layer));
    map_layer->type = MAPLAYERTYPE_GROUND;
    map_layer->texture = R_LoadTexture(map->terrain_textures[layer].diffuse);
    map_layer->buffer = R_MakeVertexArrayObject(vertices, num_vertices);
    map_layer->num_vertices = num_vertices;
    ri.MemFree(vertices);
    return map_layer;
}

static void r_sc2_cliff_quad(VERTEX **out,
                             FLOAT x0,
                             FLOAT y0,
                             FLOAT x1,
                             FLOAT y1,
                             FLOAT low,
                             FLOAT high) {
    VERTEX *v = *out;

    r_sc2_push_vertex(v++, x0, y0, low, 0.0f, 1.0f, 255);
    r_sc2_push_vertex(v++, x1, y1, low, 1.0f, 1.0f, 255);
    r_sc2_push_vertex(v++, x1, y1, high, 1.0f, 0.0f, 255);
    r_sc2_push_vertex(v++, x0, y0, low, 0.0f, 1.0f, 255);
    r_sc2_push_vertex(v++, x1, y1, high, 1.0f, 0.0f, 255);
    r_sc2_push_vertex(v++, x0, y0, high, 0.0f, 0.0f, 255);
    *out = v;
}

static LPMAPLAYER r_sc2_build_cliff_layer(sc2Map_t const *map) {
    BOX2 bounds;
    DWORD w, h, capacity;
    VERTEX *vertices;
    VERTEX *out;
    LPMAPLAYER map_layer;

    if (!map || !map->cliff_levels || !map->num_terrain_textures) {
        return NULL;
    }
    w = MIN(map->width, 192);
    h = MIN(map->height, 192);
    capacity = w * h * 12;
    vertices = ri.MemAlloc(capacity * sizeof(*vertices));
    out = vertices;
    bounds = SC2_MapBounds();

    FOR_LOOP(y, h) {
        FOR_LOOP(x, w) {
            FLOAT z = r_sc2_height_at_grid(map, x, y);
            FLOAT zr = r_sc2_height_at_grid(map, x + 1, y);
            FLOAT zb = r_sc2_height_at_grid(map, x, y + 1);
            FLOAT x0 = bounds.min.x + x * map->cell_size;
            FLOAT y0 = bounds.min.y + y * map->cell_size;
            FLOAT x1 = x0 + map->cell_size;
            FLOAT y1 = y0 + map->cell_size;

            if (zr != z && (DWORD)(out - vertices) + 6 <= capacity) {
                r_sc2_cliff_quad(&out, x1, y0, x1, y1, MIN(z, zr), MAX(z, zr));
            }
            if (zb != z && (DWORD)(out - vertices) + 6 <= capacity) {
                r_sc2_cliff_quad(&out, x0, y1, x1, y1, MIN(z, zb), MAX(z, zb));
            }
        }
    }
    if (out == vertices) {
        ri.MemFree(vertices);
        return NULL;
    }

    map_layer = ri.MemAlloc(sizeof(*map_layer));
    memset(map_layer, 0, sizeof(*map_layer));
    map_layer->type = MAPLAYERTYPE_CLIFF;
    map_layer->texture = R_LoadTexture(map->terrain_textures[0].diffuse);
    map_layer->buffer = R_MakeVertexArrayObject(vertices, (DWORD)(out - vertices));
    map_layer->num_vertices = (DWORD)(out - vertices);
    ri.MemFree(vertices);
    return map_layer;
}

static void r_sc2_build_terrain(sc2Map_t const *map) {
    DWORD layers;
    BOX2 bounds;
    FLOAT max_z = 1.0f;

    r_sc2_release_terrain();
    if (!map || !map->width || !map->height)
        return;

    sc2_terrain_segment = ri.MemAlloc(sizeof(*sc2_terrain_segment));
    memset(sc2_terrain_segment, 0, sizeof(*sc2_terrain_segment));
    layers = MAX(1, map->num_terrain_textures);
    layers = MIN(layers, SC2_MAX_TERRAIN_TEXTURES);
    FOR_LOOP(layer, layers) {
        r_sc2_add_layer(&sc2_terrain_segment->layers, r_sc2_build_terrain_layer(map, layer));
    }
    r_sc2_add_layer(&sc2_terrain_segment->layers, r_sc2_build_cliff_layer(map));

    bounds = SC2_MapBounds();
    if (map->cliff_levels) {
        FOR_LOOP(i, map->cliff_level_width * map->cliff_level_height) {
            max_z = MAX(max_z, map->cliff_levels[i] + 1.0f);
        }
    }
    sc2_terrain_segment->bbox = (BOX3){
        .min = { bounds.min.x, bounds.min.y, -1.0f },
        .max = { bounds.max.x, bounds.max.y, max_z },
    };
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
    R_Call(glUseProgram, tr.shader[SHADER_DEFAULT]->progid);
    R_Call(glUniformMatrix4fv, tr.shader[SHADER_DEFAULT]->uViewProjectionMatrix, 1, GL_FALSE, tr.viewDef.viewProjectionMatrix.v);
    R_Call(glUniformMatrix4fv, tr.shader[SHADER_DEFAULT]->uModelMatrix, 1, GL_FALSE, model_matrix.v);
    R_Call(glEnable, GL_DEPTH_TEST);
    R_Call(glDepthMask, GL_TRUE);
    R_Call(glDepthFunc, GL_LEQUAL);
    R_DrawTerrainSegment(sc2_terrain_segment, (1 << MAPLAYERTYPE_GROUND) | (1 << MAPLAYERTYPE_CLIFF));
}

bool R_SC2TraceLocation(viewDef_t const *viewdef, FLOAT x, FLOAT y, LPVECTOR3 output) {
    LINE3 line;
    FLOAT t;
    if (!viewdef || !output)
        return false;
    line = R_LineForScreenPoint(viewdef, x, y);
    if (fabsf(line.b.z - line.a.z) < 0.0001f)
        return false;
    t = (SC2_MapHeightAtPoint(0, 0) - line.a.z) / (line.b.z - line.a.z);
    if (t < 0.0f || t > 1.0f)
        return false;
    *output = (VECTOR3){
        line.a.x + (line.b.x - line.a.x) * t,
        line.a.y + (line.b.y - line.a.y) * t,
        SC2_MapHeightAtPoint(0, 0),
    };
    return true;
}

FLOAT R_SC2GetHeightAtPoint(FLOAT x, FLOAT y) {
    return SC2_MapHeightAtPoint(x, y);
}

VECTOR2 R_SC2WorldSize(void) {
    sc2Map_t const *map = SC2_MapCurrent();
    return (VECTOR2){ map->width * map->cell_size, map->height * map->cell_size };
}
