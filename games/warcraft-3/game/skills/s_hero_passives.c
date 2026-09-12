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
#define ID_POISON_ARROWS MAKEFOURCC('A', 'E', 'p', 'a')
#define ID_TRUESHOT_AURA MAKEFOURCC('A', 'E', 'a', 'r')

#define ID_REGEN_LIFE_ORC MAKEFOURCC('A', 'o', 'a', 'r')
#define ID_REGEN_LIFE_BLIGHT MAKEFOURCC('A', 'a', 'b', 'r')
#define ID_REGEN_MANA MAKEFOURCC('A', 'a', 'r', 'm')

typedef struct {
    DWORD alias;
    DWORD level;
} auraAbilityRef_t;

static auraAbilityRef_t actor_aura_ability(LPEDICT ent, DWORD base_code) {
    auraAbilityRef_t result = {0};
    char alias_name[5] = {0};

    if (!ent || !base_code) return result;

    if (ent->data.UnitAbilities && ent->data.UnitAbilities->abilList) {
        PARSE_LIST(ent->data.UnitAbilities->abilList, token, parse_segment) {
            DWORD alias = 0;
            if (strlen(token) != 4 || !G_ActorHasSkill(ent, token)) continue;
            memcpy(&alias, token, sizeof(alias));
            if (G_AbilityCode(alias) == base_code) {
                result.alias = alias;
                result.level = 1;
                return result;
            }
        }
    }

    FOR_LOOP(i, ARRAY_COUNT(ent->abilities.added)) {
        DWORD const alias = ent->abilities.added[i];
        if (!alias) continue;
        memcpy(alias_name, &alias, 4);
        if (G_ActorHasSkill(ent, alias_name) && G_AbilityCode(alias) == base_code) {
            result.alias = alias;
            result.level = 1;
            return result;
        }
    }

    FOR_LOOP(i, MAX_HERO_ABILITIES) {
        heroability_t const *hero = ent->heroabilities + i;
        if (hero->level && G_AbilityCode(hero->code) == base_code) {
            result.alias = hero->code;
            result.level = hero->level;
            return result;
        }
    }
    return result;
}

static BOOL aura_target_has_token(LPCSTR targets, LPCSTR full, LPCSTR short_name) {
    char token[32];
    LPCSTR cursor = targets;

    while (cursor && *cursor) {
        size_t len = 0;
        while (*cursor == ',' || isspace((unsigned char)*cursor)) cursor++;
        while (*cursor && *cursor != ',' && len + 1 < sizeof(token)) token[len++] = *cursor++;
        while (len && isspace((unsigned char)token[len - 1])) len--;
        token[len] = '\0';
        if (!strcasecmp(token, full) || (short_name && !strcasecmp(token, short_name))) return true;
        while (*cursor && *cursor != ',') cursor++;
    }
    return false;
}

static BOOL aura_allows_target(LPEDICT source, LPEDICT target, LPCSTR targets) {
    BOOL is_self, is_friend, is_enemy, is_neutral;
    BOOL const wants_vulnerability = aura_target_has_token(targets, "vulnerable", "vuln") ||
        aura_target_has_token(targets, "invulnerable", "invu");

    if (!source || !target || !target->inuse || !S_SpellIsAliveTarget(target)) return false;
    is_self = source == target;
    is_friend = S_SpellIsFriend(source, target);
    is_enemy = S_SpellIsEnemy(source, target);
    is_neutral = target->s.player < MAX_PLAYERS && level.mapinfo &&
        level.mapinfo->players[target->s.player].playerType == kPlayerTypeNeutral;
    BOOL const wants_relation = aura_target_has_token(targets, "friend", "frie") ||
        aura_target_has_token(targets, "allies", "alli") ||
        aura_target_has_token(targets, "enemy", "enem") ||
        aura_target_has_token(targets, "enemies", NULL) ||
        aura_target_has_token(targets, "neutral", "neut") ||
        aura_target_has_token(targets, "self", NULL);

    if (!targets || !*targets) return true;
    if (aura_target_has_token(targets, "dead", NULL)) return false;
    if (aura_target_has_token(targets, "notself", "nots") && is_self) return false;
    if (aura_target_has_token(targets, "hero", NULL) && !G_UnitIsHero(target)) return false;
    if (aura_target_has_token(targets, "nonhero", "nonh") && G_UnitIsHero(target)) return false;
    if (aura_target_has_token(targets, "mechanical", "mech") && target->targtype != TARG_MECHANICAL) return false;
    if (aura_target_has_token(targets, "organic", "orga") && target->targtype == TARG_MECHANICAL) return false;
    if (aura_target_has_token(targets, "structure", "stru") && target->targtype != TARG_STRUCTURE) return false;
    /* WC3 target lists may name both vulnerability classes; that means either
     * class is accepted, not that both conditions must hold. */
    if (wants_vulnerability &&
        !((aura_target_has_token(targets, "vulnerable", "vuln") && !target->invulnerable) ||
          (aura_target_has_token(targets, "invulnerable", "invu") && target->invulnerable))) return false;
    if ((aura_target_has_token(targets, "air", NULL) || aura_target_has_token(targets, "ground", "grou")) &&
        !(aura_target_has_token(targets, "air", NULL) && target->targtype == TARG_AIR) &&
        !(aura_target_has_token(targets, "ground", "grou") && target->targtype == TARG_GROUND)) return false;
    if (wants_relation &&
        !(aura_target_has_token(targets, "self", NULL) && is_self) &&
        !((aura_target_has_token(targets, "friend", "frie") || aura_target_has_token(targets, "allies", "alli")) && is_friend) &&
        !((aura_target_has_token(targets, "enemy", "enem") || aura_target_has_token(targets, "enemies", NULL)) && is_enemy) &&
        !(aura_target_has_token(targets, "neutral", "neut") && is_neutral)) return false;
    return true;
}

static FLOAT regen_aura_bonus(LPEDICT unit, DWORD base_code, BOOL use_maximum) {
    FLOAT bonus = 0.0f;

    FOR_LOOP(i, globals.num_edicts) {
        LPEDICT source = g_edicts + i;
        auraAbilityRef_t const ability = actor_aura_ability(source, base_code);
        abilityLevel_t const *row;
        FLOAT amount;

        if (!source->inuse || !ability.alias || !S_SpellIsAliveTarget(source)) continue;
        row = G_AbilityLevel(ability.alias, ability.level);
        FLOAT const distance = Vector2_distance(&source->s.origin2, &unit->s.origin2);
        if (distance > row->area) continue;
        if (!aura_allows_target(source, unit, row->targs)) {
            continue;
        }
        amount = row->data[0].number;
        if (row->data[1].number != 0.0f && use_maximum)
            amount *= base_code == ID_REGEN_MANA ? unit->mana.max_value : unit->health.max_value;
        bonus = MAX(bonus, amount);
    }
    return bonus;
}

FLOAT S_RegenerationHealthAura(LPEDICT unit) {
    return regen_aura_bonus(unit, ID_REGEN_LIFE_ORC, true) +
           regen_aura_bonus(unit, ID_REGEN_LIFE_BLIGHT, true);
}

FLOAT S_RegenerationManaAura(LPEDICT unit) {
    return regen_aura_bonus(unit, ID_REGEN_MANA, true);
}

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
    DWORD code = ID_SEARING_ARROWS;
    if (!level) { level = G_UnitStatusLevel(attacker, ID_POISON_ARROWS); code = ID_POISON_ARROWS; }
    return level && attacker->attack1.weapon == WPN_MISSILE ? damage + (int)S_SpellData(code, level, 1) : damage;
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
