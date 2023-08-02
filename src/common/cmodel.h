#ifndef war3map_h
#define war3map_h

#include "common.h"

struct War3MapVertex {
    USHORT accurate_height;
    USHORT waterlevel:14;
    BYTE mapedge:2;
    BYTE ground:4;
    BYTE ramp:1;
    BYTE blight:1;
    BYTE water:1;
    BYTE boundary:1;
    BYTE groundVariation:4;
    BYTE cliffVariation:4; // used also to mark mid-ramp
    BYTE level:4;
    BYTE cliff:4;
};

struct war3map {
    DWORD header;
    DWORD version;
    BYTE tileset;
    DWORD custom;
    LPDWORD grounds;
    LPDWORD cliffs;
    VECTOR2 center;
    DWORD width;
    DWORD height;
    HANDLE vertices;
    DWORD num_grounds;
    DWORD num_cliffs;
};

void CM_LoadMap(LPCSTR mapFilename);
float CM_GetHeightAtPoint(float sx, float sy);
LPDOODAD CM_GetDoodads(void);
LPCMAPPLAYER CM_GetPlayer(DWORD index);
LPCMAPINFO CM_GetMapInfo(void);
VECTOR2 CM_GetNormalizedMapPosition(float x, float y);
VECTOR2 CM_GetDenormalizedMapPosition(float x, float y);

#endif
