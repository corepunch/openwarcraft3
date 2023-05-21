#include "../g_local.h"
#include "UnitUI.h"

static struct UnitUI *g_UnitUI = NULL;

static struct SheetLayout UnitUI[] = {
  { "unitUIID", ST_ID, FOFS(UnitUI, unitUIID) },
  { "file", ST_STRING, FOFS(UnitUI, file) },
  { "unitSound", ST_STRING, FOFS(UnitUI, unitSound) },
  { "tilesets", ST_STRING, FOFS(UnitUI, tilesets) },
  { "tilesetSpecific", ST_INT, FOFS(UnitUI, tilesetSpecific) },
  { "name", ST_STRING, FOFS(UnitUI, name) },
  { "unitClass", ST_STRING, FOFS(UnitUI, unitClass) },
  { "special", ST_INT, FOFS(UnitUI, special) },
  { "inEditor", ST_INT, FOFS(UnitUI, inEditor) },
  { "hiddenInEditor", ST_INT, FOFS(UnitUI, hiddenInEditor) },
  { "hostilePal", ST_INT, FOFS(UnitUI, hostilePal) },
  { "dropItems", ST_INT, FOFS(UnitUI, dropItems) },
  { "nbrandom", ST_INT, FOFS(UnitUI, nbrandom) },
  { "nbmmIcon", ST_INT, FOFS(UnitUI, nbmmIcon) },
  { "useClickHelper", ST_INT, FOFS(UnitUI, useClickHelper) },
  { "blend", ST_FLOAT, FOFS(UnitUI, blend) },
  { "scale", ST_FLOAT, FOFS(UnitUI, scale) },
  { "scaleBull", ST_INT, FOFS(UnitUI, scaleBull) },
  { "preventPlace", ST_STRING, FOFS(UnitUI, preventPlace) },
  { "requirePlace", ST_STRING, FOFS(UnitUI, requirePlace) },
  { "isbldg", ST_INT, FOFS(UnitUI, isbldg) },
  { "maxPitch", ST_INT, FOFS(UnitUI, maxPitch) },
  { "maxRoll", ST_INT, FOFS(UnitUI, maxRoll) },
  { "elevPts", ST_INT, FOFS(UnitUI, elevPts) },
  { "elevRad", ST_INT, FOFS(UnitUI, elevRad) },
  { "fogRad", ST_INT, FOFS(UnitUI, fogRad) },
  { "walk", ST_INT, FOFS(UnitUI, walk) },
  { "run", ST_INT, FOFS(UnitUI, run) },
  { "selZ", ST_INT, FOFS(UnitUI, selZ) },
  { "weap1", ST_STRING, FOFS(UnitUI, weap1) },
  { "weap2", ST_STRING, FOFS(UnitUI, weap2) },
  { "teamColor", ST_INT, FOFS(UnitUI, teamColor) },
  { "customTeamColor", ST_INT, FOFS(UnitUI, customTeamColor) },
  { "armor", ST_STRING, FOFS(UnitUI, armor) },
  { "modelScale", ST_FLOAT, FOFS(UnitUI, modelScale) },
  { "red", ST_INT, FOFS(UnitUI, red) },
  { "green", ST_INT, FOFS(UnitUI, green) },
  { "blue", ST_INT, FOFS(UnitUI, blue) },
  { "uberSplat", ST_ID, FOFS(UnitUI, uberSplat) },
  { "unitShadow", ST_STRING, FOFS(UnitUI, unitShadow) },
  { "buildingShadow", ST_STRING, FOFS(UnitUI, buildingShadow) },
  { "shadowW", ST_INT, FOFS(UnitUI, shadowW) },
  { "shadowH", ST_INT, FOFS(UnitUI, shadowH) },
  { "shadowX", ST_INT, FOFS(UnitUI, shadowX) },
  { "shadowY", ST_INT, FOFS(UnitUI, shadowY) },
  { "occH", ST_INT, FOFS(UnitUI, occH) },
  { "InBeta", ST_INT, FOFS(UnitUI, InBeta) },
  { NULL },
};

struct UnitUI *FindUnitUI(DWORD unitUIID) {
  struct UnitUI *lpValue = g_UnitUI;
  for (; lpValue->unitUIID != unitUIID && lpValue->unitUIID; lpValue++);
  if (lpValue->unitUIID == 0) lpValue = NULL;
  return lpValue;
}

void InitUnitUI(void) {
  g_UnitUI = gi.ParseSheet("Units\\unitUI.slk", UnitUI, sizeof(struct UnitUI));
}
void ShutdownUnitUI(void) {
  gi.MemFree(g_UnitUI);
}
