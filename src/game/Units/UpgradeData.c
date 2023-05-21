#include "../g_local.h"
#include "UpgradeData.h"

static struct UpgradeData *g_UpgradeData = NULL;

static struct SheetLayout UpgradeData[] = {
	{ "upgradeid", ST_ID, FOFS(UpgradeData, upgradeid) },
	{ "comments", ST_STRING, FOFS(UpgradeData, comments) },
	{ "class", ST_STRING, FOFS(UpgradeData, class) },
	{ "race", ST_STRING, FOFS(UpgradeData, race) },
	{ "flag", ST_INT, FOFS(UpgradeData, flag) },
	{ "used", ST_INT, FOFS(UpgradeData, used) },
	{ "maxlevel", ST_INT, FOFS(UpgradeData, maxlevel) },
	{ "inherit", ST_INT, FOFS(UpgradeData, inherit) },
	{ "goldbase", ST_INT, FOFS(UpgradeData, goldbase) },
	{ "goldmod", ST_INT, FOFS(UpgradeData, goldmod) },
	{ "lumberbase", ST_INT, FOFS(UpgradeData, lumberbase) },
	{ "lumbermod", ST_INT, FOFS(UpgradeData, lumbermod) },
	{ "timebase", ST_INT, FOFS(UpgradeData, timebase) },
	{ "timemod", ST_INT, FOFS(UpgradeData, timemod) },
	{ "effect1", ST_ID, FOFS(UpgradeData, effect1) },
	{ "base1", ST_FLOAT, FOFS(UpgradeData, base1) },
	{ "mod1", ST_FLOAT, FOFS(UpgradeData, mod1) },
	{ "effect2", ST_ID, FOFS(UpgradeData, effect2) },
	{ "base2", ST_FLOAT, FOFS(UpgradeData, base2) },
	{ "mod2", ST_FLOAT, FOFS(UpgradeData, mod2) },
	{ "effect3", ST_ID, FOFS(UpgradeData, effect3) },
	{ "base3", ST_INT, FOFS(UpgradeData, base3) },
	{ "mod3", ST_INT, FOFS(UpgradeData, mod3) },
	{ "effect4", ST_ID, FOFS(UpgradeData, effect4) },
	{ "base4", ST_INT, FOFS(UpgradeData, base4) },
	{ "mod4", ST_INT, FOFS(UpgradeData, mod4) },
	{ NULL },
};

struct UpgradeData *FindUpgradeData(DWORD upgradeid) {
	struct UpgradeData *lpValue = g_UpgradeData;
	for (; lpValue->upgradeid != upgradeid && lpValue->upgradeid; lpValue++);
	if (lpValue->upgradeid == 0) lpValue = NULL;
	return lpValue;
}

void InitUpgradeData(void) {
	g_UpgradeData = gi.ParseSheet("Units\\UpgradeData.slk", UpgradeData, sizeof(struct UpgradeData));
}
void ShutdownUpgradeData(void) {
	gi.MemFree(g_UpgradeData);
}
