#include "../r_local.h"
#include "Terrain.h"

static LPTERRAIN g_Terrain = NULL;

static struct SheetLayout Terrain[] = {
	{ "tileID", ST_ID, FOFS(Terrain, tileID) },
	{ "dir", ST_STRING, FOFS(Terrain, dir) },
	{ "file", ST_STRING, FOFS(Terrain, file) },
	{ "comment", ST_STRING, FOFS(Terrain, comment) },
	{ "name", ST_STRING, FOFS(Terrain, name) },
	{ "buildable", ST_INT, FOFS(Terrain, buildable) },
	{ "footprints", ST_INT, FOFS(Terrain, footprints) },
	{ "walkable", ST_INT, FOFS(Terrain, walkable) },
	{ "flyable", ST_INT, FOFS(Terrain, flyable) },
	{ "blightPri", ST_INT, FOFS(Terrain, blightPri) },
	{ "convertTo", ST_STRING, FOFS(Terrain, convertTo) },
	{ "InBeta", ST_INT, FOFS(Terrain, InBeta) },
	{ NULL },
};

LPCTERRAIN FindTerrain(DWORD tileID) {
	struct Terrain *lpValue = g_Terrain;
	for (; lpValue->tileID != tileID && lpValue->tileID; lpValue++);
	if (lpValue->tileID == 0) lpValue = NULL;
	return lpValue;
}

void InitTerrain(void) {
	g_Terrain = ri.ParseSheet("TerrainArt\\Terrain.slk", Terrain, sizeof(struct Terrain));
}
void ShutdownTerrain(void) {
	ri.MemFree(g_Terrain);
}
