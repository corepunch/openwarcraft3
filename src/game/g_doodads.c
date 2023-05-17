#include "g_local.h"

//enum {
//    kDoodadInfo_doodID = 1,
//    kDoodadInfo_category,
//    kDoodadInfo_tilesets,
//    kDoodadInfo_tilesetSpecific,
//    kDoodadInfo_dir,
//    kDoodadInfo_file,
//    kDoodadInfo_comment,
//    kDoodadInfo_name,
//    kDoodadInfo_doodClass,
//    kDoodadInfo_soundLoop,
//    kDoodadInfo_selSize,
//    kDoodadInfo_defScale,
//    kDoodadInfo_minScale,
//    kDoodadInfo_maxScale,
//    kDoodadInfo_canPlaceRandScale,
//    kDoodadInfo_useClickHelper,
//    kDoodadInfo_maxPitch,
//    kDoodadInfo_maxRoll,
//    kDoodadInfo_visRadius,
//    kDoodadInfo_walkable,
//    kDoodadInfo_numVar,
//    kDoodadInfo_onCliffs,
//    kDoodadInfo_onWater,
//    kDoodadInfo_floats,
//    kDoodadInfo_shadow,
//    kDoodadInfo_showInFog,
//    kDoodadInfo_animInFog,
//    kDoodadInfo_fixedRot,
//    kDoodadInfo_pathTex,
//    kDoodadInfo_showInMM,
//    kDoodadInfo_useMMColor,
//    kDoodadInfo_MMRed,
//    kDoodadInfo_MMGreen,
//    kDoodadInfo_MMBlue,
//};

struct SheetLayout doodad_info[] = {
    { "doodID", ST_ID, FOFS(DoodadInfo, doodID) },
    { "dir", ST_STRING, FOFS(DoodadInfo, dir) },
    { "file", ST_STRING, FOFS(DoodadInfo, file) },
    { "pathTex", ST_STRING, FOFS(DoodadInfo, pathTex) },
    { NULL }
};

void G_InitDoodads(void) {
    game.Doodads = FS_ParseSheet("Doodads\\Doodads.slk", doodad_info, sizeof(struct DoodadInfo), FOFS(DoodadInfo, lpNext));
}

LPDOODADINFO G_FindDoodadInfo(int doodID) {
    FOR_EACH_LIST(struct DoodadInfo, info, game.Doodads) {
        if (info->doodID == doodID)
            return info;
    }
    return NULL;
}

