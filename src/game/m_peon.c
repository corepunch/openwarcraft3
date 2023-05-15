#include "g_local.h"

static void peon_think(struct edict *ent, int msec) {
    ent->s.frame++;
}

void SP_monster_peon(struct edict *ent) {
    ent->s.model = G_LoadModelDirFile("Units\\Orc", "Peon", 0);
    ent->think = peon_think;
}
