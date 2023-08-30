#include "s_skills.h"

static void ShowTrainedUnit(LPEDICT townhall, LPEDICT unit) {
    VECTOR2 origin;
    FLOAT angle;
    SP_FindEmptySpaceAround(townhall, unit->class_id, &origin, &angle);
    unit->s.origin2 = origin;
    unit->s.angle = angle;
    unit->s.renderfx &= ~RF_HIDDEN;
    unit->stand(unit);
}

void ai_build(LPEDICT ent) {
    FLOAT const k = (FLOAT)FRAMETIME / (FLOAT)UNIT_BUILD_TIME_MSEC(ent->build->class_id);
    EDICTSTAT *hp = &ent->build->health;
    hp->value += hp->max_value * k;
    if (hp->value >= hp->max_value) {
        hp->value = hp->max_value;
        ShowTrainedUnit(ent, ent->build);
        if (!(ent->build = ent->build->build)) {
            ent->stand(ent);
        }
    }
}

static umove_t train_move_train = { "stand", ai_build, NULL, &a_train };

void unit_add_build_queue(LPEDICT self, LPEDICT item) {
    if (!self->build) {
        self->build = item;
    } else {
        LPEDICT last = self->build;
        while (last->build) last = last->build;
        last->build = item;
    }
}

void unit_build(LPEDICT self, DWORD class_id) {
    LPEDICT ent = SP_SpawnAtLocation(class_id, self->s.player, &self->s.origin2);
    ent->health.value = 0;
    ent->birth(ent);
    ent->s.renderfx |= RF_HIDDEN;
    unit_add_build_queue(self, ent);
    M_SetMove(self, &train_move_train);
}

void SP_TrainUnit(LPEDICT townhall, DWORD class_id) {
    LPEDICT clent = g_edicts+townhall->s.player;
    playerState_t *player = G_GetPlayerByNumber(townhall->s.player);
    if (player_pay(player, class_id)) {
        unit_build(townhall, class_id);
        Get_Commands_f(clent);
    } else {
        fprintf(stdout, "Not enough resources\n");
    }
}

ability_t a_train = {
    
};
