#include "g_local.h"

static void peon_think(struct edict *ent, int msec) {
    ent->s.frame++;
}

void SP_monster_peon(struct edict *ent) {
//    ent->monsterinfo.currentmove = gi.GetAnimation(
    ent->think = peon_think;
}
