#include "g_local.h"

//void unit_die(edict_t *self);
//void unit_decay2(edict_t *self);
void unit_decay1(edict_t *self);
void unit_cooldown(edict_t *self);
void unit_stand(edict_t *self);

void ai_birth2(edict_t *self) {
    M_RunWait(self, unit_stand);
}

//static mmove_t unit_move_decay2 = { "Decay Bone", NULL, unit_die };
//static mmove_t unit_move_decay1 = { "Decay Flesh", NULL, unit_decay2 };
static umove_t unit_move_birth = { "birth", ai_birth, unit_stand };
static umove_t unit_move_stand = { "stand", ai_stand, unit_stand };
static umove_t unit_move_death = { "death", NULL, unit_decay1 };

//void unit_die(edict_t *self) {
//}

//void unit_decay2(edict_t *self) {
//    self->monsterinfo.currentmove = &unit_move_decay2;
//}

void unit_decay1(edict_t *self) {
//    self->monsterinfo.currentmove = &unit_move_decay1;
    self->aiflags |= AI_HOLD_FRAME;
}

void unit_stand(edict_t *self) {
    M_SetMove(self, &unit_move_stand);
    self->s.renderfx &= ~RF_NO_UBERSPLAT;
}

void unit_die(edict_t *self, edict_t *attacker) {
    M_SetMove(self, &unit_move_death);
}

void unit_birth(edict_t *self) {
    M_SetMove(self, &unit_move_birth);
    self->wait = UNIT_BUILD_TIME(self->class_id);
    self->s.renderfx |= RF_NO_UBERSPLAT;
}

void SP_monster_unit(edict_t *self) {
    self->movetype = MOVETYPE_STEP;
    self->die = unit_die;
    self->stand = unit_stand;
    self->birth = unit_birth;
    
    M_SetMove(self, &unit_move_stand);
    
    monster_start(self);
}
