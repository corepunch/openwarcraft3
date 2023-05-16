#include "r_local.h"
#include "r_mdx.h"

#define LAYER_SIZE 500000

struct tVertex maplayer[LAYER_SIZE] = {};

struct tCliff {
    int cliffid;
    struct tModel const *model;
    struct tCliff *lpNext;
};

extern GLuint program;

struct tCliff *g_cliffs = NULL;

float GetWaterOpacity(float waterlevel, float height) {
    float const opacity = MIN(0.5, (waterlevel - height) / 50.0f);
    return MAX(0, opacity);
}

float GetTileDepth(float waterlevel, float height) {
    float const opacity = MIN(0.95, (waterlevel - height) / 250.0f);
    return 1 - MAX(0, opacity);
}

float lerp(float a, float b, float t) {
    return a * (1 - t) + b * t;
}

static struct tModel const *R_LoadCliffModel(struct CliffInfo const *cinfo, char const *ccfg, int ramp) {
    char buffer[256];
    const int cliffid = *(int *)ccfg;
    LPCSTR dir = ramp ? cinfo->rampModelDir : cinfo->cliffModelDir;
    for (struct tCliff *it = g_cliffs; it; it = it->lpNext) {
        if (it->cliffid == cliffid)
            return it->model;
    }
    struct tCliff *cliff = MemAlloc(sizeof(struct tCliff));
    cliff->cliffid = cliffid;
    sprintf(buffer, "Doodads\\Terrain\\%s\\%s%s0.mdx", dir, dir, ccfg);
    cliff->model = R_LoadModel(buffer);
    cliff->lpNext = g_cliffs;
    g_cliffs = cliff;
//    printf("%s\n", buffer);
    return cliff->model;
}

void SetTileUV(int tile, struct tVertex* vertices, struct TerrainInfo *terrain, struct Terrain const *heightmap) {
    const float u = 1.f/(terrain->lpTexture->width / 64);
    const float v = 1.f/(terrain->lpTexture->height / 64);

    vertices[0].texcoord.x = u * ((tile%4)+0);
    vertices[0].texcoord.y = v * ((tile/4)+1);
    vertices[1].texcoord.x = u * ((tile%4)+1);
    vertices[1].texcoord.y = v * ((tile/4)+1);
    vertices[2].texcoord.x = u * ((tile%4)+1);
    vertices[2].texcoord.y = v * ((tile/4)+0);
    vertices[3].texcoord.x = u * ((tile%4)+0);
    vertices[3].texcoord.y = v * ((tile/4)+1);
    vertices[4].texcoord.x = u * ((tile%4)+1);
    vertices[4].texcoord.y = v * ((tile/4)+0);
    vertices[5].texcoord.x = u * ((tile%4)+0);
    vertices[5].texcoord.y = v * ((tile/4)+0);
    
    FOR_LOOP(i, 6) {
        vertices[i].texcoord.x = lerp(vertices[i].texcoord.x, u * ((tile%4)+0.5), 0.05);
        vertices[i].texcoord.y = lerp(vertices[i].texcoord.y, v * ((tile/4)+0.5), 0.05);
    }
}

struct vector2 map_position(struct Terrain const *heightmap, float x, float y) {
    float _x = (x - heightmap->center.x) / ((heightmap->size.width-1) * TILESIZE);
    float _y = (y - heightmap->center.y) / ((heightmap->size.height-1) * TILESIZE);
    return (struct vector2) { _x, _y };
}

struct color32 MakeColor(float r, float g, float b, float a) {
    return (struct color32) {
        .r = r * 255,
        .g = g * 255,
        .b = b * 255,
        .a = a * 255,
    };
}

