#include "games/starcraft-2/common/sc2_cliff_fit.h"

#define SC2_CLIFF_FIT_EPSILON 0.001f

static FLOAT sc2_clamp01(FLOAT value) {
    return MIN(1.0f, MAX(0.0f, value));
}

FLOAT SC2_CliffFitSample(FLOAT const value[SC2_CLIFF_FIT_GRID * SC2_CLIFF_FIT_GRID], FLOAT x, FLOAT y) {
    FLOAT sx = sc2_clamp01(x) * 2.0f;
    FLOAT sy = sc2_clamp01(y) * 2.0f;
    DWORD ix = MIN(1u, (DWORD)floorf(sx));
    DWORD iy = MIN(1u, (DWORD)floorf(sy));
    FLOAT tx = sx - (FLOAT)ix;
    FLOAT ty = sy - (FLOAT)iy;
    FLOAT v00 = value[ix     + iy       * SC2_CLIFF_FIT_GRID];
    FLOAT v10 = value[ix + 1 + iy       * SC2_CLIFF_FIT_GRID];
    FLOAT v01 = value[ix     + (iy + 1) * SC2_CLIFF_FIT_GRID];
    FLOAT v11 = value[ix + 1 + (iy + 1) * SC2_CLIFF_FIT_GRID];
    FLOAT a = v00 + (v10 - v00) * tx;
    FLOAT b = v01 + (v11 - v01) * tx;

    return a + (b - a) * ty;
}

FLOAT SC2_CliffFitSampleCorners(FLOAT const value[SC2_CLIFF_FIT_CORNERS], FLOAT x, FLOAT y) {
    FLOAT cx = sc2_clamp01(x);
    FLOAT cy = sc2_clamp01(y);
    FLOAT a = value[0] + (value[1] - value[0]) * cx;
    FLOAT b = value[3] + (value[2] - value[3]) * cx;

    return a + (b - a) * cy;
}

VECTOR2 SC2_CliffFitCoord(sc2CliffHeightFit_t const *fit, LPCVECTOR3 rotated) {
    if (!fit || !rotated)
        return (VECTOR2){ 0.0f, 0.0f };
    return (VECTOR2){
        sc2_clamp01((rotated->x - fit->footprint.min.x) / (fit->footprint.max.x - fit->footprint.min.x)),
        sc2_clamp01((rotated->y - fit->footprint.min.y) / (fit->footprint.max.y - fit->footprint.min.y)),
    };
}

FLOAT SC2_CliffFitVertexZ(sc2CliffHeightFit_t const *fit, LPCBOX3 bounds, LPCVECTOR3 local, LPCVECTOR3 rotated) {
    VECTOR2 coord = SC2_CliffFitCoord(fit, rotated);
    FLOAT terrain_z;
    FLOAT terrain_level;
    FLOAT model_level;

    if (!fit || !bounds || !local)
        return 0.0f;
    terrain_z = SC2_CliffFitSample(fit->height, coord.x, coord.y);
    terrain_level = SC2_CliffFitSampleCorners(fit->level, coord.x, coord.y);
    model_level = fit->model_span > SC2_CLIFF_FIT_EPSILON ? (local->z - bounds->min.z) / fit->model_span : terrain_level;
    return terrain_z + (model_level - terrain_level) * fit->terrain_span;
}

VECTOR2 SC2_CliffFitVertexXY(sc2CliffHeightFit_t const *fit, FLOAT cell_size, LPCVECTOR2 offset, LPCVECTOR3 rotated) {
    VECTOR2 coord = SC2_CliffFitCoord(fit, rotated);
    FLOAT span = 2.0f * cell_size;

    if (!offset)
        return (VECTOR2){ 0.0f, 0.0f };
    return (VECTOR2){
        offset->x + (coord.x - 0.5f) * span,
        offset->y + (coord.y - 0.5f) * span,
    };
}
