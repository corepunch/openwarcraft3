#include "g_wow_local.h"

#define GAME_WORLD 1
#include "common/world.c"
#include "common/world_wow.c"

#undef FS_ReadFile
#undef FS_FreeFile
#undef MemAlloc
#undef MemFree
#undef PF_TextRemoveBom
#undef Com_Error
#undef GAME_WORLD
