#include "g_local.h"

//void unit_die(LPEDICT self);
//void unit_decay2(LPEDICT self);
void unit_decay1(LPEDICT self);
void unit_cooldown(LPEDICT self);
void unit_stand(LPEDICT self);

void ai_birth2(LPEDICT self) {
    M_RunWait(self, unit_stand);
}

//static mmove_t unit_move_decay2 = { "Decay Bone", NULL, unit_die };
//static mmove_t unit_move_decay1 = { "Decay Flesh", NULL, unit_decay2 };
static umove_t unit_move_birth = { "birth", ai_birth, unit_stand };
static umove_t unit_move_stand = { "stand", ai_stand, unit_stand };
static umove_t unit_move_death = { "death", NULL, unit_decay1 };

//void unit_die(LPEDICT self) {
//}

//void unit_decay2(LPEDICT self) {
//    self->monsterinfo.currentmove = &unit_move_decay2;
//}

void unit_decay1(LPEDICT self) {
//    self->monsterinfo.currentmove = &unit_move_decay1;
    self->aiflags |= AI_HOLD_FRAME;
}

void unit_stand(LPEDICT self) {
    M_SetMove(self, &unit_move_stand);
    self->build = NULL;
    self->s.renderfx &= ~RF_NO_UBERSPLAT;
    self->s.ability = 0;
    
}

void unit_die(LPEDICT self, LPEDICT attacker) {
    M_SetMove(self, &unit_move_death);
}

void unit_birth(LPEDICT self) {
    M_SetMove(self, &unit_move_birth);
    self->wait = UNIT_BUILD_TIME(self->class_id);
    self->s.renderfx |= RF_NO_UBERSPLAT;
}

void SP_monster_unit(LPEDICT self) {
    self->movetype = MOVETYPE_STEP;
    self->die = unit_die;
    self->stand = unit_stand;
    self->birth = unit_birth;
    
    M_SetMove(self, &unit_move_stand);
    
    monster_start(self);
}
