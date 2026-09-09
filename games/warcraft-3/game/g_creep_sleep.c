/*
 * g_creep_sleep.c — Warcraft III natural neutral-creep sleep.
 *
 * Natural creep sleep is separate from the Dreadlord Sleep spell (AUsl/BUsL).
 * UnitData.canSleep supplies the authored night-sleep capability; the runtime
 * flag may be changed by UnitAddSleep.  Only Neutral Hostile automatically
 * enters natural sleep at night.  Sleeping units use their authored Sleep MDX
 * sequence and wake at dawn, on positive damage, or through UnitWakeUp.
 */
#include "g_local.h"

#define ID_CREEP_SLEEP MAKEFOURCC('A', 'C', 's', 'p')
#define CREEP_SLEEP_TARGET_ART \
    "Abilities\\Spells\\Other\\CreepSleep\\CreepSleepTarget.mdx"

/* common.j declares PLAYER_STATE_NO_CREEP_SLEEP as playerstate 25.  The
 * WC3-local constant lives in g_local.h instead of widening common/shared.h. */
static void creep_sleep_think(LPEDICT self);
static umove_t creep_sleep_move = { .animation = "sleep", .think = creep_sleep_think, .endfunc = NULL };

/* Match only the overlay owned by this unit, leaving unrelated target effects untouched. */
static BOOL is_creep_sleep_overlay(LPCEDICT effect, LPCEDICT unit) {
    return effect && effect->inuse && effect->owner == unit && effect->goalentity == unit;
}

/* Destroy every natural-sleep overlay before the unit leaves its sleep move. */
static void remove_creep_sleep_overlay(LPEDICT unit) {
    FOR_LOOP(i, globals.num_edicts) {
        LPEDICT effect = g_edicts + i;
        if (is_creep_sleep_overlay(effect, unit))
            G_DestroyEffect(effect);
    }
}

/* Spawn ACsp's target art at overhead. Some retail data sets do not expose
 * the hidden ability's TargetArt through the loaded Func metadata, so preserve
 * data/map overrides first and otherwise use Warcraft's canonical sleep art. */
static void add_creep_sleep_overlay(LPEDICT unit) {
    LPCSTR art = G_AbilityEffectArt(ID_CREEP_SLEEP, WC3_EFFECT_TARGET, 0);
    LPEDICT effect;

    if (art && *art) {
        effect = G_SpawnAbilityEffectTarget(ID_CREEP_SLEEP, WC3_EFFECT_TARGET, 0,
                                            unit, "overhead", false);
    } else {
        fprintf(stderr, "WC3 CreepSleep: ACsp TargetArt missing; using canonical sleep art for unit %u\n",
                unit->s.number);
        effect = G_SpawnModelEffect(CREEP_SLEEP_TARGET_ART, NULL, unit,
                                    "overhead", false);
    }
    if (!effect) {
        fprintf(stderr, "WC3 CreepSleep: failed to spawn ACsp overlay for unit %u\n", unit->s.number);
        return;
    }
    effect->owner = unit;
}

/* Restrict automatic sleep to authored neutral-creep candidates. */
static BOOL unit_is_neutral_sleep_candidate(LPCEDICT unit) {
    return unit && unit->s.player >= PLAYER_NEUTRAL_AGGRESSIVE &&
           unit->s.player < MAX_PLAYERS;
}

/* Read the authoritative player-state switch that disables natural sleep. */
static BOOL neutral_hostile_sleep_disabled(void) {
    if (!game.clients || PLAYER_NEUTRAL_AGGRESSIVE >= game.max_clients)
        return false;
    return game.clients[PLAYER_NEUTRAL_AGGRESSIVE]
               .ps.stats[WC3_PLAYERSTATE_NO_CREEP_SLEEP] != 0;
}

/* Report whether a unit retains the runtime permission to enter natural sleep. */
BOOL G_UnitCanSleep(LPCEDICT unit) {
    return unit_is_neutral_sleep_candidate(unit) && unit->sleep.can_sleep;
}

/* Distinguish the authored creep sleep move from other animation moves. */
BOOL G_UnitIsSleeping(LPCEDICT unit) {
    return unit && unit->sleep.sleeping && unit->currentmove == &creep_sleep_move;
}

/* Identify the private move record used by natural neutral-creep sleep. */
BOOL G_IsCreepSleepMove(umove_t const *move) {
    return move == &creep_sleep_move;
}

/* Clear natural sleep and remove its persistent target presentation effect. */
void G_UnitLeaveCreepSleep(LPEDICT unit) {
    if (!unit || !unit->sleep.sleeping)
        return;
    unit->sleep.sleeping = false;
    remove_creep_sleep_overlay(unit);
}

/* Leave natural sleep and restore the normal stand move when the unit is alive. */
void G_UnitWakeUp(LPEDICT unit) {
    if (!G_UnitIsSleeping(unit))
        return;
    G_UnitLeaveCreepSleep(unit);
    if (!M_IsDead(unit))
        unit_stand(unit);
}

/* Apply UnitAddSleep's mutable permission and wake a unit when disabling it. */
void G_UnitSetCanSleep(LPEDICT unit, BOOL can_sleep) {
    if (!unit_is_neutral_sleep_candidate(unit))
        return;
    unit->sleep.can_sleep = can_sleep;
    if (!can_sleep)
        G_UnitWakeUp(unit);
}

/* Enter natural sleep only for idle Neutral Hostile units during nighttime. */
BOOL G_TryEnterCreepSleep(LPEDICT unit) {
    if (!unit || unit->s.player != PLAYER_NEUTRAL_AGGRESSIVE ||
        !G_UnitCanSleep(unit) || G_UnitIsSleeping(unit) || M_IsDead(unit) ||
        unit_affectingcombat(unit) || !G_IsNight() || neutral_hostile_sleep_disabled()) {
        return false;
    }

    unit_setmove(unit, &creep_sleep_move);
    unit->sleep.sleeping = true;
    add_creep_sleep_overlay(unit);
    return true;
}

/* Wake sleeping creeps as soon as the simulation reaches daytime. */
static void creep_sleep_think(LPEDICT self) {
    if (!G_IsNight())
        G_UnitWakeUp(self);
}
