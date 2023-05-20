#include "g_local.h"

void ai_checkattack(LPEDICT self) {
    self->monsterinfo.checkattack(self);
}

void ai_attack(LPEDICT self) {
    self->enemy->monsterinfo.health -= 3;
    if (self->enemy->monsterinfo.health < 0) {
        self->enemy->die(self->enemy, self);
        self->enemy = NULL;
        self->monsterinfo.stand(self);
    }
}

void ai_walk(LPEDICT self) {
    M_ChangeAngle(self);
    M_MoveToGoal(self);
}

void ai_stand(LPEDICT self) {
    if (self->goalentity) {
        self->monsterinfo.walk(self);
    }
}
