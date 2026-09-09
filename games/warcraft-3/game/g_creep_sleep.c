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

/* common.j declares PLAYER_STATE_NO_CREEP_SLEEP as playerstate 25.  The
 * WC3-local constant lives in g_local.h instead of widening common/shared.h. */
static void creep_sleep_think(LPEDICT self);
static umove_t creep_sleep_move = { .animation = "sleep", .think = creep_sleep_think, .endfunc = NULL };

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

/* Leave natural sleep and restore the normal stand move when the unit is alive. */
void G_UnitWakeUp(LPEDICT unit) {
    if (!G_UnitIsSleeping(unit))
        return;
    unit->sleep.sleeping = false;
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
    return true;
}

/* Wake sleeping creeps as soon as the simulation reaches daytime. */
static void creep_sleep_think(LPEDICT self) {
    if (!G_IsNight())
        G_UnitWakeUp(self);
}
