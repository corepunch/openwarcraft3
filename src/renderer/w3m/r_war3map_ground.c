#include "r_war3map.h"

#define MAX_MAP_LAYERS 16
#define WATER(INDEX) \
MakeColor(color[INDEX], LerpNumber(color[INDEX], 1, 0.25f), LerpNumber(color[INDEX], 1, 0.5f), 1)

LPCTEXTURE g_groundTextures[MAX_MAP_LAYERS] = { NULL };

static VERTEX aVertexBuffer[(SEGMENT_SIZE+1)*(SEGMENT_SIZE+1)*6];
static LPVERTEX currentVertex = NULL;

VECTOR3 R_GetVertexPosition(LPCWAR3MAP map, DWORD x, DWORD y) {
    LPCWAR3MAPVERTEX vert = GetWar3MapVertex(map, x, y);
    float z = DECODE_HEIGHT(vert->accurate_height) + vert->level * TILESIZE - HEIGHT_COR;
    return (VECTOR3) {
        .x = map->center.x + x * TILESIZE,
        .y = map->center.y + y * TILESIZE,
        .z = z,
    };
}

VECTOR3 R_GetVertexNormal(LPCWAR3MAP map, DWORD x, DWORD y) {
    VECTOR3 const currentPoint = R_GetVertexPosition(map, x, y);
    VECTOR3 const leftPoint = (x > 0) ? R_GetVertexPosition(map, x - 1, y) : currentPoint;
    VECTOR3 const rightPoint = (x < map->width - 1) ? R_GetVertexPosition(map, x + 1, y) : currentPoint;
    VECTOR3 const topPoint = (y > 0) ? R_GetVertexPosition(map, x, y - 1) : currentPoint;
    VECTOR3 const bottomPoint = (y < map->height - 1) ? R_GetVertexPosition(map, x, y + 1) : currentPoint;
    VECTOR3 const diffX = Vector3_sub(&rightPoint, &leftPoint);
    VECTOR3 const diffY = Vector3_sub(&bottomPoint, &topPoint);
    VECTOR3 normal = Vector3_cross(&diffX, &diffY);
    Vector3_normalize(&normal);
    return normal;
}

static void R_MakeTile(LPCWAR3MAP map, DWORD x, DWORD y, DWORD ground, LPCTEXTURE texture) {
    struct War3MapVertex tile[4];
    GetTileVertices(x, y, map, tile);
    int _tile = GetTile(tile, ground);

    if (_tile == 0)
        return;

    if (!GetTileRamps(tile) && IsTileCliff(tile)) {
        return;
    }

    VECTOR3 p[] = {
        R_GetVertexPosition(map, x, y),
        R_GetVertexPosition(map, x + 1, y),
        R_GetVertexPosition(map, x + 1, y + 1),
        R_GetVertexPosition(map, x, y + 1),
    };

    VECTOR3 n[] = {
        R_GetVertexNormal(map, x, y),
        R_GetVertexNormal(map, x + 1, y),
        R_GetVertexNormal(map, x + 1, y + 1),
        R_GetVertexNormal(map, x, y + 1),
    };

    float const waterlevel[] = {
        GetWar3MapVertexWaterLevel(&tile[3]),
        GetWar3MapVertexWaterLevel(&tile[2]),
        GetWar3MapVertexWaterLevel(&tile[0]),
        GetWar3MapVertexWaterLevel(&tile[1]),
    };

    float const color[] = {
        GetTileDepth(waterlevel[0], p[0].z),
        GetTileDepth(waterlevel[1], p[1].z),
        GetTileDepth(waterlevel[2], p[2].z),
        GetTileDepth(waterlevel[3], p[3].z),
    };

//    VECTOR2 const tilecenter = {
//        map->center.x + (x + 0.5) * TILESIZE,
//        map->center.y + (y + 0.5) * TILESIZE,
//    };
//
//    p[0] = Vector2_lerp(&p[0], &tilecenter, 0.025);
//    p[1] = Vector2_lerp(&p[1], &tilecenter, 0.025);
//    p[2] = Vector2_lerp(&p[2], &tilecenter, 0.025);
//    p[3] = Vector2_lerp(&p[3], &tilecenter, 0.025);

    struct vertex geom[] = {
        {
            .position = p[0],
            .texcoord = {0, 0},
            .normal = n[0],
            .color = WATER(0),
        },
        {
            .position = p[1],
            .texcoord = {1, 0},
            .normal = n[1],
            .color = WATER(1),
        },
        {
            .position = p[2],
            .texcoord = {1, 1},
            .normal = n[2],
            .color = WATER(2),
        },
        {
            .position = p[0],
            .texcoord = {0, 0},
            .normal = n[0],
            .color = WATER(0),
        },
        {
            .position = p[2],
            .texcoord = {1, 1},
            .normal = n[2],
            .color = WATER(2),
        },
        {
            .position = p[3],
            .texcoord = {0, 1},
            .normal = n[3],
            .color = WATER(3),
        },
    };
    
    if (texture) {
        SetTileUV(_tile, geom, texture);
    }

    memcpy(currentVertex, geom, sizeof(geom));
    currentVertex += sizeof(geom) / sizeof(VERTEX);
}

