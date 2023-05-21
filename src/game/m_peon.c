#include "g_local.h"

//void peon_die(LPEDICT self);
//void peon_decay2(LPEDICT self);
void peon_decay1(LPEDICT self);

//static mmove_t peon_move_decay2 = { "Decay Bone", NULL, peon_die };
//static mmove_t peon_move_decay1 = { "Decay Flesh", NULL, peon_decay2 };
static mmove_t peon_move_stand = { "Stand", ai_stand };
static mmove_t peon_move_walk = { "Walk", ai_walk };
static mmove_t peon_move_melee = { "Attack", ai_melee };
static mmove_t peon_move_death = { "Death", NULL, peon_decay1 };

//void peon_die(LPEDICT self) {
//}

//void peon_decay2(LPEDICT self) {
//    self->monsterinfo.currentmove = &peon_move_decay2;
//}

void peon_decay1(LPEDICT self) {
//    self->monsterinfo.currentmove = &peon_move_decay1;
    self->monsterinfo.aiflags |= AI_HOLD_FRAME;
}

void peon_walk(LPEDICT self) {
    self->monsterinfo.currentmove = &peon_move_walk;
}

void peon_stand(LPEDICT self) {
    self->monsterinfo.currentmove = &peon_move_stand;
}

void peon_melee(LPEDICT self) {
    self->monsterinfo.currentmove = &peon_move_melee;
}

void peon_die(LPEDICT self, LPEDICT attacker) {
    self->monsterinfo.currentmove = &peon_move_death;
}

void SP_monster_peon(LPEDICT self) {
    self->movetype = MOVETYPE_STEP;
    
    self->die = peon_die;

    self->monsterinfo.stand = peon_stand;
    self->monsterinfo.walk = peon_walk;
    self->monsterinfo.melee = peon_melee;
    self->monsterinfo.currentmove = &peon_move_stand;

    self->monsterinfo.health = 100;
    
    monster_start(self);
}
