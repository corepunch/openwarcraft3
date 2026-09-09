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
static umove_t creep_sleep_move = { "sleep", creep_sleep_think, NULL };

static BOOL unit_is_neutral_sleep_candidate(LPCEDICT unit) {
    return unit && unit->s.player >= PLAYER_NEUTRAL_AGGRESSIVE &&
           unit->s.player < MAX_PLAYERS;
}

static BOOL neutral_hostile_sleep_disabled(void) {
    if (!game.clients || PLAYER_NEUTRAL_AGGRESSIVE >= game.max_clients)
        return false;
    return game.clients[PLAYER_NEUTRAL_AGGRESSIVE]
               .ps.stats[WC3_PLAYERSTATE_NO_CREEP_SLEEP] != 0;
}

BOOL G_UnitCanSleep(LPCEDICT unit) {
    return unit_is_neutral_sleep_candidate(unit) && unit->sleep.can_sleep;
}

BOOL G_UnitIsSleeping(LPCEDICT unit) {
    return unit && unit->sleep.sleeping && unit->currentmove == &creep_sleep_move;
}

BOOL G_IsCreepSleepMove(umove_t const *move) {
    return move == &creep_sleep_move;
}

void G_UnitWakeUp(LPEDICT unit) {
    if (!G_UnitIsSleeping(unit))
        return;
    unit->sleep.sleeping = false;
    if (!M_IsDead(unit))
        unit_stand(unit);
}

void G_UnitSetCanSleep(LPEDICT unit, BOOL can_sleep) {
    if (!unit_is_neutral_sleep_candidate(unit))
        return;
    unit->sleep.can_sleep = can_sleep;
    if (!can_sleep)
        G_UnitWakeUp(unit);
}

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

static void creep_sleep_think(LPEDICT self) {
    if (!G_IsNight())
        G_UnitWakeUp(self);
}
