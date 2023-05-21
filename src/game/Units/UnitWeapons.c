#include "../g_local.h"
#include "UnitWeapons.h"

static struct UnitWeapons *g_UnitWeapons = NULL;

static struct SheetLayout UnitWeapons[] = {
	{ "unitWeapID", ST_ID, FOFS(UnitWeapons, unitWeapID) },
	{ "sortWeap", ST_ID, FOFS(UnitWeapons, sortWeap) },
	{ "sort2", ST_ID, FOFS(UnitWeapons, sort2) },
	{ "comment(s)", ST_STRING, FOFS(UnitWeapons, comment) },
	{ "weapsOn", ST_INT, FOFS(UnitWeapons, weapsOn) },
	{ "weapType1", ST_STRING, FOFS(UnitWeapons, weapType1) },
	{ "targs1", ST_STRING, FOFS(UnitWeapons, targs1) },
	{ "acquire", ST_INT, FOFS(UnitWeapons, acquire) },
	{ "minRange", ST_INT, FOFS(UnitWeapons, minRange) },
	{ "rangeN1", ST_INT, FOFS(UnitWeapons, rangeN1) },
	{ "RngTst", ST_INT, FOFS(UnitWeapons, RngTst) },
	{ "RngBuff1", ST_INT, FOFS(UnitWeapons, RngBuff1) },
	{ "atkType1", ST_STRING, FOFS(UnitWeapons, atkType1) },
	{ "weapTp1", ST_STRING, FOFS(UnitWeapons, weapTp1) },
	{ "cool1", ST_FLOAT, FOFS(UnitWeapons, cool1) },
	{ "mincool1", ST_FLOAT, FOFS(UnitWeapons, mincool1) },
	{ "dice1", ST_INT, FOFS(UnitWeapons, dice1) },
	{ "sides1", ST_INT, FOFS(UnitWeapons, sides1) },
	{ "dmgplus1", ST_INT, FOFS(UnitWeapons, dmgplus1) },
	{ "DmgUpg", ST_STRING, FOFS(UnitWeapons, DmgUpg) },
	{ "dmod1", ST_STRING, FOFS(UnitWeapons, dmod1) },
	{ "dmgUp1", ST_INT, FOFS(UnitWeapons, dmgUp1) },
	{ "mindmg1", ST_INT, FOFS(UnitWeapons, mindmg1) },
	{ "avgdmg1", ST_FLOAT, FOFS(UnitWeapons, avgdmg1) },
	{ "DPS", ST_STRING, FOFS(UnitWeapons, DPS) },
	{ "maxdmg1", ST_INT, FOFS(UnitWeapons, maxdmg1) },
	{ "dmgpt1", ST_FLOAT, FOFS(UnitWeapons, dmgpt1) },
	{ "backSw1", ST_FLOAT, FOFS(UnitWeapons, backSw1) },
	{ "Farea1", ST_INT, FOFS(UnitWeapons, Farea1) },
	{ "Harea1", ST_INT, FOFS(UnitWeapons, Harea1) },
	{ "Qarea1", ST_INT, FOFS(UnitWeapons, Qarea1) },
	{ "Hfact1", ST_FLOAT, FOFS(UnitWeapons, Hfact1) },
	{ "Qfact1", ST_FLOAT, FOFS(UnitWeapons, Qfact1) },
	{ "splashTargs1", ST_STRING, FOFS(UnitWeapons, splashTargs1) },
	{ "targCount1", ST_INT, FOFS(UnitWeapons, targCount1) },
	{ "damageLoss1", ST_FLOAT, FOFS(UnitWeapons, damageLoss1) },
	{ "spillDist1", ST_INT, FOFS(UnitWeapons, spillDist1) },
	{ "spillRadius1", ST_INT, FOFS(UnitWeapons, spillRadius1) },
	{ "weapType2", ST_STRING, FOFS(UnitWeapons, weapType2) },
	{ "targs2", ST_STRING, FOFS(UnitWeapons, targs2) },
	{ "rangeN2", ST_INT, FOFS(UnitWeapons, rangeN2) },
	{ "RngTst2", ST_INT, FOFS(UnitWeapons, RngTst2) },
	{ "RngBuff2", ST_INT, FOFS(UnitWeapons, RngBuff2) },
	{ "atkType2", ST_STRING, FOFS(UnitWeapons, atkType2) },
	{ "weapTp2", ST_STRING, FOFS(UnitWeapons, weapTp2) },
	{ "cool2", ST_FLOAT, FOFS(UnitWeapons, cool2) },
	{ "mincool2", ST_INT, FOFS(UnitWeapons, mincool2) },
	{ "dice2", ST_INT, FOFS(UnitWeapons, dice2) },
	{ "sides2", ST_INT, FOFS(UnitWeapons, sides2) },
	{ "dmgplus2", ST_INT, FOFS(UnitWeapons, dmgplus2) },
	{ "dmgUp2", ST_INT, FOFS(UnitWeapons, dmgUp2) },
	{ "mindmg2", ST_INT, FOFS(UnitWeapons, mindmg2) },
	{ "avgdmg2", ST_FLOAT, FOFS(UnitWeapons, avgdmg2) },
	{ "maxdmg2", ST_FLOAT, FOFS(UnitWeapons, maxdmg2) },
	{ "dmgpt2", ST_FLOAT, FOFS(UnitWeapons, dmgpt2) },
	{ "backSw2", ST_FLOAT, FOFS(UnitWeapons, backSw2) },
	{ "Farea2", ST_INT, FOFS(UnitWeapons, Farea2) },
	{ "Harea2", ST_INT, FOFS(UnitWeapons, Harea2) },
	{ "Qarea2", ST_INT, FOFS(UnitWeapons, Qarea2) },
	{ "Hfact2", ST_FLOAT, FOFS(UnitWeapons, Hfact2) },
	{ "Qfact2", ST_FLOAT, FOFS(UnitWeapons, Qfact2) },
	{ "splashTargs2", ST_STRING, FOFS(UnitWeapons, splashTargs2) },
	{ "targCount2", ST_INT, FOFS(UnitWeapons, targCount2) },
	{ "damageLoss2", ST_INT, FOFS(UnitWeapons, damageLoss2) },
	{ "spillDist2", ST_INT, FOFS(UnitWeapons, spillDist2) },
	{ "spillRadius2", ST_INT, FOFS(UnitWeapons, spillRadius2) },
	{ "InBeta", ST_INT, FOFS(UnitWeapons, InBeta) },
	{ NULL },
};

struct UnitWeapons *FindUnitWeapons(DWORD unitWeapID) {
	struct UnitWeapons *lpValue = g_UnitWeapons;
	for (; lpValue->unitWeapID != unitWeapID && lpValue->unitWeapID; lpValue++);
	if (lpValue->unitWeapID == 0) lpValue = NULL;
	return lpValue;
}

void InitUnitWeapons(void) {
	g_UnitWeapons = gi.ParseSheet("Units\\UnitWeapons.slk", UnitWeapons, sizeof(struct UnitWeapons));
}
void ShutdownUnitWeapons(void) {
	gi.MemFree(g_UnitWeapons);
}
