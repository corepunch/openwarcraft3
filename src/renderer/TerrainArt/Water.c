#include "../r_local.h"
#include "Water.h"

static struct Water *g_Water = NULL;

static struct SheetLayout Water[] = {
	{ "waterID", ST_ID, FOFS(Water, waterID) },
	{ "height", ST_FLOAT, FOFS(Water, height) },
	{ "texFile", ST_STRING, FOFS(Water, texFile) },
	{ "mmAlpha", ST_INT, FOFS(Water, mmAlpha) },
	{ "mmRed", ST_INT, FOFS(Water, mmRed) },
	{ "mmGreen", ST_INT, FOFS(Water, mmGreen) },
	{ "mmBlue", ST_INT, FOFS(Water, mmBlue) },
	{ "numTex", ST_INT, FOFS(Water, numTex) },
	{ "texRate", ST_INT, FOFS(Water, texRate) },
	{ "texOffset", ST_INT, FOFS(Water, texOffset) },
	{ "alphaMode", ST_INT, FOFS(Water, alphaMode) },
	{ "lighting", ST_INT, FOFS(Water, lighting) },
	{ "cells", ST_INT, FOFS(Water, cells) },
	{ "minX", ST_INT, FOFS(Water, minX) },
	{ "minY", ST_INT, FOFS(Water, minY) },
	{ "minZ", ST_INT, FOFS(Water, minZ) },
	{ "maxX", ST_INT, FOFS(Water, maxX) },
	{ "maxY", ST_INT, FOFS(Water, maxY) },
	{ "maxZ", ST_INT, FOFS(Water, maxZ) },
	{ "rateX", ST_INT, FOFS(Water, rateX) },
	{ "rateY", ST_INT, FOFS(Water, rateY) },
	{ "rateZ", ST_INT, FOFS(Water, rateZ) },
	{ "revX", ST_INT, FOFS(Water, revX) },
	{ "revY", ST_INT, FOFS(Water, revY) },
	{ "shoreDir", ST_STRING, FOFS(Water, shoreDir) },
	{ "shoreSFile", ST_STRING, FOFS(Water, shoreSFile) },
	{ "shoreSVar", ST_INT, FOFS(Water, shoreSVar) },
	{ "shoreOCFile", ST_STRING, FOFS(Water, shoreOCFile) },
	{ "shoreOCVar", ST_INT, FOFS(Water, shoreOCVar) },
	{ "shoreICFile", ST_STRING, FOFS(Water, shoreICFile) },
	{ "shoreICVar", ST_INT, FOFS(Water, shoreICVar) },
	{ "Smin_A", ST_INT, FOFS(Water, Smin_A) },
	{ "Smin_R", ST_INT, FOFS(Water, Smin_R) },
	{ "Smin_G", ST_INT, FOFS(Water, Smin_G) },
	{ "Smin_B", ST_INT, FOFS(Water, Smin_B) },
	{ "Smax_A", ST_INT, FOFS(Water, Smax_A) },
	{ "Smax_R", ST_INT, FOFS(Water, Smax_R) },
	{ "Smax_G", ST_INT, FOFS(Water, Smax_G) },
	{ "Smax_B", ST_INT, FOFS(Water, Smax_B) },
	{ "Dmin_A", ST_INT, FOFS(Water, Dmin_A) },
	{ "Dmin_R", ST_INT, FOFS(Water, Dmin_R) },
	{ "Dmin_G", ST_INT, FOFS(Water, Dmin_G) },
	{ "Dmin_B", ST_INT, FOFS(Water, Dmin_B) },
	{ "Dmax_A", ST_INT, FOFS(Water, Dmax_A) },
	{ "Dmax_R", ST_INT, FOFS(Water, Dmax_R) },
	{ "Dmax_G", ST_INT, FOFS(Water, Dmax_G) },
	{ "Dmax_B", ST_INT, FOFS(Water, Dmax_B) },
	{ NULL },
};

struct Water *FindWater(DWORD waterID) {
	struct Water *lpValue = g_Water;
	for (; lpValue->waterID != waterID && lpValue->waterID; lpValue++);
	if (lpValue->waterID == 0) lpValue = NULL;
	return lpValue;
}

void InitWater(void) {
	g_Water = ri.ParseSheet("TerrainArt\\Water.slk", Water, sizeof(struct Water));
}
void ShutdownWater(void) {
	ri.MemFree(g_Water);
}
