#include "g_local.h"

void T_Damage(edict_t *target, edict_t *attacker, int damage) {
    if (target->health <= damage) {
        target->health = 0;
        target->die(target, attacker);
        attacker->stand(attacker);
        return;
    }
    target->health -= damage;
    /*if (other->melee && !other->enemy) {
     other->enemy = ent;
     other->melee(other);
     M_ChangeAngle(other);
     } else */if (target->pain) {
         target->pain(target);
     }
}

