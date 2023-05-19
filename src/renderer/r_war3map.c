#include "r_war3map.h"

LPMAPSEGMENT g_mapSegments = NULL;

static void R_FileReadShadowMap(HANDLE hMpq, LPWAR3MAP  pWorld) {
    HANDLE hFile;
    SFileOpenFileEx(hMpq, "war3map.shd", SFILE_OPEN_FROM_MPQ, &hFile);
    int const w = (pWorld->width - 1) * 4;
    int const h = (pWorld->height - 1) * 4;
    LPSTR lpShadows = ri.MemAlloc(w * h);
    SFileReadFile(hFile, lpShadows, w * h, NULL, NULL);
    LPTEXTURE pShadowmap = R_AllocateTexture(w, h);
    LPCOLOR32 lpPixels = ri.MemAlloc(w * h * sizeof(struct color32));
    FOR_LOOP(i, w * h) {
        lpPixels[i].r = 255-lpShadows[i];
        lpPixels[i].g = 255-lpShadows[i];
        lpPixels[i].b = 255-lpShadows[i];
        lpPixels[i].a = 255;
    }
    R_LoadTextureMipLevel(pShadowmap, 0, lpPixels, w, h);
    SFileCloseFile(hFile);
    
    tr.shadowmap = pShadowmap;
}

static LPMAPSEGMENT R_BuildMapSegment(LPCWAR3MAP lpMap, DWORD sx, DWORD sy) {
    LPMAPSEGMENT lpMapSegment = ri.MemAlloc(sizeof(MAPSEGMENT));
    LPMAPLAYER lpMapLayer = R_BuildMapSegmentWater(lpMap, sx, sy);
    lpMapLayer->lpNext = lpMapSegment->lpLayers;
    lpMapSegment->lpLayers = lpMapLayer;
    FOR_LOOP(dwCliff, lpMap->numCliffs) {
        if ((lpMapLayer = R_BuildMapSegmentCliffs(lpMap, sx, sy, dwCliff))) {
            lpMapLayer->lpNext = lpMapSegment->lpLayers;
            lpMapSegment->lpLayers = lpMapLayer;
        }
    }
    for (DWORD dwLayer = lpMap->numGrounds; dwLayer > 0; dwLayer--) {
        if ((lpMapLayer = R_BuildMapSegmentLayer(lpMap, sx, sy, dwLayer - 1))) {
            lpMapLayer->lpNext = lpMapSegment->lpLayers;
            lpMapSegment->lpLayers = lpMapLayer;
        }
    }
    return lpMapSegment;
}

static VECTOR3 R_GetMapVertexPoint(LPCWAR3MAP lpMap, DWORD x, DWORD y) {
    LPCWAR3MAPVERTEX lpMapVertex = GetWar3MapVertex(lpMap, x, y);
    return (VECTOR3) {
        .x = lpMap->center.x + x * TILESIZE,
        .y = lpMap->center.y + y * TILESIZE,
        .z = GetWar3MapVertexHeight(lpMapVertex),
    };
}

static void R_LoadMapSegments(LPCWAR3MAP lpMap) {
    FOR_LOOP(x, (lpMap->width - 1) / SEGMENT_SIZE) {
        FOR_LOOP(y, (lpMap->height - 1) / SEGMENT_SIZE) {
            LPMAPSEGMENT lpSegment = R_BuildMapSegment(lpMap, x, y);
            lpSegment->lpNext = g_mapSegments;
            
            lpSegment->lpCorners[CORNER_TOP_LEFT] =
            R_GetMapVertexPoint(lpMap, x * SEGMENT_SIZE, y * SEGMENT_SIZE);
            
            lpSegment->lpCorners[CORNER_TOP_RIGHT] =
            R_GetMapVertexPoint(lpMap, (x+1) * SEGMENT_SIZE, y * SEGMENT_SIZE);
            
            lpSegment->lpCorners[CORNER_BOTTOM_RIGHT] =
            R_GetMapVertexPoint(lpMap, (x+1) * SEGMENT_SIZE, (y+1) * SEGMENT_SIZE);
            
            lpSegment->lpCorners[CORNER_BOTTOM_LEFT] =
            R_GetMapVertexPoint(lpMap, x * SEGMENT_SIZE, (y+1) * SEGMENT_SIZE);
            
            g_mapSegments = lpSegment;
        }
    }
}

