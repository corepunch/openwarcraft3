#include "g_local.h"

void G_FreeEdict(LPEDICT ent) {
    memset(ent, 0, sizeof(*ent));
    ent->freetime = level.time;
}
