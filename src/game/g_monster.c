#include "g_local.h"
#include <stdlib.h>

void M_MoveFrame(LPEDICT self) {
    mmove_t const *move = self->monsterinfo.currentmove;
    ANIMATION anim = gi.GetAnimation(self->s.model, move->animation);
    if (self->s.frame < anim.firstframe || self->s.frame >= anim.lastframe) {
        self->s.frame = anim.firstframe;
    } else {
        self->s.frame += FRAMETIME;
        while (self->s.frame >= anim.lastframe) {
            self->s.frame -= MAX(1, anim.lastframe - anim.firstframe);
        }
    }
}

void monster_think(LPEDICT self) {
    M_MoveFrame(self);
    if (self->monsterinfo.currentmove) {
        self->monsterinfo.currentmove->think(self);
    }
    self->s.origin.z = gi.GetHeightAtPoint(self->s.origin.x, self->s.origin.y);
}

void monster_start(LPEDICT self) {
    if (self->monsterinfo.currentmove) {
        ANIMATION anim = gi.GetAnimation(self->s.model, self->monsterinfo.currentmove->animation);
        self->s.frame = anim.firstframe + (rand() % (anim.lastframe - anim.firstframe + 1));
    }
}

void SP_monster_peon(LPEDICT self);

void SP_SpawnUnit(LPEDICT self, LPCUNITUI lpUnit) {
    PATHSTR buffer;
    sprintf(buffer, "%s.mdx", lpUnit->file);
    self->s.model = gi.ModelIndex(buffer);
    self->think = monster_think;
    SP_monster_peon(self);
}
