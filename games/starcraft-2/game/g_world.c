#include "g_sc2_local.h"

#define GAME_WORLD 1
#include "common/world.c"
#include "games/warcraft-3/common/world_w3.c"
#include "common/routing.c"

#undef FS_ReadFile
#undef FS_FreeFile
#undef MemAlloc
#undef MemFree
#undef PF_TextRemoveBom
#undef Com_Error
#undef ge
#undef GAME_WORLD
