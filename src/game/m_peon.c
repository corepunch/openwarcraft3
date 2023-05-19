#include "g_local.h"

LPEDICT monster_target(LPCEDICT self) {
    return &game_state.edicts[self->monsterinfo.target];
}

void ai_attack(LPEDICT self) {
    LPEDICT target = monster_target(self);
    target->monsterinfo.health -= 3;
    if (target->monsterinfo.health < 0) {
        target->monsterinfo.death(target);
        self->monsterinfo.aiflags &= ~AI_HAS_TARGET;
        self->monsterinfo.stand(self);
    }
}

void ai_walk(LPEDICT self) {
    const float DISTANCE = 0.5 * FRAMETIME;
    const float ATTACK_DISTANCE = 200;
    if (self->monsterinfo.aiflags & AI_HAS_TARGET) {
        self->monsterinfo.goal.x = monster_target(self)->s.origin.x;
        self->monsterinfo.goal.y = monster_target(self)->s.origin.y;
    }
    VECTOR2 dir = Vector2_sub((LPCVECTOR2)&self->s.origin, &self->monsterinfo.goal);
    if (self->monsterinfo.aiflags & AI_HAS_TARGET && Vector2_len(&dir) < ATTACK_DISTANCE) {
        self->monsterinfo.attack(self);
        return;
    }
    if (Vector2_len(&dir) < DISTANCE) {
        self->monsterinfo.stand(self);
        self->monsterinfo.aiflags &= ~AI_HAS_GOAL;
        return;
    }
    Vector2_normalize(&dir);
    self->s.angle = atan2(-dir.y, -dir.x);
    self->s.origin.x += cos(self->s.angle) * DISTANCE;
    self->s.origin.y += sin(self->s.angle) * DISTANCE;
}

void ai_stand(LPEDICT self) {
    if (self->monsterinfo.aiflags & (AI_HAS_GOAL | AI_HAS_TARGET)) {
        self->monsterinfo.walk(self);
    }
}

void peon_die(LPEDICT self) {
    
}

static mmove_t peon_move_decay2 = { "Decay Bone", NULL, peon_die };

void peon_decay2(LPEDICT self) {
    self->monsterinfo.currentmove = &peon_move_decay2;
}

static mmove_t peon_move_decay1 = { "Decay Flesh", NULL, peon_decay2 };

void peon_decay1(LPEDICT self) {
//    self->monsterinfo.currentmove = &peon_move_decay1;
    self->monsterinfo.aiflags |= AI_HOLD_FRAME;
}

static mmove_t peon_move_stand = { "Stand", ai_stand };
static mmove_t peon_move_walk = { "Walk", ai_walk };
static mmove_t peon_move_attack = { "Attack", ai_attack };
static mmove_t peon_move_death = { "Death", NULL, peon_decay1 };

void peon_walk(LPEDICT self) {
    self->monsterinfo.currentmove = &peon_move_walk;
}

void peon_stand(LPEDICT self) {
    self->monsterinfo.currentmove = &peon_move_stand;
}

void peon_attack(LPEDICT self) {
    self->monsterinfo.currentmove = &peon_move_attack;
}

void peon_death(LPEDICT self) {
    self->monsterinfo.currentmove = &peon_move_death;
}

void SP_monster_peon(LPEDICT self) {
    self->monsterinfo.stand = peon_stand;
    self->monsterinfo.walk = peon_walk;
    self->monsterinfo.attack = peon_attack;
    self->monsterinfo.death = peon_death;
    self->monsterinfo.currentmove = &peon_move_stand;
    self->monsterinfo.health = 100;
    
    monster_start(self);
}
