#include "g_local.h"

//void peon_die(LPEDICT self);
//void peon_decay2(LPEDICT self);
void peon_decay1(LPEDICT self);

//static mmove_t peon_move_decay2 = { "Decay Bone", NULL, peon_die };
//static mmove_t peon_move_decay1 = { "Decay Flesh", NULL, peon_decay2 };
static mmove_t peon_move_stand = { ANIM_STAND, ai_stand };
static mmove_t peon_move_walk = { ANIM_WALK, ai_walk };
static mmove_t peon_move_melee = { ANIM_ATTACK, ai_melee };
static mmove_t peon_move_death = { ANIM_DEATH, NULL, peon_decay1 };

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
    
    monster_start(self);
}
