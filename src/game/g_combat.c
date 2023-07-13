#include "g_local.h"

void T_Damage(edict_t *target, edict_t *attacker, int damage) {
    if (target->health.value <= damage) {
        target->health.value = 0;
        target->die(target, attacker);
        attacker->stand(attacker);
        return;
    }
    target->health.value -= damage;
    /*if (other->melee && !other->enemy) {
     other->enemy = ent;
     other->melee(other);
     M_ChangeAngle(other);
     } else */if (target->pain) {
         target->pain(target);
     }
}

