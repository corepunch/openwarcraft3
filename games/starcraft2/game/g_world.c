#define GAME_WORLD 1

#include "g_sc2_local.h"

#include "common/world.c"
#include "common/world_w3.c"
#include "common/routing.c"

#undef FS_ReadFile
#undef FS_FreeFile
#undef MemAlloc
#undef MemFree
#undef PF_TextRemoveBom
#undef Com_Error
#undef ge
