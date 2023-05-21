#include "../g_local.h"
#include "AbilityData.h"

static struct AbilityData *g_AbilityData = NULL;

static struct SheetLayout AbilityData[] = {
	{ "alias", ST_ID, FOFS(AbilityData, alias) },
	{ "code", ST_ID, FOFS(AbilityData, code) },
	{ "uberAlias", ST_ID, FOFS(AbilityData, uberAlias) },
	{ "comments", ST_STRING, FOFS(AbilityData, comments) },
	{ "useInEditor", ST_INT, FOFS(AbilityData, useInEditor) },
	{ "hero", ST_INT, FOFS(AbilityData, hero) },
	{ "item", ST_INT, FOFS(AbilityData, item) },
	{ "sort", ST_ID, FOFS(AbilityData, sort) },
	{ "checkDep", ST_INT, FOFS(AbilityData, checkDep) },
	{ "levels", ST_INT, FOFS(AbilityData, levels) },
	{ "reqLevel", ST_INT, FOFS(AbilityData, reqLevel) },
	{ "targs", ST_STRING, FOFS(AbilityData, targs) },
	{ "Cast1", ST_FLOAT, FOFS(AbilityData, Cast1) },
	{ "Dur1", ST_FLOAT, FOFS(AbilityData, Dur1) },
	{ "HeroDur1", ST_FLOAT, FOFS(AbilityData, HeroDur1) },
	{ "Cool1", ST_FLOAT, FOFS(AbilityData, Cool1) },
	{ "Cost1", ST_INT, FOFS(AbilityData, Cost1) },
	{ "Area1", ST_ID, FOFS(AbilityData, Area1) },
	{ "Rng1", ST_INT, FOFS(AbilityData, Rng1) },
	{ "Data11", ST_STRING, FOFS(AbilityData, Data11) },
	{ "Data12", ST_ID, FOFS(AbilityData, Data12) },
	{ "Data13", ST_FLOAT, FOFS(AbilityData, Data13) },
	{ "Data14", ST_ID, FOFS(AbilityData, Data14) },
	{ "UnitID1", ST_ID, FOFS(AbilityData, UnitID1) },
	{ "Cast2", ST_INT, FOFS(AbilityData, Cast2) },
	{ "Dur2", ST_FLOAT, FOFS(AbilityData, Dur2) },
	{ "HeroDur2", ST_FLOAT, FOFS(AbilityData, HeroDur2) },
	{ "Cool2", ST_INT, FOFS(AbilityData, Cool2) },
	{ "Cost2", ST_INT, FOFS(AbilityData, Cost2) },
	{ "Area2", ST_INT, FOFS(AbilityData, Area2) },
	{ "Rng2", ST_INT, FOFS(AbilityData, Rng2) },
	{ "Data21", ST_STRING, FOFS(AbilityData, Data21) },
	{ "Data22", ST_ID, FOFS(AbilityData, Data22) },
	{ "Data23", ST_ID, FOFS(AbilityData, Data23) },
	{ "Data24", ST_ID, FOFS(AbilityData, Data24) },
	{ "UnitID2", ST_INT, FOFS(AbilityData, UnitID2) },
	{ "Cast3", ST_INT, FOFS(AbilityData, Cast3) },
	{ "Dur3", ST_FLOAT, FOFS(AbilityData, Dur3) },
	{ "HeroDur3", ST_INT, FOFS(AbilityData, HeroDur3) },
	{ "Cool3", ST_INT, FOFS(AbilityData, Cool3) },
	{ "Cost3", ST_INT, FOFS(AbilityData, Cost3) },
	{ "Area3", ST_INT, FOFS(AbilityData, Area3) },
	{ "Rng3", ST_INT, FOFS(AbilityData, Rng3) },
	{ "Data31", ST_STRING, FOFS(AbilityData, Data31) },
	{ "Data32", ST_ID, FOFS(AbilityData, Data32) },
	{ "Data33", ST_ID, FOFS(AbilityData, Data33) },
	{ "Data34", ST_ID, FOFS(AbilityData, Data34) },
	{ "UnitID3", ST_INT, FOFS(AbilityData, UnitID3) },
	{ "CastCheck", ST_INT, FOFS(AbilityData, CastCheck) },
	{ "DurCheck", ST_INT, FOFS(AbilityData, DurCheck) },
	{ "HeroDurCheck", ST_INT, FOFS(AbilityData, HeroDurCheck) },
	{ "CoolCheck", ST_INT, FOFS(AbilityData, CoolCheck) },
	{ "CostCheck", ST_INT, FOFS(AbilityData, CostCheck) },
	{ "AreaCheck", ST_INT, FOFS(AbilityData, AreaCheck) },
	{ "RngCheck", ST_INT, FOFS(AbilityData, RngCheck) },
	{ NULL },
};

struct AbilityData *FindAbilityData(DWORD alias) {
	struct AbilityData *lpValue = g_AbilityData;
	for (; lpValue->alias != alias && lpValue->alias; lpValue++);
	if (lpValue->alias == 0) lpValue = NULL;
	return lpValue;
}

void InitAbilityData(void) {
	g_AbilityData = gi.ParseSheet("Units\\AbilityData.slk", AbilityData, sizeof(struct AbilityData));
}
void ShutdownAbilityData(void) {
	gi.MemFree(g_AbilityData);
}
