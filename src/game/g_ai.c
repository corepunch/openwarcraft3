#include "g_local.h"

#include "Units/UnitWeapons.h"

static void ai_runtimer(LPEDICT self, void (*callback)(LPEDICT)) {
    if (self->unitinfo.timer == 0)
        return;
    if (self->unitinfo.timer > FRAMETIME) {
        self->unitinfo.timer -= FRAMETIME;
    } else {
        callback(self);
        self->unitinfo.timer = 0;
    }
}

static void ai_damagetarget(LPEDICT self) {
    self->enemy->unitinfo.health -= self->unitinfo.weapon->avgdmg1;
    if (self->enemy->unitinfo.health < 0) {
        self->enemy->die(self->enemy, self);
        self->enemy = NULL;
        self->unitinfo.stand(self);
    }
    if (self->enemy->unitinfo.melee && !self->enemy->enemy) {
        self->enemy->enemy = self;
        self->enemy->goalentity = self;
        self->enemy->unitinfo.melee(self->enemy);
    }
}

void ai_checkattack(LPEDICT self) {
    self->unitinfo.checkattack(self);
}

void ai_melee(LPEDICT self) {
    ai_runtimer(self, ai_damagetarget);
}

void ai_walk(LPEDICT self) {
    M_ChangeAngle(self);
    M_MoveToGoal(self);
}

void ai_stand(LPEDICT self) {
//    if (self->goalentity && self->monsterinfo.walk) {
//        self->monsterinfo.walk(self);
//    }
    if (self->goalentity && self->unitinfo.walk) {
        self->unitinfo.walk(self);
    }
}

void ai_cooldown(LPEDICT self) {
    ai_runtimer(self, self->unitinfo.melee);
}
