#ifndef __Water_h__
#define __Water_h__

#include "../../common/common.h"

typedef char SHEETSTR[80];

struct Water {
	DWORD waterID; /* BSha CSha DSha ASha NSha WSha FSha LSha */
	float height; /* -0.7 -0.7 -0.75 -0.6 -0.7 -0.7 -0.7 -0.7 */
	SHEETSTR texFile; /* ReplaceableTextures\Water\Water ReplaceableTextures\Water\Water ReplaceableTextures\Water\Water ReplaceableTextures\Water\Water ReplaceableTextures\Water\Water ReplaceableTextures\Water\Water ReplaceableTextures\Water\Water ReplaceableTextures\Water\Water */
	DWORD mmAlpha; /* 64 64 64 64 64 64 64 64 */
	DWORD mmRed; /* 0 0 128 0 0 0 0 0 */
	DWORD mmGreen; /* 0 0 0 0 0 0 0 0 */
	DWORD mmBlue; /* 192 192 0 192 192 192 192 192 */
	DWORD numTex; /* 45 45 45 45 45 45 45 45 */
	DWORD texRate; /* 15 12 12 12 15 15 15 15 */
	DWORD texOffset; /* 0 0 0 0 0 0 0 0 */
	DWORD alphaMode; /* 0 0 0 0 0 0 0 0 */
	DWORD lighting; /* 1 0 0 1 1 1 1 1 */
	DWORD cells; /* 2 2 2 2 2 2 2 2 */
	DWORD minX; /* 0 0 0 0 0 0 0 0 */
	DWORD minY; /* 0 0 0 0 0 0 0 0 */
	DWORD minZ; /* 0 0 0 0 0 0 0 0 */
	DWORD maxX; /* 0 0 0 0 0 0 0 0 */
	DWORD maxY; /* 0 0 0 0 0 0 0 0 */
	DWORD maxZ; /* 0 0 0 0 0 0 0 0 */
	DWORD rateX; /* 8 8 8 8 8 8 8 8 */
	DWORD rateY; /* 8 8 8 8 8 8 8 8 */
	DWORD rateZ; /* 1 1 1 1 1 1 1 1 */
	DWORD revX; /* 1 1 1 1 1 1 1 1 */
	DWORD revY; /* 1 1 1 1 1 1 1 1 */
	SHEETSTR shoreDir; /* Doodads\LordaeronSummer\Water Doodads\LordaeronSummer\Water Doodads\LordaeronSummer\Water Doodads\LordaeronSummer\Water Doodads\LordaeronSummer\Water Doodads\LordaeronSummer\Water Doodads\LordaeronSummer\Water Doodads\LordaeronSummer\Water */
	SHEETSTR shoreSFile; /* Shoreline Shoreline Shoreline Shoreline Shoreline Shoreline Shoreline Shoreline */
	DWORD shoreSVar; /* 1 1 1 1 1 1 1 1 */
	SHEETSTR shoreOCFile; /* ShorelineOutsideCorner ShorelineOutsideCorner ShorelineOutsideCorner ShorelineOutsideCorner ShorelineOutsideCorner ShorelineOutsideCorner ShorelineOutsideCorner ShorelineOutsideCorner */
	DWORD shoreOCVar; /* 1 1 1 1 1 1 1 1 */
	SHEETSTR shoreICFile; /* ShorelineInsideCorner ShorelineInsideCorner ShorelineInsideCorner ShorelineInsideCorner ShorelineInsideCorner ShorelineInsideCorner ShorelineInsideCorner ShorelineInsideCorner */
	DWORD shoreICVar; /* 1 1 1 1 1 1 1 1 */
	DWORD Smin_A; /* 10 10 10 10 10 10 10 10 */
	DWORD Smin_R; /* 255 200 255 100 255 255 255 255 */
	DWORD Smin_G; /* 255 255 255 100 255 255 255 255 */
	DWORD Smin_B; /* 255 255 255 100 255 255 255 255 */
	DWORD Smax_A; /* 255 219 180 150 219 219 219 219 */
	DWORD Smax_R; /* 200 200 255 150 240 117 117 117 */
	DWORD Smax_G; /* 117 255 255 150 240 117 117 117 */
	DWORD Smax_B; /* 200 255 255 255 240 200 200 200 */
	DWORD Dmin_A; /* 219 219 219 219 219 219 219 219 */
	DWORD Dmin_R; /* 117 200 255 200 117 117 117 117 */
	DWORD Dmin_G; /* 117 255 255 255 117 117 117 117 */
	DWORD Dmin_B; /* 255 255 255 255 117 200 200 200 */
	DWORD Dmax_A; /* 250 250 250 250 180 250 250 250 */
	DWORD Dmax_R; /* 255 255 255 150 150 96 96 96 */
	DWORD Dmax_G; /* 255 255 255 150 180 96 96 96 */
	DWORD Dmax_B; /* 255 255 255 150 220 192 192 192 */
};

struct Water *FindWater(DWORD waterID);
void InitWater(void);
void ShutdownWater(void);

#endif
