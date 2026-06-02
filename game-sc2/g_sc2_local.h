#ifndef G_SC2_LOCAL_H
#define G_SC2_LOCAL_H

#include "../common/common.h"
#include "../server/server.h"

#define SC2_MAX_CLIENTS 1
#define SC2_MAX_EDICTS  128

extern struct game_import gi;
extern struct game_export globals;

LPCANIMATION G_GetAnimation(DWORD modelindex, LPCSTR animname);
void         G_FreeModels(void);

#endif
