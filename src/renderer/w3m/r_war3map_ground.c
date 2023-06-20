#include "r_war3map.h"

#define MAX_MAP_LAYERS 16
#define WATER(INDEX) \
MakeColor(color[INDEX], LerpNumber(color[INDEX], 1, 0.25f), LerpNumber(color[INDEX], 1, 0.5f), 1)

LPCTEXTURE g_groundTextures[MAX_MAP_LAYERS] = { NULL };

static VERTEX aVertexBuffer[(SEGMENT_SIZE+1)*(SEGMENT_SIZE+1)*6];
static LPVERTEX currentVertex = NULL;

static void R_MakeTile(LPCWAR3MAP map, DWORD x, DWORD y, DWORD ground, LPCTEXTURE texture) {
    struct War3MapVertex tile[4];
    GetTileVertices(x, y, map, tile);
    int _tile = GetTile(tile, ground);

    if (_tile == 0)
        return;

    if (!GetTileRamps(tile) && IsTileCliff(tile)) {
        return;
    }

    VECTOR2 p[] = {
        { map->center.x + x * TILESIZE, map->center.y + y * TILESIZE },
        { map->center.x + (x + 1) * TILESIZE, map->center.y + y * TILESIZE },
        { map->center.x + (x + 1) * TILESIZE, map->center.y + (y + 1) * TILESIZE },
        { map->center.x + x * TILESIZE, map->center.y + (y + 1) * TILESIZE },
    };

    float const waterlevel[] = {
        GetWar3MapVertexWaterLevel(&tile[3]),
        GetWar3MapVertexWaterLevel(&tile[2]),
        GetWar3MapVertexWaterLevel(&tile[0]),
        GetWar3MapVertexWaterLevel(&tile[1]),
    };

    float const h[] = {
        GetWar3MapVertexHeight(&tile[3]),
        GetWar3MapVertexHeight(&tile[2]),
        GetWar3MapVertexHeight(&tile[0]),
        GetWar3MapVertexHeight(&tile[1]),
    };

    float const color[] = {
        GetTileDepth(waterlevel[0], h[0]),
        GetTileDepth(waterlevel[1], h[1]),
        GetTileDepth(waterlevel[2], h[2]),
        GetTileDepth(waterlevel[3], h[3]),
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
            .position = { p[0].x, p[0].y, h[0] },
            .texcoord = {0, 0},
            .texcoord2 = GetWar3MapPosition(map, p[0].x, p[0].y),
            .normal = {0,0,1},
            .color = WATER(0),
        },
        {
            .position = { p[1].x, p[1].y, h[1] },
            .texcoord = {1, 0},
            .texcoord2 = GetWar3MapPosition(map, p[1].x, p[1].y),
            .normal = {0,0,1},
            .color = WATER(1),
        },
        {
            .position = { p[2].x, p[2].y, h[2] },
            .texcoord = {1, 1},
            .texcoord2 = GetWar3MapPosition(map, p[2].x, p[2].y),
            .normal = {0,0,1},
            .color = WATER(2),
        },
        {
            .position = { p[0].x, p[0].y, h[0] },
            .texcoord = {0, 0},
            .texcoord2 = GetWar3MapPosition(map, p[0].x, p[0].y),
            .normal = {0,0,1},
            .color = WATER(0),
        },
        {
            .position = { p[2].x, p[2].y, h[2] },
            .texcoord = {1, 1},
            .texcoord2 = GetWar3MapPosition(map, p[2].x, p[2].y),
            .normal = {0,0,1},
            .color = WATER(2),
        },
        {
            .position = { p[3].x, p[3].y, h[3] },
            .texcoord = {0, 1},
            .texcoord2 = GetWar3MapPosition(map, p[3].x, p[3].y),
            .normal = {0,0,1},
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
        LPCSTR dir = ri.FindSheetCell(tr.terrainSheet, groundID, "dir");
        LPCSTR file = ri.FindSheetCell(tr.terrainSheet, groundID, "file");
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

void R_RenderSplat(LPCVECTOR2 position, float radius, LPCTEXTURE texture, COLOR32 color) {
    MATRIX4 mModelMatrix;

    Matrix4_identity(&mModelMatrix);
    
    //    R_Call(glUniform1i, tr.shaderSkin->uUseDiscard, 0);
    //    R_Call(glBlendFunc, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    //    R_Call(glDepthMask, GL_FALSE);
        
//    R_BindTexture(texture, 0);
//    R_RenderGeoset(geoset, &mModelMatrix);
    
    float splatSize = radius * 2;
    float sx = position->x;
    float sy = position->y;
    
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
    R_Call(glUseProgram, tr.shaderUI->progid);
    R_Call(glUniformMatrix4fv, tr.shaderUI->uProjectionMatrix, 1, GL_FALSE, tr.viewDef.projectionMatrix.v);
    R_Call(glBindVertexArray, tr.renbuf->vao);
    R_Call(glBindBuffer, GL_ARRAY_BUFFER, tr.renbuf->vbo);
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
