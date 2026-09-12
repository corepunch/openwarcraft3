#include "s_skills.h"

#define ID_CHARM MAKEFOURCC('A', 'N', 'c', 'h')
#define ID_MOON_WELL MAKEFOURCC('A', 'm', 'b', 't')

/* ---- Charm (ANch): transfer target ownership to caster -------------------- */

static BOOL charm_validate(LPEDICT caster, spellTarget_t st) {
    LPEDICT target = st.entity;
    DWORD level = S_SpellLevel(caster, ID_CHARM);
    DWORD max_level = (DWORD)S_SpellData(ID_CHARM, level, 1);

    if (!S_SpellIsEnemy(caster, target)) return false;
    if (max_level && target->data.UnitBalance->level > max_level) return false;
    return true;
}

static void charm_execute(LPEDICT caster, spellTarget_t st, ability_t const *spell) {
    LPEDICT target = st.entity;
    (void)spell;

    G_SetUnitPlayer(target, caster->s.player);
    target->owner = caster;
    target->combatentity = NULL;
    if (target->stand)
        target->stand(target);
}

/* ---- Eat Tree (Aeat): consume a tree for healing ------------------------- */

static BOOL eat_tree_validate(LPEDICT caster, spellTarget_t st) {
    LPEDICT target = st.entity;
    if (!target || target->targtype != TARG_TREE) return false;
    return true;
}

static void eat_tree_execute(LPEDICT caster, spellTarget_t st, ability_t const *spell) {
    LPEDICT target = st.entity;
    DWORD level = S_SpellLevel(caster, spell->code);
    FLOAT heal = S_SpellData(spell->code, level, 3);

    S_SpellHeal(caster, heal);
    G_FreeEdict(target);
}

/* ---- Moon Well (Ambt): transfer mana to health for a friendly unit -------- */

static BOOL moon_well_validate(LPEDICT caster, spellTarget_t st) {
    LPEDICT target = st.entity;
    LPEDICT well = caster;
    DWORD level = S_SpellLevel(well, ID_MOON_WELL);
    FLOAT mana_cost_per_point = MAX(1.0f, S_SpellData(ID_MOON_WELL, level, 1));
    FLOAT health_gain = MAX(0.0f, S_SpellData(ID_MOON_WELL, level, 2));
    FLOAT missing, offered;

    if (!S_SpellIsFriend(caster, target)) return false;
    missing = MAX(0.0f, target->health.max_value - target->health.value);
    offered = MIN(missing, caster->mana.value / mana_cost_per_point);
    offered = MIN(offered, health_gain > 0 ? health_gain : offered);
    return offered > 0;
}

static void moon_well_execute(LPEDICT caster, spellTarget_t st, ability_t const *spell) {
    LPEDICT target = st.entity;
    DWORD level = S_SpellLevel(caster, spell->code);
    FLOAT mana_cost_per_point = MAX(1.0f, S_SpellData(spell->code, level, 1));
    FLOAT health_gain = MAX(0.0f, S_SpellData(spell->code, level, 2));
    FLOAT missing, offered;

    missing = MAX(0.0f, target->health.max_value - target->health.value);
    offered = MIN(missing, caster->mana.value / mana_cost_per_point);
    offered = MIN(offered, health_gain > 0 ? health_gain : offered);
    caster->mana.value -= offered * mana_cost_per_point;
    S_SpellHeal(target, offered);
}

/* ---- Root (Aroo): toggle rooted state ------------------------------------ */

static void root_command(LPEDICT clent) {
    LPEDICT caster = G_GetMainSelectedUnit(clent->client);
    if (!caster) return;
    caster->no_pathing = !caster->no_pathing;
    caster->movetype = caster->no_pathing ? MOVETYPE_NONE : MOVETYPE_STEP;
    if (caster->stand)
        caster->stand(caster);
}

/* ---- Registration -------------------------------------------------------- */

ability_t CAbilityCharm = {
    .flags = AB_SPELL_SIMPLE,
    .name = "Charm",
    .target_type = SPELL_TARGET_UNIT,
    .validate = charm_validate,
    .execute = charm_execute,
};

ability_t CAbilityEatTree = {
    .flags = AB_SPELL_SIMPLE,
    .name = "Eat Tree",
    .target_type = SPELL_TARGET_UNIT,
    .validate = eat_tree_validate,
    .execute = eat_tree_execute,
};

ability_t CAbilityManaBattery = {
    .flags = AB_SPELL_SIMPLE,
    .name = "Moon Well",
    .target_type = SPELL_TARGET_UNIT,
    .validate = moon_well_validate,
    .execute = moon_well_execute,
};

ability_t CAbilityRoot = {
    .cmd = root_command,
};
