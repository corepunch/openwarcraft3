#include "r_war3map.h"

LPCWAR3MAPVERTEX GetWar3MapVertex(LPCWAR3MAP lpWar3Map, DWORD x, DWORD y) {
    int const index = x + y * lpWar3Map->width;
    char const *ptr = ((char const *)lpWar3Map->vertices) + index * MAP_VERTEX_SIZE;
    return (LPCWAR3MAPVERTEX)ptr;
}

VECTOR2 GetWar3MapPosition(LPCWAR3MAP lpWar3Map, float x, float y) {
    float _x = (x - lpWar3Map->center.x) / ((lpWar3Map->width-1) * TILESIZE);
    float _y = (y - lpWar3Map->center.y) / ((lpWar3Map->height-1) * TILESIZE);
    return (VECTOR2) { _x, _y };
}

float GetTileDepth(float waterlevel, float height) {
    float const opacity = MIN(0.95, (waterlevel - height) / 250.0f);
    return 1 - MAX(0, opacity);
}

struct color32 MakeColor(float r, float g, float b, float a) {
    return (struct color32) {
        .r = r * 255,
        .g = g * 255,
        .b = b * 255,
        .a = a * 255,
    };
}

void SetTileUV(DWORD dwTile, LPVERTEX lpVertices, LPCTEXTURE lpTexture) {
    const float u = 1.f/(lpTexture->width / 64);
    const float v = 1.f/(lpTexture->height / 64);

    lpVertices[0].texcoord.x = u * ((dwTile%4)+0);
    lpVertices[0].texcoord.y = v * ((dwTile/4)+1);
    lpVertices[1].texcoord.x = u * ((dwTile%4)+1);
    lpVertices[1].texcoord.y = v * ((dwTile/4)+1);
    lpVertices[2].texcoord.x = u * ((dwTile%4)+1);
    lpVertices[2].texcoord.y = v * ((dwTile/4)+0);
    lpVertices[3].texcoord.x = u * ((dwTile%4)+0);
    lpVertices[3].texcoord.y = v * ((dwTile/4)+1);
    lpVertices[4].texcoord.x = u * ((dwTile%4)+1);
    lpVertices[4].texcoord.y = v * ((dwTile/4)+0);
    lpVertices[5].texcoord.x = u * ((dwTile%4)+0);
    lpVertices[5].texcoord.y = v * ((dwTile/4)+0);
    
    FOR_LOOP(i, 6) {
        lpVertices[i].texcoord.x = LerpNumber(lpVertices[i].texcoord.x, u * ((dwTile%4)+0.5), 0.05);
        lpVertices[i].texcoord.y = LerpNumber(lpVertices[i].texcoord.y, v * ((dwTile/4)+0.5), 0.05);
    }
}

DWORD GetTile(LPCWAR3MAPVERTEX mv, DWORD ground) {
    if (ground == 0)
        return 15;
    return
        (mv[0].ground == ground ? 4 : 0) +
        (mv[1].ground == ground ? 8 : 0) +
        (mv[2].ground == ground ? 1 : 0) +
        (mv[3].ground == ground ? 2 : 0);
}

float GetWar3MapVertexHeight(LPCWAR3MAPVERTEX vert) {
    return DECODE_HEIGHT(vert->accurate_height) + vert->level * TILESIZE - HEIGHT_COR;
}

float GetWar3MapVertexWaterLevel(LPCWAR3MAPVERTEX vert) {
    return DECODE_HEIGHT(vert->waterlevel);
}

void GetTileVertices(DWORD x, DWORD y, LPCWAR3MAP lpWar3Map, LPWAR3MAPVERTEX vertices) {
    vertices[0] = *GetWar3MapVertex(lpWar3Map, x+1, y+1);
    vertices[1] = *GetWar3MapVertex(lpWar3Map, x, y+1);
    vertices[2] = *GetWar3MapVertex(lpWar3Map, x+1, y);
    vertices[3] = *GetWar3MapVertex(lpWar3Map, x, y);
}

DWORD GetTileRamps(LPCWAR3MAPVERTEX vertices) {
    return vertices[0].ramp + vertices[1].ramp + vertices[2].ramp + vertices[3].ramp;
}

DWORD IsTileCliff(LPCWAR3MAPVERTEX vertices) {
    int bIsCliff = 0;
    FOR_LOOP(index, 4) {
        bIsCliff |= vertices[index].level != vertices[0].level;
    }
    return bIsCliff;
}

DWORD IsTileWater(LPCWAR3MAPVERTEX vertices) {
    int bIsWater = 0;
    FOR_LOOP(index, 4) {
        bIsWater |= vertices[index].water;
    }
    FOR_LOOP(index, 4) {
        bIsWater &= (vertices[index].waterlevel & 0x4000) == 0;
    }
    return bIsWater;
}

