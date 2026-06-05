#include "r_sc2map.h"

#include "games/starcraft-2/common/sc2_map.c"

static VERTEX *sc2_vertices;
static DWORD   sc2_num_vertices;

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

static COLOR32 r_sc2_tile_color(DWORD x, DWORD y) {
    BYTE checker = (BYTE)(((x / 4 + y / 4) & 1) ? 18 : 0);
    return (COLOR32){ (BYTE)(72 + checker), (BYTE)(95 + checker), (BYTE)(70 + checker), 255 };
}

static void r_sc2_push_vertex(VERTEX *v, FLOAT x, FLOAT y, FLOAT z, FLOAT u, FLOAT t, COLOR32 color) {
    v->position = (VECTOR3){ x, y, z };
    v->texcoord = (VECTOR2){ u, t };
    v->normal = (VECTOR3){ 0.0f, 0.0f, 1.0f };
    v->color = color;
}

static void r_sc2_build_terrain(sc2Map_t const *map) {
    BOX2 bounds;
    DWORD w, h, num_vertices;
    VERTEX *out;

    SAFE_DELETE(sc2_vertices, ri.MemFree);
    sc2_num_vertices = 0;
    if (!map || !map->width || !map->height)
        return;

    w = MIN(map->width, 192);
    h = MIN(map->height, 192);
    num_vertices = w * h * 6;
    sc2_vertices = ri.MemAlloc(num_vertices * sizeof(*sc2_vertices));
    out = sc2_vertices;
    bounds = SC2_MapBounds();

    FOR_LOOP(y, h) {
        FOR_LOOP(x, w) {
            FLOAT x0 = bounds.min.x + x * map->cell_size;
            FLOAT y0 = bounds.min.y + y * map->cell_size;
            FLOAT x1 = x0 + map->cell_size;
            FLOAT y1 = y0 + map->cell_size;
            COLOR32 color = r_sc2_tile_color(x, y);
            r_sc2_push_vertex(out++, x0, y0, 0.0f, 0.0f, 0.0f, color);
            r_sc2_push_vertex(out++, x1, y0, 0.0f, 1.0f, 0.0f, color);
            r_sc2_push_vertex(out++, x1, y1, 0.0f, 1.0f, 1.0f, color);
            r_sc2_push_vertex(out++, x0, y0, 0.0f, 0.0f, 0.0f, color);
            r_sc2_push_vertex(out++, x1, y1, 0.0f, 1.0f, 1.0f, color);
            r_sc2_push_vertex(out++, x0, y1, 0.0f, 0.0f, 1.0f, color);
        }
    }
    sc2_num_vertices = num_vertices;
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
    if (!sc2_vertices || !sc2_num_vertices || (tr.viewDef.rdflags & RDF_NOWORLDMODEL))
        return;

    Matrix4_identity(&model_matrix);
    R_Call(glUseProgram, tr.shader[SHADER_DEFAULT]->progid);
    R_Call(glUniformMatrix4fv, tr.shader[SHADER_DEFAULT]->uViewProjectionMatrix, 1, GL_FALSE, tr.viewDef.viewProjectionMatrix.v);
    R_Call(glUniformMatrix4fv, tr.shader[SHADER_DEFAULT]->uModelMatrix, 1, GL_FALSE, model_matrix.v);
    R_Call(glEnable, GL_DEPTH_TEST);
    R_Call(glDepthMask, GL_TRUE);
    R_Call(glDisable, GL_BLEND);
    R_BindTexture(tr.texture[TEX_WHITE], 0);
    R_Call(glBindVertexArray, tr.buffer[RBUF_TEMP1]->vao);
    R_Call(glBindBuffer, GL_ARRAY_BUFFER, tr.buffer[RBUF_TEMP1]->vbo);
    R_Call(glBufferData, GL_ARRAY_BUFFER, sc2_num_vertices * sizeof(*sc2_vertices), sc2_vertices, GL_STATIC_DRAW);
    R_Call(glDrawArrays, GL_TRIANGLES, 0, sc2_num_vertices);
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
