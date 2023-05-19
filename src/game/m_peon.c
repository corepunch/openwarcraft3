#include "g_local.h"

void ai_walk(LPEDICT self);
void ai_stand(LPEDICT self);

static mmove_t peon_move_stand = { "Stand", ai_stand };
static mmove_t peon_move_walk = { "Walk", ai_walk };

void ai_walk(LPEDICT self) {
    const float DISTANCE = 0.5 * frametime;
    VECTOR2 dir = Vector2_sub((LPCVECTOR2)&self->s.origin, &self->monsterinfo.goal);
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
    if (self->monsterinfo.aiflags & AI_HAS_GOAL) {
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
