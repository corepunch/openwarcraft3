#include "g_local.h"

#include "Units/UnitWeapons.h"

static void ai_runwait(edict_t *self, void (*callback)(edict_t *)) {
    if (self->unitinfo.wait == 0)
        return;
    if (self->unitinfo.wait > FRAMETIME) {
        self->unitinfo.wait -= FRAMETIME;
    } else {
        callback(self);
        self->unitinfo.wait = 0;
    }
}

static void ai_damagetarget(edict_t *self) {
    self->enemy->unitinfo.health -= self->unitinfo.weapon->avgdmg1;
    if (self->enemy->unitinfo.health < 0) {
        self->enemy->die(self->enemy, self);
        self->enemy = NULL;
        self->unitinfo.stand(self);
    }
    if (self->enemy->unitinfo.melee && !self->enemy->enemy) {
        self->enemy->enemy = self;
        self->enemy->unitinfo.melee(self->enemy);
        M_ChangeAngle(self->enemy);
    }
}

void ai_checkattack(edict_t *self) {
    self->unitinfo.checkattack(self);
}

void M_CheckWalk(edict_t *self) {
    if (self->goalentity && self->unitinfo.walk) {
        self->enemy = NULL;
        self->unitinfo.walk(self);
    }
}

void ai_melee(edict_t *self) {
//    M_ChangeAngle(self);
    ai_runwait(self, ai_damagetarget);
}

void ai_walk(edict_t *self) {
    M_ChangeAngle(self);
    M_MoveToGoal(self);
}

void ai_stand(edict_t *self) {
    if (self->goalentity && self->unitinfo.walk) {
        self->unitinfo.walk(self);
    }
}

void ai_cooldown(edict_t *self) {
    M_CheckWalk(self);
    if (!self->enemy)
        return;
    float distance  = Vector2_distance(&self->s.origin2, &self->enemy->s.origin2);
    if (distance < self->unitinfo.weapon->rangeN1) {
        ai_runwait(self, self->unitinfo.melee);
    } else {
        self->goalentity = self->enemy;
        self->unitinfo.walk(self);
    }
}
