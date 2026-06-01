#ifndef WORLD_LOCAL_H
#define WORLD_LOCAL_H

#include "common.h"
#include "mpq.h"

struct world_state {
    LPWAR3MAP map;
    MAPINFO info;
    struct Doodad *doodads;
};

extern struct world_state world;

/* Implemented in world_w3.c / world_wow.c respectively */
bool     CM_LoadMapFormat(LPCSTR mapFilename);
FLOAT    CM_GetHeightAtPoint(FLOAT sx, FLOAT sy);
VECTOR2  CM_GetNormalizedMapPosition(FLOAT x, FLOAT y);
VECTOR2  CM_GetDenormalizedMapPosition(FLOAT x, FLOAT y);
BOX2     CM_GetWorldBounds(void);

#endif