void MakeTile(int x, int y, int ground,
              struct tVertex* vertices,
              struct TerrainInfo *terrain,
              struct Terrain const *heightmap,
              int *index)
{
    struct TerrainVertex tile[4];
    GetTileVertices(x, y, heightmap, tile);
    int _tile = GetTile(tile, ground);
    
    if (_tile == 0)
        return;
    
    if (!GetTileRamps(tile) && IsTileCliff(tile)) {
        return;
    }
    
    struct vector2 p[] = {
        { heightmap->center.x + x * TILESIZE, heightmap->center.y + y * TILESIZE },
        { heightmap->center.x + (x + 1) * TILESIZE, heightmap->center.y + y * TILESIZE },
        { heightmap->center.x + (x + 1) * TILESIZE, heightmap->center.y + (y + 1) * TILESIZE },
        { heightmap->center.x + x * TILESIZE, heightmap->center.y + (y + 1) * TILESIZE },
    };
    
    float const waterlevel[] = {
        GetTerrainVertexWaterLevel(&tile[3]),
        GetTerrainVertexWaterLevel(&tile[2]),
        GetTerrainVertexWaterLevel(&tile[0]),
        GetTerrainVertexWaterLevel(&tile[1]),
    };
    
    float const h[] = {
        GetTerrainVertexHeight(&tile[3]),
        GetTerrainVertexHeight(&tile[2]),
        GetTerrainVertexHeight(&tile[0]),
        GetTerrainVertexHeight(&tile[1]),
    };
    
    float const color[] = {
        GetTileDepth(waterlevel[0], h[0]),
        GetTileDepth(waterlevel[1], h[1]),
        GetTileDepth(waterlevel[2], h[2]),
        GetTileDepth(waterlevel[3], h[3]),
    };
    
#define WATER(INDEX) \
MakeColor(color[INDEX], lerp(color[INDEX], 1, 0.25f), lerp(color[INDEX], 1, 0.5f), 1)

    struct tVertex geom[] = {
        { {p[0].x,p[0].y,h[0]}, {0, 0}, map_position(heightmap, p[0].x, p[0].y), WATER(0) },
        { {p[1].x,p[1].y,h[1]}, {1, 0}, map_position(heightmap, p[1].x, p[1].y), WATER(1) },
        { {p[2].x,p[2].y,h[2]}, {1, 1}, map_position(heightmap, p[2].x, p[2].y), WATER(2) },
        { {p[0].x,p[0].y,h[0]}, {0, 0}, map_position(heightmap, p[0].x, p[0].y), WATER(0) },
        { {p[2].x,p[2].y,h[2]}, {1, 1}, map_position(heightmap, p[2].x, p[2].y), WATER(2) },
        { {p[3].x,p[3].y,h[3]}, {0, 1}, map_position(heightmap, p[3].x, p[3].y), WATER(3) },
    };

    SetTileUV(_tile, geom, terrain, heightmap);
    
    memcpy(&vertices[*index], geom, sizeof(geom));
    *index += 6;
}

int TileBaseLevel(struct TerrainVertex const *tile) {
    int minlevel = tile->level;
    FOR_LOOP(index, 4) {
        minlevel = MIN(minlevel, tile[index].level);
    }
    return minlevel;
}

float GetAccurateHeightAtPoint(struct Terrain const *heightmap, float sx, float sy) {
    float x = sx / TILESIZE;
    float y = sy / TILESIZE;
    float fx = floorf(x);
    float fy = floorf(y);
    float a = GetTerrainVertex(heightmap, fx, fy)->accurate_height;
    float b = GetTerrainVertex(heightmap, fx + 1, fy)->accurate_height;
    float c = GetTerrainVertex(heightmap, fx, fy + 1)->accurate_height;
    float d = GetTerrainVertex(heightmap, fx + 1, fy + 1)->accurate_height;
    float ab = lerp(a, b, x - fx);
    float cd = lerp(c, d, x - fx);
    return DECODE_HEIGHT(lerp(ab, cd, y - fy));
}

float GetAccurateWaterLevelAtPoint(struct Terrain const *heightmap, float sx, float sy) {
    float x = sx / TILESIZE;
    float y = sy / TILESIZE;
    float fx = floorf(x);
    float fy = floorf(y);
    float a = GetTerrainVertex(heightmap, fx, fy)->waterlevel;
    float b = GetTerrainVertex(heightmap, fx + 1, fy)->waterlevel;
    float c = GetTerrainVertex(heightmap, fx, fy + 1)->waterlevel;
    float d = GetTerrainVertex(heightmap, fx + 1, fy + 1)->waterlevel;
    float ab = lerp(a, b, x - fx);
    float cd = lerp(c, d, x - fx);
    return DECODE_HEIGHT(lerp(ab, cd, y - fy));
}


