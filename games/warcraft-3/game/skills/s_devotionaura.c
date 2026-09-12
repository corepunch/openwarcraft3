#include "s_skills.h"

#define ID_DEVOTION_AURA "AHad"

static void devotionaura_execute(LPEDICT caster, spellTarget_t st, ability_t const *spell) {
    (void)st;
    unit_addstatus(caster, ID_DEVOTION_AURA, 1);

    G_SpawnAbilityEffectTarget(spell->code, WC3_EFFECT_TARGET, 0, caster, NULL, false);
}

ability_t CAbilityAuraDevotion = {
    .flags = AB_SPELL_SIMPLE,
    .name = "Devotion Aura",
    .target_type = SPELL_TARGET_NONE,
    .execute = devotionaura_execute,
};
