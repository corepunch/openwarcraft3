#include "s_skills.h"

/* ---- Immolation (AEim): toggle AoE damage around caster ------------------ */

static void immolation_execute(LPEDICT caster, spellTarget_t st, spell_info_t const *spell) {
    DWORD code = spell->code;

    FOR_LOOP(i, MAX_UNIT_STATUSES) {
        if (caster->abilstatus[i].level && caster->abilstatus[i].code == code) {
            memset(&caster->abilstatus[i], 0, sizeof(caster->abilstatus[i]));
            G_InvalidateUnitInfoPanel(caster);
            return;
        }
    }
    unit_addstatus(caster, "Biml", 1);
}

static spell_info_t spell_immolation = {
    .code = MAKEFOURCC('A', 'E', 'i', 'm'),
    .name = "Immolation",
    .target_type = SPELL_TARGET_NONE,
    .flags = SPELL_TOGGLE,
    .execute = immolation_execute,
};

ability_t a_immolation = {
    .cmd = spell_cmd,
    .spell = &spell_immolation,
};

/* ---- Cold Arrows (AHca): autocast attack modifier with slow -------------- */

static void cold_arrows_execute(LPEDICT caster, spellTarget_t st, spell_info_t const *spell) {
    DWORD code = MAKEFOURCC('c', 'o', 'l', 'd');

    FOR_LOOP(i, MAX_UNIT_STATUSES) {
        if (caster->abilstatus[i].level && caster->abilstatus[i].code == code) {
            memset(&caster->abilstatus[i], 0, sizeof(caster->abilstatus[i]));
            G_InvalidateUnitInfoPanel(caster);
            return;
        }
    }
    unit_addstatus(caster, "cold", 1);
}

static spell_info_t spell_cold_arrows = {
    .code = MAKEFOURCC('A', 'H', 'c', 'a'),
    .name = "Cold Arrows",
    .target_type = SPELL_TARGET_NONE,
    .flags = SPELL_TOGGLE | SPELL_AUTOCAST,
    .execute = cold_arrows_execute,
};

ability_t a_cold_arrows = {
    .cmd = spell_cmd,
    .spell = &spell_cold_arrows,
};

/* War Stomp (AOws): damage and stun enemy ground units around the caster. */
static void war_stomp_execute(LPEDICT caster, spellTarget_t st, spell_info_t const *spell) {
    DWORD level = S_SpellLevel(caster, spell->code);
    FLOAT radius = S_SpellNumber(spell->code, ABILITY_NUMBER_AREA, level);
    DWORD damage = (DWORD)S_SpellData(spell->code, level, 1);
    FLOAT duration = S_SpellDuration(spell->code, level, false);

#define WAR_STOMP_HITS(t) ((t)->inuse && (t) != caster && S_SpellIsAliveTarget(t) && \
                           S_SpellIsEnemy(caster, t) && (t)->targtype == TARG_GROUND && \
                           Vector2_distance(&(t)->s.origin2, &caster->s.origin2) <= radius)
    FILTER_EDICTS(target, WAR_STOMP_HITS(target)) {
        T_Damage(target, caster, damage);
        if (!M_IsDead(target) && duration > 0.0f)
            unit_addtimedstatus(target, "Bstu", 1, duration);
    }
#undef WAR_STOMP_HITS
}

static spell_info_t spell_war_stomp = {
    .code = MAKEFOURCC('A', 'O', 'w', 's'),
    .name = "War Stomp",
    .target_type = SPELL_TARGET_NONE,
    .execute = war_stomp_execute,
};

ability_t a_war_stomp = {
    .cmd = spell_cmd,
    .spell = &spell_war_stomp,
};

ability_t a_aura_endurance = {0};

static void wind_walk_execute(LPEDICT caster, spellTarget_t st, spell_info_t const *spell) {
    DWORD level = S_SpellLevel(caster, spell->code);
    caster->s.renderfx |= RF_HIDDEN;
    unit_addtimedstatus(caster, "BOwk", level, S_SpellDuration(spell->code, level, true));
}

static spell_info_t spell_wind_walk = {
    .code = MAKEFOURCC('A', 'O', 'w', 'k'),
    .name = "Wind Walk",
    .target_type = SPELL_TARGET_NONE,
    .execute = wind_walk_execute,
};

ability_t a_wind_walk = {
    .cmd = spell_cmd,
    .spell = &spell_wind_walk,
};

/* Mana Burn (AEmb): remove the target's current mana, capped by DataA. */
static void mana_burn_execute(LPEDICT caster, spellTarget_t st, spell_info_t const *spell) {
    LPEDICT target = st.entity;
    DWORD level = S_SpellLevel(caster, spell->code);
    FLOAT amount = MIN(target->mana.value, S_SpellData(spell->code, level, 1));

    target->mana.value -= amount;
}

static spell_info_t spell_mana_burn = {
    .code = MAKEFOURCC('A', 'E', 'm', 'b'),
    .name = "Mana Burn",
    .target_type = SPELL_TARGET_UNIT,
    .execute = mana_burn_execute,
};

ability_t a_mana_burn = {
    .cmd = spell_cmd,
    .spell = &spell_mana_burn,
};

ability_t a_bash = {0};

/* Phoenix Fire, Invulnerable: passive abilities with no command handler.
 * Zero-initialized; the engine never calls cmd for passives. */
ability_t a_phoenix_fire = {0};
ability_t a_invulnerable = {0};

/* Harvest Lumber and Couple Instant: non-spell abilities with stub command handlers. */
static void stub_cancel_command(LPEDICT clent) {
    UI_AddCancelButton(clent);
}

ability_t a_harvest_lumber = {
    .cmd = stub_cancel_command,
};


ability_t a_couple_instant = {
    .cmd = stub_cancel_command,
};
