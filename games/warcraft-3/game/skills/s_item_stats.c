#include "s_skills.h"

/* Passive item stat bonuses — apply on pickup, reverse on drop.
 * Follows WarSmash pattern: CAbilityItemAttackBonus.onAdd/onRemove,
 * CAbilityItemDefenseBonus.onAdd/onRemove, etc. */

static void apply_attack(LPEDICT unit, FLOAT amount) {
    unit->attack1.temporaryDamageBonus += amount;
    unit->attack2.temporaryDamageBonus += amount;
    G_InvalidateUnitInfoPanel(unit);
}

static void apply_defense(LPEDICT unit, FLOAT amount) {
    unit->temporary_armor_bonus += amount;
    unit->armor_value += amount;
    G_InvalidateUnitInfoPanel(unit);
}

static void apply_life(LPEDICT unit, FLOAT amount) {
    FLOAT old_max = unit->health.max_value;
    if (old_max <= 0) old_max = 1.0f;
    FLOAT ratio = unit->health.value / old_max;
    unit->health.max_value += amount;
    G_SetHealth(unit, unit->health.max_value * ratio);
}

static void apply_mana(LPEDICT unit, FLOAT amount) {
    FLOAT old_max = unit->mana.max_value;
    if (old_max <= 0) old_max = 1.0f;
    FLOAT ratio = unit->mana.value / old_max;
    unit->mana.max_value += amount;
    unit->mana.value = unit->mana.max_value * ratio;
}

/* Attribute aliases share the authored Agility/Intelligence/Strength field order used by tomes. */
static void apply_stat(LPEDICT unit, DWORD code, FLOAT sign) {
    FLOAT str = sign * S_SpellData(code, 1, 3);
    FLOAT agi = sign * S_SpellData(code, 1, 1);
    FLOAT intel = sign * S_SpellData(code, 1, 2);
    if (!G_UnitIsHero(unit)) {
        return;
    }
    unit->hero.str = (DWORD)MAX(0, (LONG)unit->hero.str + (LONG)str);
    unit->hero.agi = (DWORD)MAX(0, (LONG)unit->hero.agi + (LONG)agi);
    unit->hero.intel = (DWORD)MAX(0, (LONG)unit->hero.intel + (LONG)intel);
    G_RecomputeHeroStats(unit);
}

/* Inventory messages retain the authored alias, including distinct values on stacked items. */
#define BZ_ITEM_BONUS_PROC(NAME, APPLY) \
    BZ_ABILITY_PROC(C##NAME) { \
        if (msg != A_ITEM_ADD && msg != A_ITEM_REMOVE) return CAbilityPassive(ent, msg, call); \
        if (!ent || !call || !call->item) return false; \
        FLOAT sign = msg == A_ITEM_ADD ? 1.0f : -1.0f; \
        APPLY(ent, sign * S_SpellData(call->item->code, 1, 1)); \
        return true; \
    }

BZ_ITEM_BONUS_PROC(AbilityAttackBonus, apply_attack)
BZ_ITEM_BONUS_PROC(AbilityDefenseBonus, apply_defense)
BZ_ITEM_BONUS_PROC(AbilityMaxLifeBonus, apply_life)
BZ_ITEM_BONUS_PROC(AbilityMaxManaBonus, apply_mana)

/* Read every attribute from the same alias on acquisition and removal. */
BZ_ABILITY_PROC(CAbilityAttributeBonus) {
    if (msg != A_ITEM_ADD && msg != A_ITEM_REMOVE) return CAbilityPassive(ent, msg, call);
    if (!ent || !call || !call->item) return false;
    FLOAT sign = msg == A_ITEM_ADD ? 1.0f : -1.0f;
    apply_stat(ent, call->item->code, sign);
    return true;
}
