#include "g_local.h"

#include "Units/UnitWeapons.h"

//void unit_die(LPEDICT self);
//void unit_decay2(LPEDICT self);
void unit_decay1(LPEDICT self);
void unit_cooldown(LPEDICT self);
void unit_stand(LPEDICT self);

//static mmove_t unit_move_decay2 = { "Decay Bone", NULL, unit_die };
//static mmove_t unit_move_decay1 = { "Decay Flesh", NULL, unit_decay2 };
static umove_t unit_move_stand = { ANIM_STAND, ai_stand, unit_stand };
static umove_t unit_move_walk = { ANIM_WALK, ai_walk };
static umove_t unit_move_melee = { ANIM_ATTACK, ai_melee, unit_cooldown };
static umove_t unit_move_death = { ANIM_DEATH, NULL, unit_decay1 };
static umove_t unit_move_cooldown = { ANIM_STAND_READY, ai_cooldown };

//void unit_die(LPEDICT self) {
//}

//void unit_decay2(LPEDICT self) {
//    self->monsterinfo.currentmove = &unit_move_decay2;
//}

#define SET_MOVE(EDICT, MOVE) \
EDICT->unitinfo.currentmove = &MOVE; \
EDICT->animation = gi.GetAnimation(EDICT->s.model, MOVE.animation);

void unit_cooldown(LPEDICT self) {
    SET_MOVE(self, unit_move_cooldown);
    self->unitinfo.wait = self->unitinfo.weapon->cool1 * 1000;
}

void unit_decay1(LPEDICT self) {
//    self->monsterinfo.currentmove = &unit_move_decay1;
    self->unitinfo.aiflags |= AI_HOLD_FRAME;
}

void unit_walk(LPEDICT self) {
    SET_MOVE(self, unit_move_walk);
}

void unit_stand(LPEDICT self) {
    SET_MOVE(self, unit_move_stand);
}

void unit_melee(LPEDICT self) {
    SET_MOVE(self, unit_move_melee);
    self->unitinfo.wait = self->unitinfo.weapon->dmgpt1 * 1000;
}

void unit_die(LPEDICT self, LPEDICT attacker) {
    SET_MOVE(self, unit_move_death);
}

void SP_monster_unit(LPEDICT self) {
    self->movetype = MOVETYPE_STEP;
    
    self->die = unit_die;

    self->unitinfo.stand = unit_stand;
    self->unitinfo.walk = unit_walk;
    self->unitinfo.melee = unit_melee;
    
    SET_MOVE(self, unit_move_stand);
    
    monster_start(self);
}
