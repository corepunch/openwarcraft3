#include "r_wowmap.h"

static float Wow_GrassClamp(float value, float min_value, float max_value) {
    return MAX(min_value, MIN(value, max_value));
}

static DWORD Wow_GrassHash(DWORD value) {
    value ^= value >> 16;
    value *= 0x7feb352dU;
    value ^= value >> 15;
    value *= 0x846ca68bU;
    value ^= value >> 16;
    return value;
}

static float Wow_GrassRandom(LPDWORD seed) {
    *seed = Wow_GrassHash(*seed + 0x9e3779b9U);
    return (float)(*seed & 0xffff) / 65535.0f;
}

static DWORD Wow_GrassLayerSlot(wowLayer_t const *layers, DWORD layer_count, DWORD wanted_layer) {
    DWORD unique_texture_ids[4] = { 0, 0, 0, 0 };
    DWORD unique_count = 0;

    FOR_LOOP(layer_index, MIN(layer_count, 4)) {
        DWORD slot = Wow_AlphaSlotForTexture(unique_texture_ids, &unique_count, layers[layer_index].texture_id);
        if (layer_index == wanted_layer) {
            return slot;
        }
    }
    return 0;
}

static BYTE Wow_GrassLayerCoverage(BYTE const alpha[4][WOW_ALPHA_TEXELS],
                                   wowLayer_t const *layers,
                                   DWORD layer_count,
                                   DWORD layer_index,
                                   DWORD alpha_index) {
    DWORD slot;

    if (!layers || layer_index >= layer_count || alpha_index >= WOW_ALPHA_TEXELS) {
        return 0;
    }

    slot = Wow_GrassLayerSlot(layers, layer_count, layer_index);
    if (layer_index == 0) {
        int coverage = 255;
        FOR_LOOP(i, 3) {
            coverage -= alpha[i + 1][alpha_index];
        }
        return (BYTE)MAX(0, MIN(coverage, 255));
    }
    return alpha[slot][alpha_index];
}

static BYTE Wow_GrassEffectCoverage(BYTE const alpha[4][WOW_ALPHA_TEXELS],
                                    wowLayer_t const *layers,
                                    DWORD layer_count,
                                    DWORD alpha_index) {
    BYTE best = 0;

    if (!layers) {
        return 0;
    }

    FOR_LOOP(layer_index, MIN(layer_count, 4)) {
        BYTE coverage;
        if (!layers[layer_index].effect_id) {
            continue;
        }
        coverage = Wow_GrassLayerCoverage(alpha, layers, layer_count, layer_index, alpha_index);
        if (coverage > best) {
            best = coverage;
        }
    }
    return best;
}

static void Wow_SetGrassVertex(VERTEX *vertex, VECTOR3 position, VECTOR3 normal, float u, float v, COLOR32 color) {
    *vertex = Wow_Vertex(position.x, position.y, position.z, u, v, color);
    vertex->normal = normal;
}

static void Wow_PushGrassBlade(VERTEX *vertices,
                               LPDWORD count,
                               LPCVECTOR3 origin,
                               float angle,
                               float width,
                               float height,
                               COLOR32 color) {
    VECTOR3 right = { cosf(angle) * width, sinf(angle) * width, 0.0f };
    VECTOR3 normal = { -right.y, right.x, 0.30f };
    VECTOR3 base_left = { origin->x - right.x, origin->y - right.y, origin->z };
    VECTOR3 base_right = { origin->x + right.x, origin->y + right.y, origin->z };
    VECTOR3 top = { origin->x, origin->y, origin->z + height };

    if (Vector3_lengthsq(&normal) <= 0.000001f) {
        normal = (VECTOR3){ 0.0f, 0.0f, 1.0f };
    } else {
        Vector3_normalize(&normal);
    }

    Wow_SetGrassVertex(&vertices[(*count)++], base_left, normal, 0.0f, 0.0f, color);
    Wow_SetGrassVertex(&vertices[(*count)++], base_right, normal, 1.0f, 0.0f, color);
    Wow_SetGrassVertex(&vertices[(*count)++], top, normal, 0.5f, 1.0f, color);
}

