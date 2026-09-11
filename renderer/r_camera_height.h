#ifndef R_CAMERA_HEIGHT_H
#define R_CAMERA_HEIGHT_H

#include "r_local.h"

typedef struct {
    FLOAT *samples;
    DWORD width, height;
    VECTOR2 origin;
    FLOAT cell_size;
} cameraHeightMap_t;

typedef struct {
    cameraHeightMap_t *map;
    LPCVOID data;
    DWORD width, height_count, radius, samples;
    VECTOR2 origin;
    FLOAT cell_size;
    FLOAT (*get_height)(LPCVOID data, DWORD x, DWORD y);
} cameraHeightBuild_t;

void R_BuildCameraHeightMap(cameraHeightBuild_t const *params);
void R_FreeCameraHeightMap(cameraHeightMap_t *map);
FLOAT R_SampleCameraHeightMap(cameraHeightMap_t const *map, FLOAT x, FLOAT y);

#endif
