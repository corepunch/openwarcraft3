#ifndef R_TERRAIN_LAYERS_H
#define R_TERRAIN_LAYERS_H

#include "renderer/r_local.h"

KNOWN_AS(MapLayer, MAPLAYER);
KNOWN_AS(MapSegment, MAPSEGMENT);

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
    BOX3 bbox;
};

void R_DrawTerrainSegment(LPCMAPSEGMENT segment, DWORD mask);

#endif