void MakeCliff(int x, int y, int cliffindex,
              struct tVertex* vertices,
              struct CliffInfo *cliff,
              struct Terrain const *heightmap,
              int *index)
{
    struct TerrainVertex tile[4];
    GetTileVertices(x, y, heightmap, tile);
    int remap[4] = { 3, 1, 0, 2 };
    
    if (GetTileRamps(tile) || !IsTileCliff(tile) || tile[remap[0]].cliff != cliffindex)
        return;
    
    char cliffcfg[5] = { 0 };
    int const tileramps = GetTileRamps(tile);
    int const baselevel = TileBaseLevel(tile);
    
    FOR_LOOP(index, 4) {
        struct TerrainVertex const *vert = &tile[remap[index]];
        int const diff = vert->level - baselevel;
        if (diff == 0) {
            cliffcfg[index] = (tileramps > 1 && vert->ramp) ? 'L' : 'A';
        } else if (diff == 1) {
            cliffcfg[index] = (tileramps > 1 && vert->ramp) ? 'H' : 'B';
        } else {
            cliffcfg[index] = (tileramps > 1 && vert->ramp) ? 'X' : 'C';
        }
    }
    
    FOR_LOOP(gindx, heightmap->numGrounds) {
        if (heightmap->lpGrounds[gindx] == cliff->groundTile) {
            ((struct TerrainVertex *)GetTerrainVertex(heightmap, x+1, y+1))->ground = gindx;
            ((struct TerrainVertex *)GetTerrainVertex(heightmap, x, y+1))->ground = gindx;
            ((struct TerrainVertex *)GetTerrainVertex(heightmap, x+1, y))->ground = gindx;
            ((struct TerrainVertex *)GetTerrainVertex(heightmap, x, y))->ground = gindx;
            break;
        }
    }
    
    struct tModel const *pModel = R_LoadCliffModel(cliff, cliffcfg, tileramps > 1);

    assert(pModel && pModel->lpGeosets);

    struct tModelGeoset *pGeoset = pModel->lpGeosets;
    
    FOR_LOOP(t, pGeoset->numTriangles) {
        const int i = pGeoset->lpTriangles[t];
        const float fx = pGeoset->lpVertices[i].x + (x+1) * TILESIZE;
        const float fy = pGeoset->lpVertices[i].y + y * TILESIZE;
        const float fh = GetAccurateHeightAtPoint(heightmap, fx, fy);
        const float fw = GetAccurateWaterLevelAtPoint(heightmap, fx, fy);
        const float fz = pGeoset->lpVertices[i].z + baselevel * TILESIZE + fh - HEIGHT_COR;
        const float dp = GetTileDepth(fw, fz);
        maplayer[*index + t].color = MakeColor(dp, lerp(dp, 1, 0.25), lerp(dp, 1, 0.5), 1);
        maplayer[*index + t].position.x = heightmap->center.x + fx;
        maplayer[*index + t].position.y = heightmap->center.y + fy;
        maplayer[*index + t].position.z = fz;
        maplayer[*index + t].texcoord.x = pGeoset->lpTexcoord[i].x;
        maplayer[*index + t].texcoord.y = pGeoset->lpTexcoord[i].y;
        maplayer[*index + t].texcoord2 = map_position(heightmap, heightmap->center.x + fx, heightmap->center.y + fy);
    }
    
    *index += pGeoset->numTriangles;
}


int IsTileVisible(int x, int y) {
    return true;
}

struct CliffInfo *BindCliffTexture(struct Terrain const *heightmap, int cliffindex) {
    char buffer[256];
    int const cliffID = heightmap->lpCliffs[cliffindex];
    struct CliffInfo *cliff = FindCliffInfo(cliffID);
    if (!cliff)
        return NULL;
    if (!cliff->texture) {
        sprintf(buffer, "%s\\%s.blp", cliff->texDir, cliff->texFile);
        cliff->texture = R_LoadTexture(buffer);
    }
    R_BindTexture(cliff->texture, 0);
    return cliff;
}

struct TerrainInfo *BindGroundTexture(struct Terrain const *heightmap, int ground) {
    char buffer[256];
    int const tileID = heightmap->lpGrounds[ground];
    struct TerrainInfo *terrain = FindTerrainInfo(tileID);
    if (!terrain)
        return NULL;
    if (!terrain->lpTexture) {
        sprintf(buffer, "%s\\%s.blp", terrain->sDirectory, terrain->sFilename);
        terrain->lpTexture = R_LoadTexture(buffer);
    }
    R_BindTexture(terrain->lpTexture, 0);
    return terrain;
}

void R_RenderMapLayer(struct Terrain const *heightmap, int ground) {
    int index = 0;
    struct TerrainInfo *terrain = BindGroundTexture(heightmap, ground);
    
    if (!terrain)
        return;
    
    FOR_LOOP(x, heightmap->size.width - 1) {
        FOR_LOOP(y, heightmap->size.height - 1) {
            if (!IsTileVisible(x, y))
                continue;
            MakeTile(x, y, ground, maplayer, terrain, heightmap, &index);
        }
    }

    struct matrix4 model_matrix;
    matrix4_identity(&model_matrix);
    
    glUniformMatrix4fv( glGetUniformLocation( program, "u_model_matrix" ), 1, GL_FALSE, model_matrix.v );
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBindVertexArray( tr.renbuf->vao );
    glBindBuffer( GL_ARRAY_BUFFER, tr.renbuf->vbo );
    glBufferData( GL_ARRAY_BUFFER, sizeof(maplayer), maplayer, GL_STATIC_DRAW );
    glDrawArrays( GL_TRIANGLES, 0, index );
}

