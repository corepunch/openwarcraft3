#include "s_skills.h"

#define ID_DEVOTION_AURA "AHad"

BZ_SIMPLE_SPELL_PROC(AbilityAuraDevotion) {
    (void)st;
    unit_addstatus(caster, ID_DEVOTION_AURA, 1);

    G_SpawnAbilityEffectTarget(spell->code, WC3_EFFECT_TARGET, 0, caster, NULL, false);
}
