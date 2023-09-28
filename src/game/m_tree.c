#include "g_local.h"

void tree_decay1(LPEDICT self);
void tree_stand(LPEDICT self);

static umove_t tree_move_birth = { "birth", ai_idle, tree_stand };
static umove_t tree_move_stand = { "stand", ai_idle, tree_stand };
static umove_t tree_move_pain = { "stand hit", ai_pain, tree_stand };
static umove_t tree_move_death = { "death", NULL, tree_decay1 };

void tree_decay1(LPEDICT self) {
//    self->monsterinfo.currentmove = &tree_move_decay1;
    self->aiflags |= AI_HOLD_FRAME;
}

void tree_pain(LPEDICT self) {
    M_SetMove(self, &tree_move_pain);
}

void tree_stand(LPEDICT self) {
    M_SetMove(self, &tree_move_stand);
}

void tree_die(LPEDICT self, LPEDICT attacker) {
    M_SetMove(self, &tree_move_death);
    G_PublishEvent(self, EVENT_UNIT_DEATH);
}

void tree_birth(LPEDICT self) {
    M_SetMove(self, &tree_move_birth);
}

void SP_monster_tree(LPEDICT self) {
    self->stand = tree_stand;
    self->pain = tree_pain;
    self->die = tree_die;

    M_SetMove(self, &tree_move_stand);
 
    void monster_think(LPEDICT self);
    self->think = monster_think;
    monster_start(self);
}
