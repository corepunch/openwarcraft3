#include "g_local.h"
//
//enum {
//    kDestructableData_DestructableID = 1,
//    kDestructableData_category,
//    kDestructableData_tilesets,
//    kDestructableData_tilesetSpecific,
//    kDestructableData_dir,
//    kDestructableData_file,
//    kDestructableData_lightweight,
//    kDestructableData_fatLOS,
//    kDestructableData_texID,
//    kDestructableData_texFile,
//    kDestructableData_comment,
//    kDestructableData_name,
//    kDestructableData_doodClass,
//    kDestructableData_useClickHelper,
//    kDestructableData_onCliffs,
//    kDestructableData_onWater,
//    kDestructableData_canPlaceDead,
//    kDestructableData_walkable,
//    kDestructableData_cliffHeight,
//    kDestructableData_targType,
//    kDestructableData_armor,
//    kDestructableData_numVar,
//    kDestructableData_HP,
//    kDestructableData_occH,
//    kDestructableData_flyH,
//    kDestructableData_fixedRot,
//    kDestructableData_selSize,
//    kDestructableData_minScale,
//    kDestructableData_maxScale,
//    kDestructableData_canPlaceRandScale,
//    kDestructableData_maxPitch,
//    kDestructableData_maxRoll,
//    kDestructableData_radius,
//    kDestructableData_fogRadius,
//    kDestructableData_fogVis,
//    kDestructableData_pathTex,
//    kDestructableData_pathTexDeath,
//    kDestructableData_deathSnd,
//    kDestructableData_shadow,
//    kDestructableData_showInMM,
//    kDestructableData_useMMColor,
//    kDestructableData_MMRed,
//    kDestructableData_MMGreen,
//    kDestructableData_MMBlue,
//    kDestructableData_InBeta,
//};

static struct SheetLayout destructable_data[] = {
    { "DestructableID", ST_ID, FOFS(DestructableData, DestructableID) },
    { "category", ST_STRING, FOFS(DestructableData, category) },
    { "tilesets", ST_STRING, FOFS(DestructableData, tilesets) },
    { "tilesetSpecific", ST_STRING, FOFS(DestructableData, tilesetSpecific) },
    { "dir", ST_STRING, FOFS(DestructableData, dir) },
    { "file", ST_STRING, FOFS(DestructableData, file) },
    { "lightweight", ST_INT, FOFS(DestructableData, lightweight) },
    { "fatLOS", ST_INT, FOFS(DestructableData, fatLOS) },
    { "texID", ST_INT, FOFS(DestructableData, texID) },
    { "texFile", ST_STRING, FOFS(DestructableData, texFile) },
    { NULL }
};

void G_InitDestructables(void) {
    game.DestructableData = gi.ParseSheet("Units\\DestructableData.slk", destructable_data, sizeof(struct DestructableData));
}