void Wow_BuildGrassForChunk(wowAdtChunk_t *chunk,
                            BYTE const alpha[4][WOW_ALPHA_TEXELS],
                            wowLayer_t const *layers,
                            DWORD layer_count) {
    enum {
        WOW_GRASS_SAMPLES_PER_AXIS = 8 / WOW_GRASS_CELL_STEP,
        WOW_GRASS_MAX_CLUMPS_PER_SAMPLE = 4,
        WOW_GRASS_MAX_VERTICES = WOW_GRASS_SAMPLES_PER_AXIS * WOW_GRASS_SAMPLES_PER_AXIS *
                                 WOW_GRASS_MAX_CLUMPS_PER_SAMPLE * WOW_GRASS_VERTICES_PER_CLUMP,
    };
    VERTEX *vertices;
    DWORD num_vertices = 0;
    float density = Wow_GrassClamp(WOW_GRASS_DENSITY, 0.0f, 2.0f);

    if (!chunk || !alpha || !layers || layer_count == 0 || density <= 0.0f) {
        return;
    }

    vertices = ri.MemAlloc(sizeof(VERTEX) * WOW_GRASS_MAX_VERTICES);
    if (!vertices) {
        return;
    }

    chunk->grass_bounds = Wow_EmptyBounds();
    for (int row = 0; row < 8; row += WOW_GRASS_CELL_STEP) {
        for (int col = 0; col < 8; col += WOW_GRASS_CELL_STEP) {
            DWORD seed = (chunk->alpha_index_x * 73856093U) ^
                         (chunk->alpha_index_y * 19349663U) ^
                         ((DWORD)row * 83492791U) ^
                         ((DWORD)col * 2654435761U);
            float local_row = row + 0.20f + Wow_GrassRandom(&seed) * (WOW_GRASS_CELL_STEP - 0.40f);
            float local_col = col + 0.20f + Wow_GrassRandom(&seed) * (WOW_GRASS_CELL_STEP - 0.40f);
            int cell_row = (int)floorf(MIN(local_row, 7.999f));
            int cell_col = (int)floorf(MIN(local_col, 7.999f));
            int alpha_x = MAX(0, MIN((int)(local_col * 8.0f), 63));
            int alpha_y = MAX(0, MIN((int)(local_row * 8.0f), 63));
            BYTE coverage = Wow_GrassEffectCoverage(alpha, layers, layer_count, alpha_y * 64 + alpha_x);
            int clumps;

            if (coverage < 64) {
                continue;
            }

            clumps = MAX(1, (int)ceilf(((float)coverage / 255.0f) * density * 2.0f));
            clumps = MIN(clumps, WOW_GRASS_MAX_CLUMPS_PER_SAMPLE);
            FOR_LOOP(clump, clumps) {
                float row_jitter = local_row + (Wow_GrassRandom(&seed) - 0.5f) * 0.45f;
                float col_jitter = local_col + (Wow_GrassRandom(&seed) - 0.5f) * 0.45f;
                float height;
                VECTOR3 origin;
                float angle;
                float blade_height;
                float blade_width;
                BYTE green;
                COLOR32 color;

                row_jitter = Wow_GrassClamp(row_jitter, 0.001f, 7.999f);
                col_jitter = Wow_GrassClamp(col_jitter, 0.001f, 7.999f);
                cell_row = (int)floorf(row_jitter);
                cell_col = (int)floorf(col_jitter);
                if (!Wow_HeightInCell(chunk->heights,
                                      cell_row,
                                      cell_col,
                                      row_jitter - cell_row,
                                      col_jitter - cell_col,
                                      &height)) {
                    continue;
                }

                origin = (VECTOR3){
                    chunk->position.x - row_jitter * WOW_ADT_UNIT_SIZE,
                    chunk->position.y - col_jitter * WOW_ADT_UNIT_SIZE,
                    chunk->position.z + height + 0.04f,
                };
                angle = Wow_GrassRandom(&seed) * 6.2831853f;
                blade_height = 1.05f + Wow_GrassRandom(&seed) * 1.25f;
                blade_width = 0.22f + Wow_GrassRandom(&seed) * 0.22f;
                green = (BYTE)(105 + Wow_GrassRandom(&seed) * 70.0f);
                color = Wow_Color((BYTE)(42 + Wow_GrassRandom(&seed) * 22.0f),
                                  green,
                                  (BYTE)(28 + Wow_GrassRandom(&seed) * 18.0f),
                                  220);

                if (num_vertices + WOW_GRASS_VERTICES_PER_CLUMP > WOW_GRASS_MAX_VERTICES) {
                    break;
                }
                Wow_PushGrassBlade(vertices, &num_vertices, &origin, angle, blade_width, blade_height, color);
                Wow_PushGrassBlade(vertices, &num_vertices, &origin, angle + 1.5707963f, blade_width * 0.85f, blade_height * 0.92f, color);
                Wow_AddBoundsPoint(&chunk->grass_bounds, &origin);
                origin.z += blade_height;
                Wow_AddBoundsPoint(&chunk->grass_bounds, &origin);
            }
        }
    }

    if (num_vertices) {
        chunk->grass_buffer = R_MakeVertexArrayObject(vertices, num_vertices);
        chunk->num_grass_vertices = num_vertices;
        wow_world.num_grass_chunks++;
        wow_world.num_grass_vertices += num_vertices;
    }
    ri.MemFree(vertices);
}

