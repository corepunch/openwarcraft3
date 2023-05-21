#include "../r_local.h"
#include "CliffTypes.h"

static struct CliffTypes *g_CliffTypes = NULL;

static struct SheetLayout CliffTypes[] = {
	{ "cliffID", ST_ID, FOFS(CliffTypes, cliffID) },
	{ "cliffModelDir", ST_STRING, FOFS(CliffTypes, cliffModelDir) },
	{ "rampModelDir", ST_STRING, FOFS(CliffTypes, rampModelDir) },
	{ "texDir", ST_STRING, FOFS(CliffTypes, texDir) },
	{ "texFile", ST_STRING, FOFS(CliffTypes, texFile) },
	{ "name", ST_STRING, FOFS(CliffTypes, name) },
	{ "groundTile", ST_ID, FOFS(CliffTypes, groundTile) },
	{ "upperTile", ST_INT, FOFS(CliffTypes, upperTile) },
	{ "cliffClass", ST_ID, FOFS(CliffTypes, cliffClass) },
	{ "oldID", ST_INT, FOFS(CliffTypes, oldID) },
	{ NULL },
};

struct CliffTypes *FindCliffTypes(DWORD cliffID) {
	struct CliffTypes *lpValue = g_CliffTypes;
	for (; lpValue->cliffID != cliffID && lpValue->cliffID; lpValue++);
	if (lpValue->cliffID == 0) lpValue = NULL;
	return lpValue;
}

void InitCliffTypes(void) {
	g_CliffTypes = ri.ParseSheet("TerrainArt\\CliffTypes.slk", CliffTypes, sizeof(struct CliffTypes));
}
void ShutdownCliffTypes(void) {
	ri.MemFree(g_CliffTypes);
}
