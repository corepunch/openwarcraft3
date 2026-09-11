#ifndef __wc3_common_terrain_h__
#define __wc3_common_terrain_h__

#include "common/mapinfo.h"

/* Warcraft III W3E terrain decode constants belong to the game module, not shared engine code. */
#define HEIGHT_COR (TILE_SIZE * 2) /* world units; W3E layerHeight - 2 correction */
#define WATER_HEIGHT_COR 80
#define DECODE_HEIGHT(x) (((x) - 0x2000) / 4)

#endif
