#include "s_skills.h"

#define ID_DEVOTION_AURA "AHad"

static void devotionaura_execute(LPEDICT caster, spellTarget_t st, abilityitem_t const *spell) {
    (void)st;
    unit_addstatus(caster, ID_DEVOTION_AURA, 1);

    G_SpawnAbilityEffectTarget(spell->code, WC3_EFFECT_TARGET, 0, caster, NULL, false);
}

BZ_SIMPLE_SPELL_PROC(AbilityAuraDevotion, devotionaura_execute)