LPMAPLAYER R_BuildMapSegmentLayer(LPCWAR3MAP map, DWORD sx, DWORD sy, DWORD layer) {
    LPMAPLAYER mapLayer = ri.MemAlloc(sizeof(MAPLAYER));
    PATHSTR zBuffer;
    if (g_groundTextures[layer] == NULL) {
        char groundID[5] = { 0 };
        memcpy(groundID, &map->grounds[layer], 4);
        LPCSTR dir = ri.FindSheetCell(tr.sheet[SHEET_TERRAIN], groundID, "dir");
        LPCSTR file = ri.FindSheetCell(tr.sheet[SHEET_TERRAIN], groundID, "file");
        if (file && dir) {
            sprintf(zBuffer, "%s\\%s.blp", dir, file);
            g_groundTextures[layer] = R_LoadTexture(zBuffer);
        } else {
            return NULL;
        }
    }
    mapLayer->texture = g_groundTextures[layer];
    mapLayer->type = MAPLAYERTYPE_GROUND;
    currentVertex = aVertexBuffer;
    for (DWORD x = sx * SEGMENT_SIZE; x < (sx + 1) * SEGMENT_SIZE; x++) {
        for (DWORD y = sy * SEGMENT_SIZE; y < (sy + 1) * SEGMENT_SIZE; y++) {
            R_MakeTile(map, x, y, layer, mapLayer->texture);
        }
    }
    mapLayer->num_vertices = (DWORD)(currentVertex - aVertexBuffer);
    mapLayer->buffer = R_MakeVertexArrayObject(aVertexBuffer, mapLayer->num_vertices);
    return mapLayer;
}

void R_RenderSplat(LPCVECTOR2 position,
                   float radius,
                   LPCTEXTURE texture,
                   LPCSHADER shader,
                   COLOR32 color)
{
    MATRIX4 mModelMatrix;

    Matrix4_identity(&mModelMatrix);
    
    float const splatSize = radius * 2;
    float const sx = position->x;
    float const sy = position->y;
    
    VECTOR2 tmin = GetWar3MapPosition(tr.world, sx - radius, sy - radius);
    VECTOR2 tmax = GetWar3MapPosition(tr.world, sx + radius, sy + radius);

    currentVertex = aVertexBuffer;

    for (DWORD x = MAX(0, floor(tmin.x*tr.world->width)-1);
         x < MIN(tr.world->width, ceil(tmax.x*tr.world->width));
         x++)
    {
        for (DWORD y = MAX(0, floor(tmin.y*tr.world->height)-1);
             y < MIN(tr.world->height, ceil(tmax.y*tr.world->height));
             y++)
        {
            R_MakeTile(tr.world, x, y, 0, NULL);
        }
    }
    int num_vertices = (DWORD)(currentVertex - aVertexBuffer);
    
    FOR_LOOP(i, num_vertices){
        LPVERTEX v = &aVertexBuffer[i];
        v->texcoord.x = (v->position.x + radius - sx) / splatSize;
        v->texcoord.y = 1 - (v->position.y + radius - sy) / splatSize;
        v->color = color;
    }

    R_BindTexture(texture, 0);
    R_Call(glUseProgram, shader->progid);
    R_Call(glUniformMatrix4fv, shader->uProjectionMatrix, 1, GL_FALSE, tr.viewDef.projectionMatrix.v);
    R_Call(glBindVertexArray, tr.buffer[RBUF_TEMP1]->vao);
    R_Call(glBindBuffer, GL_ARRAY_BUFFER, tr.buffer[RBUF_TEMP1]->vbo);
    R_Call(glBufferData, GL_ARRAY_BUFFER, sizeof(VERTEX) * num_vertices, aVertexBuffer, GL_STATIC_DRAW);
    R_Call(glDrawArrays, GL_TRIANGLES, 0, num_vertices);
}

VECTOR3 CM_PointIntoHeightmap(LPCVECTOR3 point) {
    return (VECTOR3) {
        .x = (point->x - tr.world->center.x) / TILESIZE,
        .y = (point->y - tr.world->center.y) / TILESIZE,
        .z = point->z
    };
}

short R_GetHeightMapValue(int x, int y) {
    return GetWar3MapVertexHeight(GetWar3MapVertex(tr.world, x, y));
}

VECTOR3 R_PointFromHeightmap(LPCVECTOR3 point) {
    return (VECTOR3) {
        .x = point->x * TILESIZE + tr.world->center.x,
        .y = point->y * TILESIZE + tr.world->center.y,
        .z = point->z
    };
}

bool R_TraceLocation(viewDef_t const *viewdef, float x, float y, LPVECTOR3 output) {
    LINE3 const gline = R_LineForScreenPoint(viewdef, x, y);
    LINE3 line = {
        .a = CM_PointIntoHeightmap(&gline.a),
        .b = CM_PointIntoHeightmap(&gline.b),
    };
    FOR_LOOP(x, tr.world->width) {
        FOR_LOOP(y, tr.world->height) {
            TRIANGLE3 const tri1 = {
                { x, y, R_GetHeightMapValue(x, y) },
                { x+1, y, R_GetHeightMapValue(x+1, y) },
                { x+1, y+1, R_GetHeightMapValue(x+1, y+1) },
            };
            TRIANGLE3 const tri2 = {
                { x+1, y+1, R_GetHeightMapValue(x+1, y+1) },
                { x, y+1, R_GetHeightMapValue(x, y+1) },
                { x, y, R_GetHeightMapValue(x, y) },
            };
            if (Line3_intersect_triangle(&line, &tri1, output)) {
                *output = R_PointFromHeightmap(output);
                return true;
            }
            if (Line3_intersect_triangle(&line, &tri2, output)) {
                *output = R_PointFromHeightmap(output);
                return true;
            }
        }
    }
    return false;
}
