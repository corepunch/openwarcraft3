#include "g_local.h"

void ai_walk(LPEDICT self);
void ai_stand(LPEDICT self);

static mmove_t peon_move_stand = { "Stand", ai_stand };
static mmove_t peon_move_walk = { "Walk", ai_walk };

void ai_walk(LPEDICT self) {
    const float DISTANCE = 0.5 * frametime;
    const float TARGET = Vector2_distance((LPVECTOR2)&self->s.origin, &self->objective);
    if (TARGET < DISTANCE) {
        self->monsterinfo.stand(self);
        self->objective = (VECTOR2) {0,0};
        return;
    }
    VECTOR2 dir = {
        self->s.origin.x - self->objective.x,
        self->s.origin.y - self->objective.y,
    };
    Vector2_normalize(&dir);
    self->s.angle = atan2(-dir.y, -dir.x);
    self->s.origin.x += cos(self->s.angle) * DISTANCE;
    self->s.origin.y += sin(self->s.angle) * DISTANCE;
}

void ai_stand(LPEDICT self) {
    if (self->objective.x != 0) {
        self->monsterinfo.walk(self);
    }
}

void peon_walk(LPEDICT self) {
    self->monsterinfo.currentmove = &peon_move_walk;
}

void peon_stand(LPEDICT self) {
    self->monsterinfo.currentmove = &peon_move_stand;
}

void SP_monster_peon(LPEDICT self) {
    self->monsterinfo.stand = peon_stand;
    self->monsterinfo.walk = peon_walk;
    self->monsterinfo.currentmove = &peon_move_stand;
    
    monster_start(self);
}
