#include "s_skills.h"

#define ID_BRILLIANCE MAKEFOURCC('A', 'H', 'a', 'b')
#define ID_CRITICAL_STRIKE MAKEFOURCC('A', 'O', 'c', 'r')
#define ID_SPIKED_CARAPACE MAKEFOURCC('A', 'U', 't', 's')
#define ID_UNHOLY_AURA MAKEFOURCC('A', 'U', 'a', 'u')
#define ID_EVASION MAKEFOURCC('A', 'E', 'e', 'v')
#define ID_VAMPIRIC_AURA MAKEFOURCC('A', 'U', 'a', 'v')
#define ID_THORNS_AURA MAKEFOURCC('A', 'E', 'a', 'h')
#define ID_MANA_SHIELD MAKEFOURCC('A', 'N', 'm', 's')
#define ID_DRUNKEN_BRAWLER MAKEFOURCC('A', 'N', 'd', 'b')
#define ID_SEARING_ARROWS MAKEFOURCC('A', 'H', 'f', 'a')
#define ID_TRUESHOT_AURA MAKEFOURCC('A', 'E', 'a', 'r')

static FLOAT hero_aura_bonus(LPEDICT unit, DWORD code, DWORD data) {
    FLOAT bonus = 0.0f;

    FOR_LOOP(i, globals.num_edicts) {
        LPEDICT source = g_edicts + i;
        DWORD level = G_UnitAbilityLevel(source, code);
        if (!source->inuse || !level || !S_SpellIsAliveTarget(source) || !S_SpellIsFriend(source, unit)) continue;
        if (Vector2_distance(&source->s.origin2, &unit->s.origin2) > S_SpellNumber(code, ABILITY_NUMBER_AREA, level))
            continue;
        bonus = MAX(bonus, S_SpellData(code, level, data));
    }
    return bonus;
}

FLOAT S_BrillianceManaRegen(LPEDICT unit) { return hero_aura_bonus(unit, ID_BRILLIANCE, 1); }
FLOAT S_UnholyHealthRegen(LPEDICT unit) { return hero_aura_bonus(unit, ID_UNHOLY_AURA, 2); }
FLOAT S_UnholyMoveBonus(LPEDICT unit) { return hero_aura_bonus(unit, ID_UNHOLY_AURA, 1); }
FLOAT S_VampiricLifeSteal(LPEDICT unit) { return hero_aura_bonus(unit, ID_VAMPIRIC_AURA, 1); }

FLOAT S_TrueshotAttackBonus(LPEDICT unit) {
    return unit->attack1.type == ATK_PIERCE ? hero_aura_bonus(unit, ID_TRUESHOT_AURA, 1) : 0.0f;
}

int S_SearingArrowDamage(LPEDICT attacker, int damage) {
    DWORD level = G_UnitStatusLevel(attacker, ID_SEARING_ARROWS);
    return level && attacker->attack1.weapon == WPN_MISSILE
        ? damage + (int)S_SpellData(ID_SEARING_ARROWS, level, 1) : damage;
}

/* Mana Shield converts incoming damage to mana loss using the authored Ams4 factor. */
int S_ManaShieldDamage(LPEDICT target, int damage) {
    DWORD level = G_UnitAbilityLevel(target, ID_MANA_SHIELD);
    FLOAT loss, absorbed;
    if (!level || damage <= 0 || target->mana.value <= 0.0f) return damage;
    loss = MAX(0.001f, S_SpellData(ID_MANA_SHIELD, level, 1));
    absorbed = MIN((FLOAT)damage, target->mana.value / loss);
    target->mana.value -= absorbed * loss;
    return damage - (int)absorbed;
}

FLOAT S_ThornsDamageReturn(LPCEDICT target, LPCEDICT attacker, FLOAT damage) {
    if (!target || !attacker || (attacker->attack1.weapon != WPN_NORMAL && attacker->attack1.weapon != WPN_INSTANT))
        return 0.0f;
    return damage * hero_aura_bonus((LPEDICT)target, ID_THORNS_AURA, 1);
}

BOOL S_EvasionRoll(LPEDICT target) {
    DWORD level = G_UnitAbilityLevel(target, ID_EVASION);
    if (level && (FLOAT)(rand() % 10000) / 10000.0f < S_SpellData(ID_EVASION, level, 1)) return true;
    level = G_UnitAbilityLevel(target, ID_DRUNKEN_BRAWLER);
    return level && (FLOAT)(rand() % 10000) / 10000.0f < S_SpellData(ID_DRUNKEN_BRAWLER, level, 4);
}

int S_CriticalStrikeDamage(LPEDICT attacker, int damage) {
    DWORD level = G_UnitAbilityLevel(attacker, ID_CRITICAL_STRIKE);
    DWORD code = ID_CRITICAL_STRIKE;
    if (!level) { level = G_UnitAbilityLevel(attacker, ID_DRUNKEN_BRAWLER); code = ID_DRUNKEN_BRAWLER; }
    if (!level || (FLOAT)(rand() % 100) >= S_SpellData(code, level, 1)) return damage;
    return (int)((FLOAT)damage * MAX(1.0f, S_SpellData(code, level, 2)));
}

FLOAT S_SpikedArmorBonus(LPCEDICT unit) {
    DWORD level = G_UnitAbilityLevel(unit, ID_SPIKED_CARAPACE);
    return level ? S_SpellData(ID_SPIKED_CARAPACE, level, 3) : 0.0f;
}

FLOAT S_SpikedDamageReturn(LPCEDICT unit, FLOAT damage) {
    DWORD level = G_UnitAbilityLevel(unit, ID_SPIKED_CARAPACE);
    if (!level) return 0.0f;
    return MAX(S_SpellData(ID_SPIKED_CARAPACE, level, 2), damage * S_SpellData(ID_SPIKED_CARAPACE, level, 1));
}

ability_t a_brilliance_aura = { .flags = ABILITY_PASSIVE };
ability_t a_critical_strike = { .flags = ABILITY_PASSIVE };
ability_t a_spiked_carapace = { .flags = ABILITY_PASSIVE };
ability_t a_unholy_aura = { .flags = ABILITY_PASSIVE };
ability_t a_evasion = { .flags = ABILITY_PASSIVE };
ability_t a_vampiric_aura = { .flags = ABILITY_PASSIVE };
ability_t a_aura_spell = { .flags = ABILITY_PASSIVE };
ability_t a_mana_shield = { .flags = ABILITY_PASSIVE };
ability_t a_drunken_brawler = { .flags = ABILITY_PASSIVE };
ability_t a_cleaving_attack = { .flags = ABILITY_PASSIVE };