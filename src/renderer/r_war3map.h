#ifndef __r_war3map_h__
#define __r_war3map_h__

#include "r_local.h"

#define SEGMENT_SIZE 8

ADD_TYPEDEFS(MapLayer, MAPLAYER);
ADD_TYPEDEFS(MapSegment, MAPSEGMENT);

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
    MAPLAYERTYPE dwType;
    LPCBUFFER lpBuffer;
    LPCTEXTURE lpTexture;
    LPMAPLAYER lpNext;
    DWORD numVertices;
};

struct MapSegment {
    LPMAPLAYER lpLayers;
    LPMAPSEGMENT lpNext;
    VECTOR3 lpCorners[CORNER_COUNT];
};

LPMAPLAYER R_BuildMapSegmentLayer(LPCWAR3MAP lpMap, DWORD sx, DWORD sy, DWORD dwLayer);
LPMAPLAYER R_BuildMapSegmentCliffs(LPCWAR3MAP lpMap, DWORD sx, DWORD sy, DWORD dwCliff);
LPMAPLAYER R_BuildMapSegmentWater(LPCWAR3MAP lpMap, DWORD sx, DWORD sy);

float LerpNumber(float a, float b, float t);
VECTOR2 GetWar3MapPosition(LPCWAR3MAP lpWar3Map, float x, float y);
float GetTileDepth(float waterlevel, float height);
struct color32 MakeColor(float r, float g, float b, float a);

#endif
