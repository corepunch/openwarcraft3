#include "../g_local.h"
#include "UnitBalance.h"

static LPUNITBALANCE g_UnitBalance = NULL;

static struct SheetLayout UnitBalance[] = {
	{ "unitBalanceID", ST_ID, FOFS(UnitBalance, unitBalanceID) },
	{ "sortBalance", ST_ID, FOFS(UnitBalance, sortBalance) },
	{ "sort2", ST_ID, FOFS(UnitBalance, sort2) },
	{ "comment(s)", ST_STRING, FOFS(UnitBalance, comment) },
	{ "level", ST_INT, FOFS(UnitBalance, level) },
	{ "goldcost", ST_INT, FOFS(UnitBalance, goldcost) },
	{ "lumbercost", ST_INT, FOFS(UnitBalance, lumbercost) },
	{ "goldRep", ST_INT, FOFS(UnitBalance, goldRep) },
	{ "lumberRep", ST_INT, FOFS(UnitBalance, lumberRep) },
	{ "fmade", ST_INT, FOFS(UnitBalance, fmade) },
	{ "fused", ST_INT, FOFS(UnitBalance, fused) },
	{ "bountydice", ST_INT, FOFS(UnitBalance, bountydice) },
	{ "bountysides", ST_INT, FOFS(UnitBalance, bountysides) },
	{ "bountyplus", ST_INT, FOFS(UnitBalance, bountyplus) },
	{ "stockMax", ST_INT, FOFS(UnitBalance, stockMax) },
	{ "stockRegen", ST_INT, FOFS(UnitBalance, stockRegen) },
	{ "stockStart", ST_INT, FOFS(UnitBalance, stockStart) },
	{ "HP", ST_INT, FOFS(UnitBalance, HP) },
	{ "realHP", ST_INT, FOFS(UnitBalance, realHP) },
	{ "regenHP", ST_FLOAT, FOFS(UnitBalance, regenHP) },
	{ "regenType", ST_STRING, FOFS(UnitBalance, regenType) },
	{ "manaN", ST_INT, FOFS(UnitBalance, manaN) },
	{ "realM", ST_INT, FOFS(UnitBalance, realM) },
	{ "mana0", ST_INT, FOFS(UnitBalance, mana0) },
	{ "regenMana", ST_FLOAT, FOFS(UnitBalance, regenMana) },
	{ "def", ST_INT, FOFS(UnitBalance, def) },
	{ "defUp", ST_INT, FOFS(UnitBalance, defUp) },
	{ "realdef", ST_FLOAT, FOFS(UnitBalance, realdef) },
	{ "defType", ST_STRING, FOFS(UnitBalance, defType) },
	{ "spd", ST_INT, FOFS(UnitBalance, spd) },
	{ "bldtm", ST_INT, FOFS(UnitBalance, bldtm) },
	{ "sight", ST_INT, FOFS(UnitBalance, sight) },
	{ "nsight", ST_INT, FOFS(UnitBalance, nsight) },
	{ "STR", ST_INT, FOFS(UnitBalance, STR) },
	{ "INT", ST_INT, FOFS(UnitBalance, INT) },
	{ "AGI", ST_INT, FOFS(UnitBalance, AGI) },
	{ "STRplus", ST_FLOAT, FOFS(UnitBalance, STRplus) },
	{ "INTplus", ST_FLOAT, FOFS(UnitBalance, INTplus) },
	{ "AGIplus", ST_FLOAT, FOFS(UnitBalance, AGIplus) },
	{ "abilTest", ST_FLOAT, FOFS(UnitBalance, abilTest) },
	{ "Primary", ST_ID, FOFS(UnitBalance, Primary) },
	{ "upgrades", ST_STRING, FOFS(UnitBalance, upgrades) },
	{ "InBeta", ST_INT, FOFS(UnitBalance, InBeta) },
	{ NULL },
};

LPCUNITBALANCE FindUnitBalance(DWORD unitBalanceID) {
	struct UnitBalance *lpValue = g_UnitBalance;
	for (; lpValue->unitBalanceID != unitBalanceID && lpValue->unitBalanceID; lpValue++);
	if (lpValue->unitBalanceID == 0) lpValue = NULL;
	return lpValue;
}

void InitUnitBalance(void) {
	g_UnitBalance = gi.ParseSheet("Units\\UnitBalance.slk", UnitBalance, sizeof(struct UnitBalance));
}
void ShutdownUnitBalance(void) {
	gi.MemFree(g_UnitBalance);
}