void MakeWaterTile(int x, int y,
                   struct tVertex* vertices,
                   struct Terrain const *heightmap,
                   int *index)
{
    struct TerrainVertex tile[4];
    GetTileVertices(x, y, heightmap, tile);
        
    if (!IsTileWater(tile))
        return;
    
    struct vector2 const pos[] = {
        { heightmap->center.x + x * TILESIZE, heightmap->center.y + y * TILESIZE },
        { heightmap->center.x + (x + 1) * TILESIZE, heightmap->center.y + y * TILESIZE },
        { heightmap->center.x + (x + 1) * TILESIZE, heightmap->center.y + (y + 1) * TILESIZE },
        { heightmap->center.x + x * TILESIZE, heightmap->center.y + (y + 1) * TILESIZE },
    };
    
    float const waterlevel[] = {
        GetTerrainVertexWaterLevel(&tile[3]),
        GetTerrainVertexWaterLevel(&tile[2]),
        GetTerrainVertexWaterLevel(&tile[0]),
        GetTerrainVertexWaterLevel(&tile[1]),
    };
    
    float const height[] = {
        GetTerrainVertexHeight(&tile[3]),
        GetTerrainVertexHeight(&tile[2]),
        GetTerrainVertexHeight(&tile[0]),
        GetTerrainVertexHeight(&tile[1]),
    };
    
    float const opac[] = {
        GetWaterOpacity(waterlevel[0], height[0]),
        GetWaterOpacity(waterlevel[1], height[1]),
        GetWaterOpacity(waterlevel[2], height[2]),
        GetWaterOpacity(waterlevel[3], height[3]),
    };

    struct tVertex geom[] = {
        { 1, 1, 1, opac[0], pos[0].x, pos[0].y, waterlevel[0], 0, 0 },
        { 1, 1, 1, opac[1], pos[1].x, pos[1].y, waterlevel[1], 1, 0 },
        { 1, 1, 1, opac[2], pos[2].x, pos[2].y, waterlevel[2], 1, 1 },
        { 1, 1, 1, opac[0], pos[0].x, pos[0].y, waterlevel[0], 0, 0 },
        { 1, 1, 1, opac[2], pos[2].x, pos[2].y, waterlevel[2], 1, 1 },
        { 1, 1, 1, opac[3], pos[3].x, pos[3].y, waterlevel[3], 0, 1 },
    };

    memcpy(&vertices[*index], geom, sizeof(geom));
    *index += 6;
}

void RenderWater(struct Terrain const *heightmap) {
    int index = 0;
    FOR_LOOP(x, heightmap->size.width - 1) {
        FOR_LOOP(y, heightmap->size.height - 1) {
            if (!IsTileVisible(x, y))
                continue;
            MakeWaterTile(x, y, maplayer, heightmap, &index);
        }
    }
    struct matrix4 model_matrix;
    matrix4_identity(&model_matrix);
    
    R_BindTexture(tr.waterTexture, 0);
    
    glUniformMatrix4fv( glGetUniformLocation( program, "u_model_matrix" ), 1, GL_FALSE, model_matrix.v );
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBindVertexArray( tr.renbuf->vao );
    glBindBuffer( GL_ARRAY_BUFFER, tr.renbuf->vbo );
    glBufferData( GL_ARRAY_BUFFER, sizeof(maplayer), maplayer, GL_STATIC_DRAW );
    glDrawArrays( GL_TRIANGLES, 0, index );
}

void R_RenderMapCliffs(struct Terrain const *heightmap, int cliffindex) {
    int index = 0;
    struct CliffInfo *cliff = BindCliffTexture(heightmap, cliffindex);

    if (!cliff)
        return;

    FOR_LOOP(x, heightmap->size.width - 1) {
        FOR_LOOP(y, heightmap->size.height - 1) {
            if (!IsTileVisible(x, y))
                continue;
            MakeCliff(x, y, cliffindex, maplayer, cliff, heightmap, &index);
        }
    }

    struct matrix4 model_matrix;
    matrix4_identity(&model_matrix);
    
    glUniformMatrix4fv( glGetUniformLocation( program, "u_model_matrix" ), 1, GL_FALSE, model_matrix.v );
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBindVertexArray( tr.renbuf->vao );
    glBindBuffer( GL_ARRAY_BUFFER, tr.renbuf->vbo );
    glBufferData( GL_ARRAY_BUFFER, sizeof(maplayer), maplayer, GL_STATIC_DRAW );
    glDrawArrays( GL_TRIANGLES, 0, index );
}

void R_DrawWorld(void) {
    R_BindTexture(tr.shadowmap, 1);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    FOR_LOOP(ground, tr.world->numGrounds) {
        R_RenderMapLayer(tr.world, ground);
    }
    FOR_LOOP(cliff, tr.world->numCliffs) {
        R_RenderMapCliffs(tr.world, cliff);
    }
}

void R_DrawAlphaSurfaces() {
    RenderWater(tr.world);
}

void R_DrawEntities() {
    FOR_LOOP(i, tr.refdef.num_entities) {
        struct render_entity const *ent = &tr.refdef.entities[i];
        RenderModel(ent);
    }
}


