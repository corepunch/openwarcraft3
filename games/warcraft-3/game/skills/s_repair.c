#include "s_skills.h"

void repair_build(LPEDICT ent, LPEDICT building);

static void ai_repair(LPEDICT ent) {
    FLOAT const k = (FLOAT)FRAMETIME / (FLOAT)UNIT_BUILD_TIME_MSEC(ent->build->class_id);
    EDICTSTAT *hp = &ent->build->health;
    hp->value += hp->max_value * k;
    if (hp->value >= hp->max_value) {
        hp->value = hp->max_value;
        ent->build->stand(ent->build);
        ent->stand(ent);
    }
}

static umove_t repair_move_build = { "stand work", ai_repair, NULL, &a_repair };

void repair_build(LPEDICT ent, LPEDICT building) {
    VECTOR2 origin;
    FLOAT angle;
    unit_setmove(ent, &repair_move_build);
    SP_FindEmptySpaceAround(building, ent->class_id, &origin, &angle);
    ent->s.origin2 = origin;
    ent->s.angle = angle - M_PI;
    ent->build = building;
//    building->health.value = 0;
//    building->build = building;
}

ability_t a_repair = {
//    .cmd = build_command,
};
