#include "s_skills.h"

#define ID_BRILLIANCE MAKEFOURCC('A', 'H', 'a', 'b')
#define ID_CRITICAL_STRIKE MAKEFOURCC('A', 'O', 'c', 'r')
#define ID_SPIKED_CARAPACE MAKEFOURCC('A', 'U', 't', 's')
#define ID_UNHOLY_AURA MAKEFOURCC('A', 'U', 'a', 'u')
#define ID_EVASION MAKEFOURCC('A', 'E', 'e', 'v')
#define ID_VAMPIRIC_AURA MAKEFOURCC('A', 'U', 'a', 'v')

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

BOOL S_EvasionRoll(LPEDICT target) {
    DWORD level = G_UnitAbilityLevel(target, ID_EVASION);
    return level && (FLOAT)(rand() % 10000) / 10000.0f < S_SpellData(ID_EVASION, level, 1);
}

int S_CriticalStrikeDamage(LPEDICT attacker, int damage) {
    DWORD level = G_UnitAbilityLevel(attacker, ID_CRITICAL_STRIKE);
    if (!level || (FLOAT)(rand() % 100) >= S_SpellData(ID_CRITICAL_STRIKE, level, 1)) return damage;
    return (int)((FLOAT)damage * MAX(1.0f, S_SpellData(ID_CRITICAL_STRIKE, level, 2)));
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