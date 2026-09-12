#include "s_skills.h"

void holylight_done(LPEDICT self);

static umove_t move_heal = { "stand channel", ai_idle, holylight_done, &CAbilityHolyBolt };

void holylight_done(LPEDICT self) {
    self->stand(self);
}

/* Holy Light has unique target validation: friendlies are healed, undead
 * enemies take half-damage.  Self-target and non-undead enemies are rejected. */
static BOOL CAbilityHolyBolt_Validate(LPEDICT caster, spellTarget_t st) {
    LPEDICT target = st.entity;

    if (target == caster) return false;
    if (!S_SpellIsAliveTarget(target)) return false;
    if (S_SpellIsEnemy(caster, target)) {
        LPCSTR race = target->data.UnitData->race;
        return race && !strcmp(race, STR_UNDEAD);
    }
    return S_SpellIsFriend(caster, target);
}

static void CAbilityHolyBolt_Execute(LPEDICT caster, spellTarget_t st, ability_t const *spell) {
    LPEDICT target = st.entity;
    DWORD level = S_SpellLevel(caster, spell->code);
    FLOAT amount = S_SpellData(spell->code, level, 1);

    G_SpawnAbilityEffectTarget(spell->code, WC3_EFFECT_TARGET, 0, target, NULL, true);
    unit_setmove(caster, &move_heal);
    if (S_SpellIsFriend(caster, target))
        S_SpellHeal(target, amount);
    else
        S_SpellDamage(target, caster, (int)(amount * 0.5f));
}

ability_t CAbilityHolyBolt = {
    .flags = AB_SPELL,
    .name = "Holy Light",
    .target_type = SPELL_TARGET_UNIT,
    .validate = CAbilityHolyBolt_Validate,
    .execute = CAbilityHolyBolt_Execute,
};
