#include "g_local.h"

enum {
    kDestructableData_DestructableID = 1,
    kDestructableData_category,
    kDestructableData_tilesets,
    kDestructableData_tilesetSpecific,
    kDestructableData_dir,
    kDestructableData_file,
    kDestructableData_lightweight,
    kDestructableData_fatLOS,
    kDestructableData_texID,
    kDestructableData_texFile,
    kDestructableData_comment,
    kDestructableData_name,
    kDestructableData_doodClass,
    kDestructableData_useClickHelper,
    kDestructableData_onCliffs,
    kDestructableData_onWater,
    kDestructableData_canPlaceDead,
    kDestructableData_walkable,
    kDestructableData_cliffHeight,
    kDestructableData_targType,
    kDestructableData_armor,
    kDestructableData_numVar,
    kDestructableData_HP,
    kDestructableData_occH,
    kDestructableData_flyH,
    kDestructableData_fixedRot,
    kDestructableData_selSize,
    kDestructableData_minScale,
    kDestructableData_maxScale,
    kDestructableData_canPlaceRandScale,
    kDestructableData_maxPitch,
    kDestructableData_maxRoll,
    kDestructableData_radius,
    kDestructableData_fogRadius,
    kDestructableData_fogVis,
    kDestructableData_pathTex,
    kDestructableData_pathTexDeath,
    kDestructableData_deathSnd,
    kDestructableData_shadow,
    kDestructableData_showInMM,
    kDestructableData_useMMColor,
    kDestructableData_MMRed,
    kDestructableData_MMGreen,
    kDestructableData_MMBlue,
    kDestructableData_InBeta,
};

static struct SheetLayout destructable_data[] = {
    { kDestructableData_DestructableID, ST_ID, FOFS(DestructableData, DestructableID) },
    { kDestructableData_category, ST_STRING, FOFS(DestructableData, category) },
    { kDestructableData_tilesets, ST_STRING, FOFS(DestructableData, tilesets) },
    { kDestructableData_tilesetSpecific, ST_STRING, FOFS(DestructableData, tilesetSpecific) },
    { kDestructableData_dir, ST_STRING, FOFS(DestructableData, dir) },
    { kDestructableData_file, ST_STRING, FOFS(DestructableData, file) },
    { kDestructableData_lightweight, ST_INT, FOFS(DestructableData, lightweight) },
    { kDestructableData_fatLOS, ST_INT, FOFS(DestructableData, fatLOS) },
    { kDestructableData_texID, ST_INT, FOFS(DestructableData, texID) },
    { kDestructableData_texFile, ST_STRING, FOFS(DestructableData, texFile) },
    { 0 }
};

void G_InitDestructables(void) {
    game.DestructableData = FS_ParseSheet("Units\\DestructableData.slk", destructable_data, sizeof(struct DestructableData), FOFS(DestructableData, lpNext));
}

struct DestructableData *G_FindDestructableData(int DestructableID) {
    FOR_EACH_LIST(struct DestructableData, info, game.DestructableData) {
        if (info->DestructableID == DestructableID)
            return info;
    }
    return NULL;
}

