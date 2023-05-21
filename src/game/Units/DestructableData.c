#include "../g_local.h"
#include "DestructableData.h"

static LPDESTRUCTABLEDATA g_DestructableData = NULL;

static struct SheetLayout DestructableData[] = {
	{ "DestructableID", ST_ID, FOFS(DestructableData, DestructableID) },
	{ "category", ST_ID, FOFS(DestructableData, category) },
	{ "tilesets", ST_STRING, FOFS(DestructableData, tilesets) },
	{ "tilesetSpecific", ST_INT, FOFS(DestructableData, tilesetSpecific) },
	{ "dir", ST_STRING, FOFS(DestructableData, dir) },
	{ "file", ST_STRING, FOFS(DestructableData, file) },
	{ "lightweight", ST_INT, FOFS(DestructableData, lightweight) },
	{ "fatLOS", ST_INT, FOFS(DestructableData, fatLOS) },
	{ "texID", ST_INT, FOFS(DestructableData, texID) },
	{ "texFile", ST_STRING, FOFS(DestructableData, texFile) },
	{ "comment", ST_STRING, FOFS(DestructableData, comment) },
	{ "name", ST_STRING, FOFS(DestructableData, name) },
	{ "doodClass", ST_ID, FOFS(DestructableData, doodClass) },
	{ "useClickHelper", ST_INT, FOFS(DestructableData, useClickHelper) },
	{ "onCliffs", ST_INT, FOFS(DestructableData, onCliffs) },
	{ "onWater", ST_INT, FOFS(DestructableData, onWater) },
	{ "canPlaceDead", ST_INT, FOFS(DestructableData, canPlaceDead) },
	{ "walkable", ST_INT, FOFS(DestructableData, walkable) },
	{ "cliffHeight", ST_INT, FOFS(DestructableData, cliffHeight) },
	{ "targType", ST_STRING, FOFS(DestructableData, targType) },
	{ "armor", ST_STRING, FOFS(DestructableData, armor) },
	{ "numVar", ST_INT, FOFS(DestructableData, numVar) },
	{ "HP", ST_INT, FOFS(DestructableData, HP) },
	{ "occH", ST_INT, FOFS(DestructableData, occH) },
	{ "flyH", ST_INT, FOFS(DestructableData, flyH) },
	{ "fixedRot", ST_INT, FOFS(DestructableData, fixedRot) },
	{ "selSize", ST_INT, FOFS(DestructableData, selSize) },
	{ "minScale", ST_FLOAT, FOFS(DestructableData, minScale) },
	{ "maxScale", ST_FLOAT, FOFS(DestructableData, maxScale) },
	{ "canPlaceRandScale", ST_INT, FOFS(DestructableData, canPlaceRandScale) },
	{ "maxPitch", ST_INT, FOFS(DestructableData, maxPitch) },
	{ "maxRoll", ST_INT, FOFS(DestructableData, maxRoll) },
	{ "radius", ST_INT, FOFS(DestructableData, radius) },
	{ "fogRadius", ST_INT, FOFS(DestructableData, fogRadius) },
	{ "fogVis", ST_INT, FOFS(DestructableData, fogVis) },
	{ "pathTex", ST_STRING, FOFS(DestructableData, pathTex) },
	{ "pathTexDeath", ST_STRING, FOFS(DestructableData, pathTexDeath) },
	{ "deathSnd", ST_STRING, FOFS(DestructableData, deathSnd) },
	{ "shadow", ST_STRING, FOFS(DestructableData, shadow) },
	{ "showInMM", ST_INT, FOFS(DestructableData, showInMM) },
	{ "useMMColor", ST_INT, FOFS(DestructableData, useMMColor) },
	{ "MMRed", ST_INT, FOFS(DestructableData, MMRed) },
	{ "MMGreen", ST_INT, FOFS(DestructableData, MMGreen) },
	{ "MMBlue", ST_INT, FOFS(DestructableData, MMBlue) },
	{ "InBeta", ST_INT, FOFS(DestructableData, InBeta) },
	{ NULL },
};

LPCDESTRUCTABLEDATA FindDestructableData(DWORD DestructableID) {
	struct DestructableData *lpValue = g_DestructableData;
	for (; lpValue->DestructableID != DestructableID && lpValue->DestructableID; lpValue++);
	if (lpValue->DestructableID == 0) lpValue = NULL;
	return lpValue;
}

void InitDestructableData(void) {
	g_DestructableData = gi.ParseSheet("Units\\DestructableData.slk", DestructableData, sizeof(struct DestructableData));
}
void ShutdownDestructableData(void) {
	gi.MemFree(g_DestructableData);
}
