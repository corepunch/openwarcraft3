#include "r_war3map.h"

LPMAPSEGMENT g_mapSegments = NULL;

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

    tr.shadowmap = pShadowmap;
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
    FOR_LOOP(x, (map->width - 1) / SEGMENT_SIZE) {
        FOR_LOOP(y, (map->height - 1) / SEGMENT_SIZE) {
            LPMAPSEGMENT segment = R_BuildMapSegment(map, x, y);

            segment->corners[CORNER_TOP_LEFT] =
            R_GetMapVertexPoint(map, x * SEGMENT_SIZE, y * SEGMENT_SIZE);

            segment->corners[CORNER_TOP_RIGHT] =
            R_GetMapVertexPoint(map, (x+1) * SEGMENT_SIZE, y * SEGMENT_SIZE);

            segment->corners[CORNER_BOTTOM_RIGHT] =
            R_GetMapVertexPoint(map, (x+1) * SEGMENT_SIZE, (y+1) * SEGMENT_SIZE);

            segment->corners[CORNER_BOTTOM_LEFT] =
            R_GetMapVertexPoint(map, x * SEGMENT_SIZE, (y+1) * SEGMENT_SIZE);

            ADD_TO_LIST(segment, g_mapSegments);
        }
    }
}

LPWAR3MAP FileReadWar3Map(HANDLE archive) {
    LPWAR3MAP war3Map = ri.MemAlloc(sizeof(WAR3MAP));
    HANDLE file;
    SFileOpenFileEx(archive, "war3map.w3e", SFILE_OPEN_FROM_MPQ, &file);
    SFileReadFile(file, &war3Map->header, 4, NULL, NULL);
    SFileReadFile(file, &war3Map->version, 4, NULL, NULL);
    SFileReadFile(file, &war3Map->tileset, 1, NULL, NULL);
    SFileReadFile(file, &war3Map->custom, 4, NULL, NULL);
    SFileReadArray(file, war3Map, grounds, 4, ri.MemAlloc);
    SFileReadArray(file, war3Map, cliffs, 4, ri.MemAlloc);
    SFileReadFile(file, &war3Map->width, 4, NULL, NULL);
    SFileReadFile(file, &war3Map->height, 4, NULL, NULL);
    SFileReadFile(file, &war3Map->center, 8, NULL, NULL);
    int const vertexblocksize = MAP_VERTEX_SIZE * war3Map->width * war3Map->height;
    war3Map->vertices = ri.MemAlloc(vertexblocksize);
    SFileReadFile(file, war3Map->vertices, vertexblocksize, 0, 0);
    SFileCloseFile(file);
    return war3Map;
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

VECTOR3 R_GetPointOnScreen(LPCVECTOR3 point) {
    VECTOR3 screen;
    Matrix4_multiply_vector3(&tr.viewDef.projection_matrix, point, &screen);
    return screen;
}

static bool R_IsSegmentVisible(LPCMAPSEGMENT segment) {
    VECTOR3 corners[CORNER_COUNT];
    FOR_LOOP(corner, CORNER_COUNT) {
        corners[corner] = R_GetPointOnScreen(&segment->corners[corner]);
    }
    if (corners[CORNER_TOP_RIGHT].x < -1 && corners[CORNER_BOTTOM_RIGHT].x < -1) return false;
    if (corners[CORNER_TOP_LEFT].x > 1 && corners[CORNER_BOTTOM_LEFT].x > 1) return false;
    if (corners[CORNER_TOP_LEFT].y > 1.0 && corners[CORNER_TOP_RIGHT].y > 1.0) return false;
    if (corners[CORNER_BOTTOM_LEFT].y < -1.0 && corners[CORNER_BOTTOM_RIGHT].y < -1.0) return false;
    return true;
}

static void R_DrawSegment(LPCMAPSEGMENT segment, DWORD mask) {
    if (!R_IsSegmentVisible(segment))
        return;
    FOR_EACH_LIST(MAPLAYER, layer, segment->layers) {
        if (((1 << layer->type) & mask) == 0)
            continue;
        if (layer == segment->layers) {
            glDisable(GL_BLEND);
        } else {
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        }
        R_BindTexture(layer->texture, 0);
        R_DrawBuffer(layer->buffer, layer->num_vertices);
    }
}

void R_DrawWorld(void) {
    glUseProgram(tr.shaderStatic->progid);

    FOR_EACH_LIST(MAPSEGMENT, segment, g_mapSegments) {
        R_DrawSegment(segment, (1 << MAPLAYERTYPE_GROUND) | (1 << MAPLAYERTYPE_CLIFF));
    }
}

void R_DrawAlphaSurfaces(void) {
    glUseProgram(tr.shaderStatic->progid);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    FOR_EACH_LIST(MAPSEGMENT, segment, g_mapSegments) {
        R_DrawSegment(segment, (1 << MAPLAYERTYPE_WATER));
    }
}
