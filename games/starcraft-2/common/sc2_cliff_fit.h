#ifndef SC2_CLIFF_FIT_H
#define SC2_CLIFF_FIT_H

#include "common/shared.h"

#define SC2_CLIFF_FIT_GRID 3u
#define SC2_CLIFF_FIT_CORNERS 4u

typedef struct sc2CliffHeightFit_s {
    FLOAT height[SC2_CLIFF_FIT_GRID * SC2_CLIFF_FIT_GRID];
    FLOAT level[SC2_CLIFF_FIT_CORNERS];
    BOX2 footprint;
    FLOAT terrain_span;
    FLOAT model_span;
} sc2CliffHeightFit_t;

FLOAT SC2_CliffFitSample(FLOAT const value[SC2_CLIFF_FIT_GRID * SC2_CLIFF_FIT_GRID], FLOAT x, FLOAT y);
FLOAT SC2_CliffFitSampleCorners(FLOAT const value[SC2_CLIFF_FIT_CORNERS], FLOAT x, FLOAT y);
VECTOR2 SC2_CliffFitCoord(sc2CliffHeightFit_t const *fit, LPCVECTOR3 rotated);
FLOAT SC2_CliffFitVertexZ(sc2CliffHeightFit_t const *fit, LPCBOX3 bounds, LPCVECTOR3 local, LPCVECTOR3 rotated);
VECTOR2 SC2_CliffFitVertexXY(sc2CliffHeightFit_t const *fit, FLOAT cell_size, LPCVECTOR2 offset, LPCVECTOR3 rotated);

#endif
