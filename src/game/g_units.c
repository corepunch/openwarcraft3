#include "g_local.h"

struct SheetLayout unit_info[] = {
    { 1, ST_ID, FOFS(UnitData, unitID) },
    { 2, ST_STRING, FOFS(UnitData, sort) },
    { 3, ST_STRING, FOFS(UnitData, comment) },
    { 4, ST_STRING, FOFS(UnitData, race) },
    { 5, ST_INT, FOFS(UnitData, prio) },
    { 6, ST_INT, FOFS(UnitData, threat) },
    { 7, ST_INT, FOFS(UnitData, type) },
    { 8, ST_INT, FOFS(UnitData, valid) },
    { 9, ST_INT, FOFS(UnitData, deathType) },
    { 10, ST_FLOAT, FOFS(UnitData, death) },
    { 11, ST_INT, FOFS(UnitData, canSleep) },
    { 12, ST_INT, FOFS(UnitData, cargoSize) },
    { 13, ST_STRING, FOFS(UnitData, movetp) },
    { 14, ST_INT, FOFS(UnitData, moveHeight) },
    { 15, ST_INT, FOFS(UnitData, moveFloor) },
    { 16, ST_INT, FOFS(UnitData, launchX) },
    { 17, ST_INT, FOFS(UnitData, launchY) },
    { 18, ST_INT, FOFS(UnitData, launchZ) },
    { 19, ST_INT, FOFS(UnitData, impactZ) },
    { 20, ST_FLOAT, FOFS(UnitData, turnRate) },
    { 21, ST_INT, FOFS(UnitData, propWin) },
    { 22, ST_INT, FOFS(UnitData, orientInterp) },
    { 23, ST_INT, FOFS(UnitData, formation) },
    { 24, ST_FLOAT, FOFS(UnitData, castpt) },
    { 25, ST_FLOAT, FOFS(UnitData, castbsw) },
    { 26, ST_STRING, FOFS(UnitData, targType) },
    { 27, ST_STRING, FOFS(UnitData, pathTex) },
    { 28, ST_INT, FOFS(UnitData, fatLOS) },
    { 29, ST_INT, FOFS(UnitData, collision) },
    { 30, ST_INT, FOFS(UnitData, points) },
    { 31, ST_INT, FOFS(UnitData, buffType) },
    { 32, ST_INT, FOFS(UnitData, buffRadius) },
    { 33, ST_INT, FOFS(UnitData, nameCount) },
    { 34, ST_INT, FOFS(UnitData, InBeta) },
    { 0 }
};

struct SheetLayout unit_ui_info[] = {
    { 1, ST_ID, FOFS(UnitUI, unitUIID) },
    { 2, ST_STRING, FOFS(UnitUI, file) },
    { 3, ST_STRING, FOFS(UnitUI, unitSound) },
    { 4, ST_STRING, FOFS(UnitUI, tilesets) },
    { 5, ST_INT, FOFS(UnitUI, tilesetSpecific) },
    { 6, ST_STRING, FOFS(UnitUI, name) },
    { 7, ST_STRING, FOFS(UnitUI, unitClass) },
    { 8, ST_INT, FOFS(UnitUI, special) },
    { 9, ST_INT, FOFS(UnitUI, inEditor) },
    { 10, ST_INT, FOFS(UnitUI, hiddenInEditor) },
    { 11, ST_INT, FOFS(UnitUI, hostilePal) },
    { 12, ST_INT, FOFS(UnitUI, dropItems) },
    { 13, ST_INT, FOFS(UnitUI, nbrandom) },
    { 14, ST_INT, FOFS(UnitUI, nbmmIcon) },
    { 15, ST_INT, FOFS(UnitUI, useClickHelper) },
    { 16, ST_FLOAT, FOFS(UnitUI, blend) },
    { 17, ST_FLOAT, FOFS(UnitUI, scale) },
    { 18, ST_INT, FOFS(UnitUI, scaleBull) },
    { 19, ST_STRING, FOFS(UnitUI, preventPlace) },
    { 20, ST_INT, FOFS(UnitUI, requirePlace) },
    { 21, ST_INT, FOFS(UnitUI, isbldg) },
    { 22, ST_INT, FOFS(UnitUI, maxPitch) },
    { 23, ST_INT, FOFS(UnitUI, maxRoll) },
    { 24, ST_INT, FOFS(UnitUI, elevPts) },
    { 25, ST_INT, FOFS(UnitUI, elevRad) },
    { 26, ST_INT, FOFS(UnitUI, fogRad) },
    { 27, ST_INT, FOFS(UnitUI, walk) },
    { 28, ST_INT, FOFS(UnitUI, run) },
    { 29, ST_INT, FOFS(UnitUI, selZ) },
    { 30, ST_INT, FOFS(UnitUI, weap1) },
    { 31, ST_INT, FOFS(UnitUI, weap2) },
    { 32, ST_INT, FOFS(UnitUI, teamColor) },
    { 33, ST_INT, FOFS(UnitUI, customTeamColor) },
    { 34, ST_STRING, FOFS(UnitUI, armor) },
    { 35, ST_INT, FOFS(UnitUI, modelScale) },
    { 36, ST_INT, FOFS(UnitUI, red) },
    { 37, ST_INT, FOFS(UnitUI, green) },
    { 38, ST_INT, FOFS(UnitUI, blue) },
    { 39, ST_STRING, FOFS(UnitUI, uberSplat) },
    { 40, ST_STRING, FOFS(UnitUI, unitShadow) },
    { 41, ST_STRING, FOFS(UnitUI, buildingShadow) },
    { 42, ST_INT, FOFS(UnitUI, shadowW) },
    { 43, ST_INT, FOFS(UnitUI, shadowH) },
    { 44, ST_INT, FOFS(UnitUI, shadowX) },
    { 45, ST_INT, FOFS(UnitUI, shadowY) },
    { 46, ST_INT, FOFS(UnitUI, occH) },
    { 47, ST_INT, FOFS(UnitUI, InBeta) },
    { 0 }
};

void G_InitUnits(void) {
    game.UnitData = FS_ParseSheet("Units\\UnitData.slk", unit_info, sizeof(struct UnitData), FOFS(UnitData, lpNext));
    game.UnitUI = FS_ParseSheet("Units\\unitUI.slk", unit_info, sizeof(struct UnitUI), FOFS(UnitUI, lpNext));
}
