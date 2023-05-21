#include "../g_local.h"
#include "UnitAbilities.h"

static LPUNITABILITIES g_UnitAbilities = NULL;

static struct SheetLayout UnitAbilities[] = {
	{ "unitAbilID", ST_ID, FOFS(UnitAbilities, unitAbilID) },
	{ "sortAbil", ST_ID, FOFS(UnitAbilities, sortAbil) },
	{ "comment(s)", ST_STRING, FOFS(UnitAbilities, comment) },
	{ "auto", ST_ID, FOFS(UnitAbilities, auto_) },
	{ "abilList", ST_STRING, FOFS(UnitAbilities, abilList) },
	{ "heroAbilList", ST_STRING, FOFS(UnitAbilities, heroAbilList) },
	{ "InBeta", ST_INT, FOFS(UnitAbilities, InBeta) },
	{ NULL },
};

LPCUNITABILITIES FindUnitAbilities(DWORD unitAbilID) {
	struct UnitAbilities *lpValue = g_UnitAbilities;
	for (; lpValue->unitAbilID != unitAbilID && lpValue->unitAbilID; lpValue++);
	if (lpValue->unitAbilID == 0) lpValue = NULL;
	return lpValue;
}

void InitUnitAbilities(void) {
	g_UnitAbilities = gi.ParseSheet("Units\\UnitAbilities.slk", UnitAbilities, sizeof(struct UnitAbilities));
}
void ShutdownUnitAbilities(void) {
	gi.MemFree(g_UnitAbilities);
}
