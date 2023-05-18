#include "g_local.h"

struct SheetLayout unit_info[] = {
    { "unitID", ST_ID, FOFS(UnitData, unitID) },
    { "sort", ST_STRING, FOFS(UnitData, sort) },
    { "comment", ST_STRING, FOFS(UnitData, comment) },
    { "race", ST_STRING, FOFS(UnitData, race) },
    { "prio", ST_INT, FOFS(UnitData, prio) },
    { "threat", ST_INT, FOFS(UnitData, threat) },
    { "type", ST_INT, FOFS(UnitData, type) },
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
    { "buffType", ST_INT, FOFS(UnitData, buffType) },
    { "buffRadius", ST_INT, FOFS(UnitData, buffRadius) },
    { "nameCount", ST_INT, FOFS(UnitData, nameCount) },
    { "InBeta", ST_INT, FOFS(UnitData, InBeta) },
    { NULL }
};

struct SheetLayout unit_ui_info[] = {
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
    { "requirePlace", ST_INT, FOFS(UnitUI, requirePlace) },
    { "isbldg", ST_INT, FOFS(UnitUI, isbldg) },
    { "maxPitch", ST_INT, FOFS(UnitUI, maxPitch) },
    { "maxRoll", ST_INT, FOFS(UnitUI, maxRoll) },
    { "elevPts", ST_INT, FOFS(UnitUI, elevPts) },
    { "elevRad", ST_INT, FOFS(UnitUI, elevRad) },
    { "fogRad", ST_INT, FOFS(UnitUI, fogRad) },
    { "walk", ST_INT, FOFS(UnitUI, walk) },
    { "run", ST_INT, FOFS(UnitUI, run) },
    { "selZ", ST_INT, FOFS(UnitUI, selZ) },
    { "weap1", ST_INT, FOFS(UnitUI, weap1) },
    { "weap2", ST_INT, FOFS(UnitUI, weap2) },
    { "teamColor", ST_INT, FOFS(UnitUI, teamColor) },
    { "customTeamColor", ST_INT, FOFS(UnitUI, customTeamColor) },
    { "armor", ST_STRING, FOFS(UnitUI, armor) },
    { "modelScale", ST_INT, FOFS(UnitUI, modelScale) },
    { "red", ST_INT, FOFS(UnitUI, red) },
    { "green", ST_INT, FOFS(UnitUI, green) },
    { "blue", ST_INT, FOFS(UnitUI, blue) },
    { "uberSplat", ST_STRING, FOFS(UnitUI, uberSplat) },
    { "unitShadow", ST_STRING, FOFS(UnitUI, unitShadow) },
    { "buildingShadow", ST_STRING, FOFS(UnitUI, buildingShadow) },
    { "shadowW", ST_INT, FOFS(UnitUI, shadowW) },
    { "shadowH", ST_INT, FOFS(UnitUI, shadowH) },
    { "shadowX", ST_INT, FOFS(UnitUI, shadowX) },
    { "shadowY", ST_INT, FOFS(UnitUI, shadowY) },
    { "occH", ST_INT, FOFS(UnitUI, occH) },
    { "InBeta", ST_INT, FOFS(UnitUI, InBeta) },
    { NULL }
};

void G_InitUnits(void) {
    game.UnitData = gi.ParseSheet("Units\\UnitData.slk", unit_info, sizeof(struct UnitData), FOFS(UnitData, lpNext));
    game.UnitUI = gi.ParseSheet("Units\\unitUI.slk", unit_ui_info, sizeof(struct UnitUI), FOFS(UnitUI, lpNext));

//    FOR_EACH_LIST(struct UnitUI, ud, game.UnitUI) {
//        printf("%.4s %s\n", (char*)&ud->unitUIID, ud->file);
//    }
}

LPUNITUI G_FindUnitUI(int unitUIID) {
    FOR_EACH_LIST(struct UnitUI, info, game.UnitUI) {
        if (info->unitUIID == unitUIID)
            return info;
    }
    return NULL;
}

