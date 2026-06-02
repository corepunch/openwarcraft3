#define GAME_WORLD 1

#include "g_wow_local.h"

#include "../common/world.c"
#include "../common/world_wow.c"

#undef FS_ReadFile
#undef FS_FreeFile
#undef MemAlloc
#undef MemFree
#undef PF_TextRemoveBom
#undef Com_Error

void G_BakeStaticObstacles(void) {
}

DWORD G_BuildHeatmap(LPEDICT goalentity) {
    (void)goalentity;
    return 0;
}

BOOL G_ClosestPathablePointForRadius(LPCVECTOR2 location, FLOAT radius, LPVECTOR2 out) {
    (void)radius;
    if (!location || !out)
        return false;
    *out = *location;
    return true;
}

VECTOR2 G_GetFlowDirection(DWORD heatmapindex, FLOAT fx, FLOAT fy) {
    (void)heatmapindex;
    (void)fx;
    (void)fy;
    return (VECTOR2){ 0.0f, 0.0f };
}
