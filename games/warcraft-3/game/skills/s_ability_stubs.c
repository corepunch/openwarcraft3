#include "s_skills.h"

/* Name=Immolation
 * Ubertip="Immolates nearby enemy units, dealing damage over time."
 * Untip="Deactivate Immolation"
 * Unubertip=""
 */

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

/* Name=Cold Arrows
 * Ubertip="Adds cold damage to attacks and slows the movement speed of the attacked unit."
 * Untip="Right-click to activate auto-casting."
 * Unubertip="Right-click to deactivate auto-casting."
 */

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

/* Name=War Stomp
 * Ubertip="Slams the ground, dealing <AOws,DataA1> damage to nearby enemy land units and stunning them for <AOws,Dur1> seconds."
 */
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

/* Name=Endurance Aura
 * Ubertip="Increases nearby friendly units' movement speed and attack rate."
 */
ability_t a_aura_endurance = {0};

/* Name=Wind Walk
 * Ubertip="Allows the Blademaster to become invisible and move faster until it attacks or uses an ability."
 */
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

/* Name=Mana Burn
 * Ubertip="Sends a bolt of negative energy that burns a target enemy unit's mana and deals damage proportional to the amount of mana burned."
 */
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

/* Name=Dark Ritual
 * Ubertip="Sacrifices a friendly non-Hero unit, converting a percentage of its hit points into mana for the caster."
 * Dark Ritual converts the authored fraction of an allied non-hero's maximum
 * life into caster mana, then uses the normal damage/death path to sacrifice it. */
static void dark_ritual_execute(LPEDICT caster, spellTarget_t st, spell_info_t const *spell) {
    LPEDICT target = st.entity;
    DWORD level = S_SpellLevel(caster, spell->code);
    FLOAT mana = target->health.max_value * S_SpellData(spell->code, level, 1);

    caster->mana.value = MIN(caster->mana.max_value, caster->mana.value + mana);
    T_Damage(target, caster, (DWORD)MAX(1.0f, target->health.value));
}

static spell_info_t spell_dark_ritual = {
    .code = MAKEFOURCC('A', 'U', 'd', 'r'),
    .name = "Dark Ritual",
    .target_type = SPELL_TARGET_UNIT,
    .execute = dark_ritual_execute,
};

ability_t a_dark_ritual = {
    .cmd = spell_cmd,
    .spell = &spell_dark_ritual,
};

/* Name=Frost Armor
 * Ubertip="Creates a shield of frost around a target friendly unit. The shield adds <ACfu,DataB1> armor and slows melee units that attack it for <ACfu,Dur1> seconds. Lasts <ACfu,DataA1> seconds."
 * Untip="Right-click to activate auto-casting."
 * Unubertip="Right-click to deactivate auto-casting."
 */
static void frost_armor_execute(LPEDICT caster, spellTarget_t st, spell_info_t const *spell) {
    LPEDICT target = st.entity;
    DWORD level = S_SpellLevel(caster, spell->code);
    AbilityData_t const *data = G_AbilityData(spell->code);
    LPCSTR buff = data->level[level - 1].buffID;

    if (!target || !buff || strlen(buff) < 4) {
        fprintf(stderr, "WC3: %.4s has no authored BuffID\n", (LPCSTR)&spell->code);
        return;
    }
    unit_addtimedstatus(target, buff, level, S_SpellData(spell->code, level, 1));
    G_SpawnAbilityEffectTarget(spell->code, WC3_EFFECT_TARGET, 0, target, NULL, true);
}

static spell_info_t spell_frost_armor = {
    .code = MAKEFOURCC('A', 'U', 'f', 'a'),
    .name = "Frost Armor",
    .target_type = SPELL_TARGET_UNIT,
    .execute = frost_armor_execute,
};

static spell_info_t spell_frost_armor_variant = {
    .code = MAKEFOURCC('A', 'U', 'f', 'u'),
    .name = "Frost Armor Variant",
    .target_type = SPELL_TARGET_UNIT,
    .execute = frost_armor_execute,
};

