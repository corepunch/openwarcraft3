#include "../g_local.h"
#include "UnitData.h"

static struct UnitData *g_UnitData = NULL;

static struct SheetLayout UnitData[] = {
	{ "unitID", ST_ID, FOFS(UnitData, unitID) },
	{ "sort", ST_ID, FOFS(UnitData, sort) },
	{ "comment(s)", ST_STRING, FOFS(UnitData, comment) },
	{ "race", ST_STRING, FOFS(UnitData, race) },
	{ "prio", ST_INT, FOFS(UnitData, prio) },
	{ "threat", ST_INT, FOFS(UnitData, threat) },
	{ "type", ST_STRING, FOFS(UnitData, type) },
	{ "valid", ST_INT, FOFS(UnitData, valid) },
	{ "deathType", ST_INT, FOFS(UnitData, deathType) },
	{ "death", ST_FLOAT, FOFS(UnitData, death) },
	{ "canSleep", ST_INT, FOFS(UnitData, canSleep) },
	{ "cargoSize", ST_INT, FOFS(UnitData, cargoSize) },
	{ "movetp", ST_STRING, FOFS(UnitData, movetp) },
	{ "moveHeight", ST_INT, FOFS(UnitData, moveHeight) },
	{ "moveFloor", ST_INT, FOFS(UnitData, moveFloor) },
	{ "launchX", ST_INT, FOFS(UnitData, launchX) },
	{ "launchY", ST_INT, FOFS(UnitData, launchY) },
	{ "launchZ", ST_INT, FOFS(UnitData, launchZ) },
	{ "impactZ", ST_INT, FOFS(UnitData, impactZ) },
	{ "turnRate", ST_FLOAT, FOFS(UnitData, turnRate) },
	{ "propWin", ST_INT, FOFS(UnitData, propWin) },
	{ "orientInterp", ST_INT, FOFS(UnitData, orientInterp) },
	{ "formation", ST_INT, FOFS(UnitData, formation) },
	{ "castpt", ST_FLOAT, FOFS(UnitData, castpt) },
	{ "castbsw", ST_FLOAT, FOFS(UnitData, castbsw) },
	{ "targType", ST_STRING, FOFS(UnitData, targType) },
	{ "pathTex", ST_STRING, FOFS(UnitData, pathTex) },
	{ "fatLOS", ST_INT, FOFS(UnitData, fatLOS) },
	{ "collision", ST_INT, FOFS(UnitData, collision) },
	{ "points", ST_INT, FOFS(UnitData, points) },
	{ "buffType", ST_STRING, FOFS(UnitData, buffType) },
	{ "buffRadius", ST_INT, FOFS(UnitData, buffRadius) },
	{ "nameCount", ST_INT, FOFS(UnitData, nameCount) },
	{ "InBeta", ST_INT, FOFS(UnitData, InBeta) },
	{ NULL },
};

struct UnitData *FindUnitData(DWORD unitID) {
	struct UnitData *lpValue = g_UnitData;
	for (; lpValue->unitID != unitID && lpValue->unitID; lpValue++);
	if (lpValue->unitID == 0) lpValue = NULL;
	return lpValue;
}

void InitUnitData(void) {
	g_UnitData = gi.ParseSheet("Units\\UnitData.slk", UnitData, sizeof(struct UnitData));
}
void ShutdownUnitData(void) {
	gi.MemFree(g_UnitData);
}
