#include "s_skills.h"

#define ID_TIMED_LIFE "BTLF"

static void summon_unit(LPEDICT caster, DWORD unit_id, DWORD index, DWORD count, FLOAT duration) {
    VECTOR2 loc;
    FLOAT angle;
    LPEDICT summon;

    if (!caster || !unit_id)
        return;

    angle = count > 0 ? (2.0f * (FLOAT)M_PI * (FLOAT)index) / (FLOAT)count : 0.0f;
    loc = caster->s.origin2;
    loc.x += cosf(angle) * MAX(64.0f, caster->collision + 32.0f);
    loc.y += sinf(angle) * MAX(64.0f, caster->collision + 32.0f);
    SP_FindEmptySpaceAround(caster, unit_id, &loc, &angle);

    summon = SP_SpawnAtLocation(unit_id, caster->s.player, &loc);
    if (!summon)
        return;
    summon->owner = caster;
    G_ActivateUnitFood(summon);
    if (summon->stand)
        summon->stand(summon);
    if (duration > 0)
        unit_addtimedstatus(summon, ID_TIMED_LIFE, 1, duration);
    G_PublishSummonEvents(caster, summon);
}

void S_SummonUnits(LPEDICT caster, DWORD unit_id, DWORD count, FLOAT duration) {
    if (!count) count = 1;
    FOR_LOOP(i, count) summon_unit(caster, unit_id, i, count, duration);
}

/* Replace only the caster's prior Feral Spirit summons before spawning the new cast. */
static void feral_spirit_execute(LPEDICT caster, spellTarget_t st, ability_t const *spell) {
    DWORD level, unit_id, count;
    FLOAT duration, distance;
    VECTOR2 loc;

    if (!caster) return;
    level = S_SpellLevel(caster, spell->code);
    unit_id = S_SpellUnitId(spell->code, level);
    count = (DWORD)S_SpellData(spell->code, level, 2);
    duration = S_SpellDuration(spell->code, level, false);
    distance = S_SpellNumber(spell->code, ABILITY_NUMBER_AREA, level);
    if (!unit_id || !count) return;

    /* Warsmash keeps the previous Feral Spirit cast as ability-owned summons:
     * recasting kills only surviving wolves from that caster's prior cast. */
    FILTER_EDICTS(unit, unit->inuse && unit->owner == caster &&
                  unit->summon_ability == spell->code && !M_IsDead(unit)) {
        if (unit->die) unit->die(unit, caster);
        else unit_die(unit, caster);
    }

    loc = caster->s.origin2;
    loc.x += cosf(caster->s.angle) * distance;
    loc.y += sinf(caster->s.angle) * distance;
    FOR_LOOP(i, count) {
        LPEDICT summon = S_SummonAt(caster, unit_id, &loc, duration);
        if (!summon) continue;
        summon->summon_ability = spell->code;
        G_SpawnAbilityEffectTarget(spell->code, WC3_EFFECT_SPECIAL, 0, summon, NULL, true);
    }
}

LPEDICT S_SummonAt(LPEDICT caster, DWORD unit_id, LPCVECTOR2 loc, FLOAT duration) {
    LPEDICT summon;
    if (!caster || !unit_id || !loc) return NULL;
    summon = SP_SpawnAtLocation(unit_id, caster->s.player, loc);
    if (!summon) return NULL;
    summon->owner = caster; G_ActivateUnitFood(summon);
    if (summon->stand) summon->stand(summon);
    if (duration > 0.0f) unit_addtimedstatus(summon, ID_TIMED_LIFE, 1, duration);
    G_PublishSummonEvents(caster, summon);
    return summon;
}

static void summon_execute(LPEDICT caster, spellTarget_t st, ability_t const *spell) {
    DWORD level = S_SpellLevel(caster, spell->code);
    DWORD unit_id = S_SpellUnitId(spell->code, level);
    DWORD count = (DWORD)S_SpellData(spell->code, level, 2);
    FLOAT duration = S_SpellDuration(spell->code, level, false);

    if (!caster || !unit_id) return;
    S_SummonUnits(caster, unit_id, count, duration);
}

/* Name=Summon Water Elemental
 * Ubertip="Summons a Water Elemental to fight for the caster."
 */
ability_t CAbilityWaterElemental = {
    .flags = AB_SPELL,
    .name = "Water Elemental",
    .target_type = SPELL_TARGET_NONE,
    .execute = summon_execute,
};

/* Name=Feral Spirit
 * Ubertip="Summons Spirit Wolf companions."
 */
ability_t CAbilitySpiritWolf = {
    .flags = AB_SPELL,
    .name = "Feral Spirit",
    .target_type = SPELL_TARGET_NONE,
    .execute = feral_spirit_execute,
};

/* Name=Force of Nature
 * Ubertip="Summons treants from a target area to fight for the caster."
 */
ability_t CAbilityForceOfNature = {
    .flags = AB_SPELL,
    .name = "Force of Nature",
    .target_type = SPELL_TARGET_NONE,
    .execute = summon_execute,
};

/* Name=Summon Bear
 * Ubertip="Summons Misha, a powerful bear, to attack your enemies."
 */
ability_t CAbilitySummonGrizzly = {
    .flags = AB_SPELL,
    .name = "Summon Bear",
    .target_type = SPELL_TARGET_NONE,
    .execute = summon_execute,
};
/* Name=Summon Quilbeast
 * Ubertip="Summons an angry quilbeast to fling spines at your enemies."
 */
ability_t CAbilitySummonQuillbeast = {
    .flags = AB_SPELL,
    .name = "Summon Quilbeast",
    .target_type = SPELL_TARGET_NONE,
    .execute = summon_execute,
};
/* Name=Summon Hawk
 * Ubertip="Summons a hawk to fight for the caster."
 */
ability_t CAbilitySummonWarEagle = {
    .flags = AB_SPELL,
    .name = "Summon Hawk",
    .target_type = SPELL_TARGET_NONE,
    .execute = summon_execute,
};
