#include "g_local.h"

void tree_decay1(edict_t *self);
void tree_stand(edict_t *self);

static umove_t tree_move_birth = { "birth", ai_stand, tree_stand };
static umove_t tree_move_stand = { "stand", ai_stand, tree_stand };
static umove_t tree_move_pain = { "stand hit", ai_pain, tree_stand };
static umove_t tree_move_death = { "death", NULL, tree_decay1 };

void tree_decay1(edict_t *self) {
//    self->monsterinfo.currentmove = &tree_move_decay1;
    self->aiflags |= AI_HOLD_FRAME;
}

void tree_pain(edict_t *self) {
    M_SetMove(self, &tree_move_pain);
}

void tree_stand(edict_t *self) {
    M_SetMove(self, &tree_move_stand);
}

void tree_die(edict_t *self, edict_t *attacker) {
    M_SetMove(self, &tree_move_death);
}

void tree_birth(edict_t *self) {
    M_SetMove(self, &tree_move_birth);
}

void SP_monster_tree(edict_t *self) {
    self->stand = tree_stand;
    self->pain = tree_pain;
    self->die = tree_die;

    M_SetMove(self, &tree_move_stand);
 
    void monster_think(edict_t *self);
    self->think = monster_think;
    monster_start(self);
}