LPWAR3MAP FileReadWar3Map(HANDLE hArchive) {
    LPWAR3MAP lpWar3Map = ri.MemAlloc(sizeof(WAR3MAP));
    HANDLE hFile;
    SFileOpenFileEx(hArchive, "war3map.w3e", SFILE_OPEN_FROM_MPQ, &hFile);
    SFileReadFile(hFile, &lpWar3Map->header, 4, NULL, NULL);
    SFileReadFile(hFile, &lpWar3Map->version, 4, NULL, NULL);
    SFileReadFile(hFile, &lpWar3Map->tileset, 1, NULL, NULL);
    SFileReadFile(hFile, &lpWar3Map->custom, 4, NULL, NULL);
    SFileReadArray(hFile, lpWar3Map, Grounds, 4, ri.MemAlloc);
    SFileReadArray(hFile, lpWar3Map, Cliffs, 4, ri.MemAlloc);
    SFileReadFile(hFile, &lpWar3Map->width, 4, NULL, NULL);
    SFileReadFile(hFile, &lpWar3Map->height, 4, NULL, NULL);
    SFileReadFile(hFile, &lpWar3Map->center, 8, NULL, NULL);
    int const vertexblocksize = MAP_VERTEX_SIZE * lpWar3Map->width * lpWar3Map->height;
    lpWar3Map->vertices = ri.MemAlloc(vertexblocksize);
    SFileReadFile(hFile, lpWar3Map->vertices, vertexblocksize, 0, 0);
    SFileCloseFile(hFile);
    return lpWar3Map;
}

void R_RegisterMap(char const *szMapFilename) {
    HANDLE hMpq;
    LPWAR3MAP lpMap;
    ri.FileExtract(szMapFilename, TMP_MAP);
    SFileOpenArchive(TMP_MAP, 0, 0, &hMpq);
    lpMap = FileReadWar3Map(hMpq);
    R_FileReadShadowMap(hMpq, lpMap);
    SFileCloseArchive(hMpq);
    tr.world = lpMap;
    
    R_LoadMapSegments(lpMap);
}

VECTOR3 R_GetPointOnScreen(LPCVECTOR3 point) {
    VECTOR3 screen;
    Matrix4_multiply_vector3(&tr.viewDef.projection_matrix, point, &screen);
    return screen;
}

static bool R_IsSegmentVisible(LPCMAPSEGMENT lpSegment) {
    VECTOR3 corners[CORNER_COUNT];
    FOR_LOOP(corner, CORNER_COUNT) {
        corners[corner] = R_GetPointOnScreen(&lpSegment->lpCorners[corner]);
    }
    if (corners[CORNER_TOP_RIGHT].x < -1 && corners[CORNER_BOTTOM_RIGHT].x < -1) return false;
    if (corners[CORNER_TOP_LEFT].x > 1 && corners[CORNER_BOTTOM_LEFT].x > 1) return false;
    if (corners[CORNER_TOP_LEFT].y > 1.0 && corners[CORNER_TOP_RIGHT].y > 1.0) return false;
    if (corners[CORNER_BOTTOM_LEFT].y < -1.0 && corners[CORNER_BOTTOM_RIGHT].y < -1.0) return false;
    return true;
}

static void R_DrawSegment(LPCMAPSEGMENT lpSegment, DWORD dwMask) {
    if (!R_IsSegmentVisible(lpSegment))
        return;
    FOR_EACH_LIST(MAPLAYER, lpLayer, lpSegment->lpLayers) {
        if (((1 << lpLayer->dwType) & dwMask) == 0)
            continue;
        if (lpLayer == lpSegment->lpLayers) {
            glDisable(GL_BLEND);
        } else {
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        }
        R_BindTexture(lpLayer->lpTexture, 0);
        R_DrawBuffer(lpLayer->lpBuffer, lpLayer->numVertices);
    }
}

void R_DrawWorld(void) {
    glUseProgram(tr.shaderStatic->progid);
    
    FOR_EACH_LIST(MAPSEGMENT, lpSegment, g_mapSegments) {
        R_DrawSegment(lpSegment, (1 << MAPLAYERTYPE_GROUND) | (1 << MAPLAYERTYPE_CLIFF));
    }
}

void R_DrawAlphaSurfaces(void) {
    glUseProgram(tr.shaderStatic->progid);
    
    FOR_EACH_LIST(MAPSEGMENT, lpSegment, g_mapSegments) {
        R_DrawSegment(lpSegment, (1 << MAPLAYERTYPE_WATER));
    }
}
