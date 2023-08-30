#include "r_war3map.h"

LPMAPSEGMENT g_mapSegments = NULL;

#ifdef DEBUG_PATHFINDING
LPCTEXTURE pathTexture = NULL;
void R_SetPathTexture(LPCCOLOR32 debugTexture) {
    if (pathTexture) {
        R_LoadTextureMipLevel(pathTexture, 0, debugTexture, pathTexture->width, pathTexture->height);
        R_Call(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        R_Call(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }
}
#endif

static void R_FileReadShadowMap(HANDLE hMpq, LPWAR3MAP  pWorld) {
    HANDLE file;
    SFileOpenFileEx(hMpq, "war3map.shd", SFILE_OPEN_FROM_MPQ, &file);
    int const w = (pWorld->width - 1) * 4;
    int const h = (pWorld->height - 1) * 4;
    LPSTR shadows = ri.MemAlloc(w * h);
    SFileReadFile(file, shadows, w * h, NULL, NULL);
    LPTEXTURE pShadowmap = R_AllocateTexture(w, h);
    LPCOLOR32 pixels = ri.MemAlloc(w * h * sizeof(struct color32));
    FOR_LOOP(i, w * h) {
        pixels[i].r = 255-shadows[i];
        pixels[i].g = 255-shadows[i];
        pixels[i].b = 255-shadows[i];
        pixels[i].a = 255;
    }
    R_LoadTextureMipLevel(pShadowmap, 0, pixels, w, h);
    SFileCloseFile(file);

    tr.texture[TEX_SHADOWMAP] = pShadowmap;
}

static LPMAPSEGMENT R_BuildMapSegment(LPCWAR3MAP map, DWORD sx, DWORD sy) {
    LPMAPSEGMENT mapSegment = ri.MemAlloc(sizeof(MAPSEGMENT));
    LPMAPLAYER mapLayer = R_BuildMapSegmentWater(map, sx, sy);
    ADD_TO_LIST(mapLayer, mapSegment->layers);
    FOR_LOOP(cliff, map->num_cliffs) {
        if ((mapLayer = R_BuildMapSegmentCliffs(map, sx, sy, cliff))) {
            ADD_TO_LIST(mapLayer, mapSegment->layers);
        }
    }
    for (DWORD layer = map->num_grounds; layer > 0; layer--) {
        if ((mapLayer = R_BuildMapSegmentLayer(map, sx, sy, layer - 1))) {
            ADD_TO_LIST(mapLayer, mapSegment->layers);
        }
    }
    mapSegment->bbox.min = MAKE(VECTOR3, FLT_MAX, FLT_MAX, FLT_MAX);
    mapSegment->bbox.max = MAKE(VECTOR3, -FLT_MAX, -FLT_MAX, -FLT_MAX);
    return mapSegment;
}

static VECTOR3 R_GetMapVertexPoint(LPCWAR3MAP map, DWORD x, DWORD y) {
    LPCWAR3MAPVERTEX mapVertex = GetWar3MapVertex(map, x, y);
    return (VECTOR3) {
        .x = map->center.x + x * TILESIZE,
        .y = map->center.y + y * TILESIZE,
        .z = GetWar3MapVertexHeight(mapVertex),
    };
}

static void R_LoadMapSegments(LPCWAR3MAP map) {
    FOR_LOOP(fx, (map->width - 1) / SEGMENT_SIZE) {
        FOR_LOOP(fy, (map->height - 1) / SEGMENT_SIZE) {
            LPMAPSEGMENT segment = R_BuildMapSegment(map, fx, fy);
            ADD_TO_LIST(segment, g_mapSegments);
            FOR_LOOP(sx, SEGMENT_SIZE+1) {
                FOR_LOOP(sy, SEGMENT_SIZE+1) {
                    FLOAT x = fx * SEGMENT_SIZE + sx;
                    FLOAT y = fy * SEGMENT_SIZE + sy;
                    VECTOR3 v = R_GetMapVertexPoint(map, x, y);
                    segment->bbox.min.x = MIN(segment->bbox.min.x, v.x);
                    segment->bbox.min.y = MIN(segment->bbox.min.y, v.y);
                    segment->bbox.min.z = MIN(segment->bbox.min.z, v.z);
                    segment->bbox.max.x = MAX(segment->bbox.max.x, v.x);
                    segment->bbox.max.y = MAX(segment->bbox.max.y, v.y);
                    segment->bbox.max.z = MAX(segment->bbox.max.z, v.z);
                }
            }
        }
    }
}

void R_AllocateFogOfWar(LPWAR3MAP map) {
    R_InitFogOfWar((map->width - 1) * 4, (map->height - 1) * 4);
}

LPWAR3MAP FileReadWar3Map(HANDLE archive) {
    LPWAR3MAP map = ri.MemAlloc(sizeof(WAR3MAP));
    HANDLE file;
    SFileOpenFileEx(archive, "war3map.w3e", SFILE_OPEN_FROM_MPQ, &file);
    SFileReadFile(file, &map->header, 4, NULL, NULL);
    SFileReadFile(file, &map->version, 4, NULL, NULL);
    SFileReadFile(file, &map->tileset, 1, NULL, NULL);
    SFileReadFile(file, &map->custom, 4, NULL, NULL);
    SFileReadArray(file, map, grounds, 4, ri.MemAlloc);
    SFileReadArray(file, map, cliffs, 4, ri.MemAlloc);
    SFileReadFile(file, &map->width, 4, NULL, NULL);
    SFileReadFile(file, &map->height, 4, NULL, NULL);
    SFileReadFile(file, &map->center, 8, NULL, NULL);
    int const vertexblocksize = MAP_VERTEX_SIZE * map->width * map->height;
    map->vertices = ri.MemAlloc(vertexblocksize);
    R_AllocateFogOfWar(map);
    SFileReadFile(file, map->vertices, vertexblocksize, 0, 0);
    SFileCloseFile(file);
#ifdef DEBUG_PATHFINDING
    pathTexture = R_AllocateTexture((map->width - 1) * 4, (map->height - 1) * 4);
    R_Call(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    R_Call(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
#endif
    FOR_LOOP(y, map->height) {
//        printf("%04x  ", y);
        FOR_LOOP(x, map->width) {
            LPWAR3MAPVERTEX vert = (LPWAR3MAPVERTEX)GetWar3MapVertex(map, x, y);
            if (!vert->ramp)
                continue;
            vert->cliffVariation = 0; // used also to mark mid-ramp
            LPCWAR3MAPVERTEX l = GetWar3MapVertex(map, x-1, y);
            LPCWAR3MAPVERTEX r = GetWar3MapVertex(map, x+1, y);
            LPCWAR3MAPVERTEX t = GetWar3MapVertex(map, x, y-1);
            LPCWAR3MAPVERTEX b = GetWar3MapVertex(map, x, y+1);
            if (l && r && l->ramp && r->ramp && l->level != r->level) {
                vert->cliffVariation = 1;
            } else if (t && b && t->ramp && b->ramp && t->level != b->level) {
                vert->cliffVariation = 1;
            }
//            printf("%x", GetWar3MapVertex(map, x, y)->level);
        }
//        printf("\n");
    }
    return map;
}

void R_RegisterMap(char const *mapFilename) {
    HANDLE hMpq;
    LPWAR3MAP map;
    ri.FileExtract(mapFilename, TMP_MAP);
    SFileOpenArchive(TMP_MAP, 0, 0, &hMpq);
    map = FileReadWar3Map(hMpq);
    R_FileReadShadowMap(hMpq, map);
    SFileCloseArchive(hMpq);
    tr.world = map;

    R_LoadMapSegments(map);
}

static void R_DrawSegment(LPCMAPSEGMENT segment, DWORD mask) {
    if (!Frustum_ContainsAABox(&tr.viewDef.frustum, &segment->bbox))
        return;
    FOR_EACH_LIST(MAPLAYER, layer, segment->layers) {
        if (((1 << layer->type) & mask) == 0)
            continue;
        if (layer == segment->layers) {
            R_Call(glDisable, GL_BLEND);
        } else {
            R_Call(glEnable, GL_BLEND);
            R_Call(glBlendFunc, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        }
        R_BindTexture(layer->texture, 0);
        R_DrawBuffer(layer->buffer, layer->num_vertices);
    }
}

void R_DrawWorld(void) {
    if (tr.viewDef.rdflags & RDF_NOWORLDMODEL)
        return;

    R_Call(glUseProgram, tr.shader[SHADER_DEFAULT]->progid);
    
#ifdef DEBUG_PATHFINDING
    R_BindTexture(pathTexture, 1);
#endif
    
    FOR_EACH_LIST(MAPSEGMENT, segment, g_mapSegments) {
        R_DrawSegment(segment, (1 << MAPLAYERTYPE_GROUND) | (1 << MAPLAYERTYPE_CLIFF));
    }
}

void R_DrawAlphaSurfaces(void) {
    if (tr.viewDef.rdflags & RDF_NOWORLDMODEL)
        return;
    
    R_Call(glUseProgram, tr.shader[SHADER_DEFAULT]->progid);
    R_Call(glEnable, GL_BLEND);
    R_Call(glBlendFunc, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    R_Call(glDepthMask, GL_FALSE);

    FOR_EACH_LIST(MAPSEGMENT, segment, g_mapSegments) {
        R_DrawSegment(segment, (1 << MAPLAYERTYPE_WATER));
    }
}
