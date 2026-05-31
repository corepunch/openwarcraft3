#include "s_skills.h"

#define ID_CHARM MAKEFOURCC('A', 'N', 'c', 'h')
#define ID_EAT_TREE MAKEFOURCC('A', 'e', 'a', 't')
#define ID_MOON_WELL MAKEFOURCC('A', 'm', 'b', 't')

static BOOL charm_selecttarget(LPEDICT clent, LPEDICT target) {
    LPEDICT caster = G_GetMainSelectedUnit(clent->client);
    DWORD code = S_SpellCurrentCode(clent, ID_CHARM);
    DWORD level = S_SpellLevel(caster, code);
    FLOAT range = S_SpellRange(code, level);
    DWORD max_level = (DWORD)S_SpellData(code, level, 1);

    if (!S_SpellIsEnemy(caster, target) || !S_SpellAllowsTarget(code, caster, target)) {
        return false;
    }
    if (!S_SpellTargetInRange(caster, target, range)) {
        return false;
    }
    if (max_level && UNIT_LEVEL(target->class_id) > max_level) {
        return false;
    }
    if (!S_SpellCooldownReady(caster, code) || !S_SpellSpendMana(caster, code, level)) {
        return false;
    }
    S_SpellStartCooldown(caster, code, level);
    target->s.player = caster->s.player;
    target->owner = caster;
    target->combatentity = NULL;
    if (target->stand) {
        target->stand(target);
    }
    return true;
}

static void charm_command(LPEDICT clent) {
    UI_AddCancelButton(clent);
    clent->client->menu.on_entity_selected = charm_selecttarget;
}

static BOOL eat_tree_selecttarget(LPEDICT clent, LPEDICT target) {
    LPEDICT caster = G_GetMainSelectedUnit(clent->client);
    DWORD code = S_SpellCurrentCode(clent, ID_EAT_TREE);
    DWORD level = S_SpellLevel(caster, code);
    FLOAT range = S_SpellRange(code, level);
    FLOAT heal = S_SpellData(code, level, 3);

    if (!caster || !target || target->targtype != TARG_TREE) {
        return false;
    }
    if (!S_SpellTargetInRange(caster, target, range)) {
        return false;
    }
    if (!S_SpellCooldownReady(caster, code) || !S_SpellSpendMana(caster, code, level)) {
        return false;
    }
    S_SpellStartCooldown(caster, code, level);
    S_SpellHeal(caster, heal);
    G_FreeEdict(target);
    return true;
}

static void eat_tree_command(LPEDICT clent) {
    UI_AddCancelButton(clent);
    clent->client->menu.on_entity_selected = eat_tree_selecttarget;
}

static BOOL moon_well_selecttarget(LPEDICT clent, LPEDICT target) {
    LPEDICT caster = G_GetMainSelectedUnit(clent->client);
    DWORD code = S_SpellCurrentCode(clent, ID_MOON_WELL);
    DWORD level = S_SpellLevel(caster, code);
    FLOAT range = S_SpellRange(code, level);
    FLOAT mana_cost_per_point = MAX(1.0f, S_SpellData(code, level, 1));
    FLOAT health_gain = MAX(0.0f, S_SpellData(code, level, 2));
    FLOAT missing;
    FLOAT offered;

    if (!S_SpellIsFriend(caster, target) || !S_SpellTargetInRange(caster, target, range)) {
        return false;
    }
    missing = MAX(0.0f, target->health.max_value - target->health.value);
    offered = MIN(missing, caster->mana.value / mana_cost_per_point);
    offered = MIN(offered, health_gain > 0 ? health_gain : offered);
    if (offered <= 0) {
        return false;
    }
    caster->mana.value -= offered * mana_cost_per_point;
    S_SpellHeal(target, offered);
    return true;
}

static void moon_well_command(LPEDICT clent) {
    UI_AddCancelButton(clent);
    clent->client->menu.on_entity_selected = moon_well_selecttarget;
}

static void root_command(LPEDICT clent) {
    LPEDICT caster = G_GetMainSelectedUnit(clent->client);

    if (!caster) {
        return;
    }
    caster->no_pathing = !caster->no_pathing;
    caster->movetype = caster->no_pathing ? MOVETYPE_NONE : MOVETYPE_STEP;
    if (caster->stand) {
        caster->stand(caster);
    }
}

ability_t a_charm = {
    .cmd = charm_command,
};

ability_t a_eat_tree = {
    .cmd = eat_tree_command,
};

ability_t a_moon_well = {
    .cmd = moon_well_command,
};

ability_t a_root = {
    .cmd = root_command,
};
