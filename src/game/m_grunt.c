#include "g_local.h"

void grunt_decay1(LPEDICT self);

static mmove_t grunt_move_stand = { "Stand - 1", ai_stand };
static mmove_t grunt_move_walk = { "walk", ai_walk };
static mmove_t grunt_move_melee = { "Attack - 1", ai_melee };
static mmove_t grunt_move_death = { "Death", NULL, grunt_decay1 };

void grunt_decay1(LPEDICT self) {
//    self->monsterinfo.currentmove = &grunt_move_decay1;
    self->monsterinfo.aiflags |= AI_HOLD_FRAME;
}

void grunt_walk(LPEDICT self) {
    self->monsterinfo.currentmove = &grunt_move_walk;
}

void grunt_stand(LPEDICT self) {
    self->monsterinfo.currentmove = &grunt_move_stand;
}

void grunt_melee(LPEDICT self) {
    self->monsterinfo.currentmove = &grunt_move_melee;
}

void grunt_die(LPEDICT self, LPEDICT attacker) {
    self->monsterinfo.currentmove = &grunt_move_death;
}

void SP_monster_grunt(LPEDICT self) {
    self->movetype = MOVETYPE_STEP;
    
    self->die = grunt_die;
    
    self->monsterinfo.stand = grunt_stand;
    self->monsterinfo.walk = grunt_walk;
    self->monsterinfo.melee = grunt_melee;
    self->monsterinfo.currentmove = &grunt_move_stand;
    
    monster_start(self);
}