ability_t a_frost_armor = { .cmd = spell_cmd, .spell = &spell_frost_armor };
ability_t a_frost_armor_variant = { .cmd = spell_cmd, .spell = &spell_frost_armor_variant };

static void divine_shield_think(LPEDICT ent) {
    LPEDICT caster = ent->owner;

    if (!caster || !caster->inuse || G_Time() < ent->spawn_time) return;
    caster->invulnerable = ent->resources;
    G_FreeEdict(ent);
}

/* Name=Divine Shield
 * Ubertip="Makes the Paladin invulnerable to damage for <AHds,Dur1> seconds."
 */
static void divine_shield_execute(LPEDICT caster, spellTarget_t st, spell_info_t const *spell) {
    DWORD level = S_SpellLevel(caster, spell->code);
    LPEDICT thinker = G_Spawn();

    thinker->owner = caster;
    thinker->resources = caster->invulnerable;
    thinker->spawn_time = G_Time() + (DWORD)(MAX(0.1f, S_SpellDuration(spell->code, level, true)) * 1000.0f);
    caster->invulnerable = true;
    thinker->think = divine_shield_think;
    G_SpawnAbilityEffectTarget(spell->code, WC3_EFFECT_CASTER, 0, caster, NULL, true);
}

static spell_info_t spell_divine_shield = {
    .code = MAKEFOURCC('A', 'H', 'd', 's'),
    .name = "Divine Shield",
    .target_type = SPELL_TARGET_NONE,
    .execute = divine_shield_execute,
};

ability_t a_divine_shield = { .cmd = spell_cmd, .spell = &spell_divine_shield };

/* Name=Bash
 * Ubertip="Gives a chance that an attack will deal bonus damage and stun the target."
 * TODO: no command handler; the attack-resolution path must consume this passive.
 */
ability_t a_bash = {0};

/* Entangling Roots applies the authored timed buff; movement owns the root
 * consumer so expiry naturally restores the unit without a second cleanup path. */
/* Name=Entangling Roots
 * Ubertip="Roots a target enemy unit in place, preventing movement for <AEer,Dur1> seconds."
 */
static void entangling_roots_execute(LPEDICT caster, spellTarget_t st, spell_info_t const *spell) {
    DWORD level = S_SpellLevel(caster, spell->code);
    AbilityData_t const *data = G_AbilityData(spell->code);
    LPCSTR buff = data->level[level - 1].buffID;
    LPEDICT target = st.entity;
    FLOAT duration;

    if (!target || !buff || strlen(buff) < 4) {
        fprintf(stderr, "WC3: Entangling Roots has no authored BuffID\n");
        return;
    }
    duration = S_SpellDuration(spell->code, level, G_UnitIsHero(target));
    unit_addtimedstatus(target, buff, level, duration);
    G_SpawnAbilityEffectTarget(spell->code, WC3_EFFECT_TARGET, 0, target, NULL, true);
}

static spell_info_t spell_entangling_roots = {
    .code = MAKEFOURCC('A', 'E', 'e', 'r'),
    .name = "Entangling Roots",
    .target_type = SPELL_TARGET_UNIT,
    .execute = entangling_roots_execute,
};

ability_t a_entangling_roots = {
    .cmd = spell_cmd,
    .spell = &spell_entangling_roots,
};

/* Explicit coverage marker; no command hook means this cannot create a dead button. */
ability_t a_unimplemented = { .flags = ABILITY_PASSIVE };

/* Name=Phoenix Fire
 * Ubertip="Automatically attacks nearby enemy units with flaming projectiles."
 * TODO: passive attack-resolution behavior is not implemented here.
 */
ability_t a_phoenix_fire = {0};
/* Name=Invulnerable
 * Ubertip="This unit cannot be damaged."
 */
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
