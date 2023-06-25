#ifndef __r_war3map_h__
#define __r_war3map_h__

#include "r_local.h"

#define SEGMENT_SIZE 8

KNOWN_AS(MapLayer, MAPLAYER);
KNOWN_AS(MapSegment, MAPSEGMENT);

enum {
    CORNER_TOP_LEFT,
    CORNER_TOP_RIGHT,
    CORNER_BOTTOM_RIGHT,
    CORNER_BOTTOM_LEFT,
    CORNER_COUNT
};

typedef enum {
    MAPLAYERTYPE_GROUND,
    MAPLAYERTYPE_CLIFF,
    MAPLAYERTYPE_WATER,
} MAPLAYERTYPE;

struct MapLayer {
    MAPLAYERTYPE type;
    LPCBUFFER buffer;
    LPCTEXTURE texture;
    LPMAPLAYER next;
    DWORD num_vertices;
};

struct MapSegment {
    LPMAPLAYER layers;
    LPMAPSEGMENT next;
    VECTOR3 corners[CORNER_COUNT];
};

LPMAPLAYER R_BuildMapSegmentLayer(LPCWAR3MAP map, DWORD sx, DWORD sy, DWORD layer);
LPMAPLAYER R_BuildMapSegmentCliffs(LPCWAR3MAP map, DWORD sx, DWORD sy, DWORD cliff);
LPMAPLAYER R_BuildMapSegmentWater(LPCWAR3MAP map, DWORD sx, DWORD sy);

VECTOR2 GetWar3MapPosition(LPCWAR3MAP war3Map, float x, float y);
float GetTileDepth(float waterlevel, float height);
struct color32 MakeColor(float r, float g, float b, float a);
LPCWAR3MAPVERTEX GetWar3MapVertex(LPCWAR3MAP terrain, DWORD x, DWORD y);
DWORD GetTile(LPCWAR3MAPVERTEX mv, DWORD ground);
float GetWar3MapVertexHeight(LPCWAR3MAPVERTEX vert);
float GetWar3MapVertexWaterLevel(LPCWAR3MAPVERTEX vert);
void GetTileVertices(DWORD x, DWORD y, LPCWAR3MAP terrain, LPWAR3MAPVERTEX vertices);
void SetTileUV(DWORD tile, LPVERTEX vertices, LPCTEXTURE texture);
DWORD GetTileRamps(LPCWAR3MAPVERTEX vertices);
DWORD IsTileCliff(LPCWAR3MAPVERTEX vertices);
DWORD IsTileWater(LPCWAR3MAPVERTEX vertices);

VECTOR2 GetWar3MapSize(LPCWAR3MAP war3Map);

#endif
