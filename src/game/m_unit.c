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
    G_PublishEvent(self, EVENT_UNIT_DEATH);
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

void order_move(LPEDICT self, LPEDICT target);
void order_stop(LPEDICT clent);

BOOL unit_issue_order(LPEDICT self, LPCSTR order, LPCVECTOR2 point) {
    if (!strcmp(order, "move") || !strcmp(order, "attack")) {
        LPEDICT waypoint = Waypoint_add(point);
        order_move(self, waypoint);
        return true;
    }
//    printf("%.4s %s\n", &self->class_id, order);
    return false;
}

BOOL unit_issue_immediate_order(LPEDICT self, LPCSTR order) {
    if (!strcmp(order, "stop")) {
        order_stop(self);
        return true;
    }
//    printf("%.4s %s\n", &self->class_id, order);
    return false;
}

LPEDICT unit_create_or_find(DWORD player, DWORD unitid, LPCVECTOR2 location, FLOAT facing) {
    FOR_LOOP(i, globals.num_edicts) {
        LPEDICT ent = &globals.edicts[i];
        if (ent->class_id == unitid &&
            Vector2_distance(location, &ent->s.origin2) < 10)
        {
            ent->s.player = player;
            ent->s.angle = facing;
            return ent;
        }
    }
    LPEDICT unit = SP_SpawnAtLocation(unitid, player, location);
    unit->s.angle = facing;
    return unit;
}