BOOL Wow_GrassChunkInRange(wowAdtChunk_t const *chunk) {
    VECTOR3 camera_origin;
    VECTOR3 center;
    float dx = 0.0f;
    float dy = 0.0f;
    float draw_distance = WOW_GRASS_DRAW_DISTANCE;

    if (!chunk || !chunk->grass_buffer || !chunk->num_grass_vertices || draw_distance <= 0.0f) {
        return false;
    }

    camera_origin = tr.viewDef.camerastate[0].origin;
    if (camera_origin.x < chunk->grass_bounds.min.x) {
        dx = chunk->grass_bounds.min.x - camera_origin.x;
    } else if (camera_origin.x > chunk->grass_bounds.max.x) {
        dx = camera_origin.x - chunk->grass_bounds.max.x;
    }
    if (camera_origin.y < chunk->grass_bounds.min.y) {
        dy = chunk->grass_bounds.min.y - camera_origin.y;
    } else if (camera_origin.y > chunk->grass_bounds.max.y) {
        dy = camera_origin.y - chunk->grass_bounds.max.y;
    }
    if (dx * dx + dy * dy > draw_distance * draw_distance) {
        return false;
    }

    center = (VECTOR3){
        (chunk->grass_bounds.min.x + chunk->grass_bounds.max.x) * 0.5f,
        (chunk->grass_bounds.min.y + chunk->grass_bounds.max.y) * 0.5f,
        (chunk->grass_bounds.min.z + chunk->grass_bounds.max.z) * 0.5f,
    };
    return Frustum_ContainsSphere(&tr.viewDef.frustum, &(SPHERE3){
        .center = center,
        .radius = WOW_ADT_CHUNK_SIZE,
    });
}

void Wow_DrawGrass(void) {
    wowAdtChunk_t *chunk;
    VECTOR3 camera_origin;
    DWORD drawn_chunks = 0;
    DWORD drawn_vertices = 0;
    float draw_distance;

    if (!wow_world.num_grass_vertices) {
        return;
    }

    Wow_InitGrassShader();
    if (!wow_grass_shader) {
        return;
    }

    camera_origin = tr.viewDef.camerastate[0].origin;
    draw_distance = WOW_GRASS_DRAW_DISTANCE;

    R_Call(glUseProgram, wow_grass_shader->progid);
    R_Call(glUniformMatrix4fv, wow_grass_shader->uViewProjectionMatrix, 1, GL_FALSE, tr.viewDef.viewProjectionMatrix.v);
    R_Call(glUniformMatrix4fv, wow_grass_shader->uLightMatrix, 1, GL_FALSE, tr.viewDef.lightMatrix.v);
    R_Call(glUniform1f, wow_uGrassTime, (GLfloat)tr.viewDef.time * 0.001f);
    R_Call(glUniform3f, wow_uGrassCameraOrigin, camera_origin.x, camera_origin.y, camera_origin.z);
    R_Call(glUniform1f, wow_uGrassDrawDistance, draw_distance);
    R_Call(glEnable, GL_DEPTH_TEST);
    R_Call(glDepthFunc, GL_LEQUAL);
    R_Call(glDisable, GL_CULL_FACE);
    R_Call(glEnable, GL_BLEND);
    R_Call(glBlendFunc, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    R_Call(glDepthMask, GL_FALSE);

    for (chunk = wow_world.chunks; chunk; chunk = chunk->next) {
        if (!Wow_GrassChunkInRange(chunk)) {
            continue;
        }
        R_DrawBuffer(chunk->grass_buffer, chunk->num_grass_vertices);
        drawn_chunks++;
        drawn_vertices += chunk->num_grass_vertices;
    }

    R_Call(glDepthMask, GL_TRUE);
    R_Call(glDisable, GL_BLEND);

    {
        static int logged_x = -1;
        static int logged_y = -1;
        if (logged_x != wow_world.adt_center_x || logged_y != wow_world.adt_center_y) {
            logged_x = wow_world.adt_center_x;
            logged_y = wow_world.adt_center_y;
            fprintf(stderr,
                    "R_DrawWorld: grass chunks=%u/%u vertices=%u/%u draw_distance=%.0f\n",
                    (unsigned)drawn_chunks,
                    (unsigned)wow_world.num_grass_chunks,
                    (unsigned)drawn_vertices,
                    (unsigned)wow_world.num_grass_vertices,
                    (double)draw_distance);
        }
    }
}
