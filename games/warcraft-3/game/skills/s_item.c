#include "s_skills.h"

#define ID_ITEM_HEAL MAKEFOURCC('A', 'I', 'h', 'e')
#define ID_ITEM_MANA MAKEFOURCC('A', 'I', 'm', 'a')
#define ID_ITEM_LIFE_GAIN MAKEFOURCC('A', 'I', 'm', 'i')

static void item_heal_command(LPEDICT clent) {
    LPEDICT target = G_GetMainSelectedUnit(clent->client);
    DWORD code = S_SpellCurrentCode(clent, ID_ITEM_HEAL);
    FLOAT amount = S_SpellData(code, 1, 1);

    if (!S_SpellIsAliveTarget(target)) {
        return;
    }
    S_SpellHeal(target, amount);
}

static void item_mana_command(LPEDICT clent) {
    LPEDICT target = G_GetMainSelectedUnit(clent->client);
    DWORD code = S_SpellCurrentCode(clent, ID_ITEM_MANA);
    FLOAT amount = S_SpellData(code, 1, 1);

    if (!target || amount <= 0) {
        return;
    }
    target->mana.value = MIN(target->mana.max_value, target->mana.value + amount);
}

static void item_permanent_life_command(LPEDICT clent) {
    LPEDICT target = G_GetMainSelectedUnit(clent->client);
    DWORD code = S_SpellCurrentCode(clent, ID_ITEM_LIFE_GAIN);
    FLOAT amount = S_SpellData(code, 1, 1);

    if (!target || amount <= 0) {
        return;
    }
    target->health.max_value += amount;
    target->health.value += amount;
}

ability_t a_item_heal = {
    .cmd = item_heal_command,
};

ability_t a_item_mana_regain = {
    .cmd = item_mana_command,
};

ability_t a_item_permanent_life_gain = {
    .cmd = item_permanent_life_command,
};

ability_t a_item_attack_bonus = {0};
ability_t a_item_stat_bonus = {0};
ability_t a_item_permanent_stat_gain = {0};
ability_t a_item_defense_bonus = {0};
ability_t a_item_life_bonus = {0};
ability_t a_item_mana_bonus = {0};
ability_t a_item_figurine_summon = {0};
ability_t a_item_experience_gain = {0};
ability_t a_item_level_gain = {0};
