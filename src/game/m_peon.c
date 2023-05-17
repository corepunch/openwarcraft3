#include "g_local.h"

static void peon_think(LPEDICT lpEdict, DWORD msec) {
    lpEdict->s.frame++;
}

void SP_monster_peon(LPEDICT lpEdict) {
//    lpEdict->monsterinfo.currentmove = gi.GetAnimation(
    lpEdict->think = peon_think;
}
