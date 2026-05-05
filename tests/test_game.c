/*
 * test_game.c — Tests for game utilities not covered by other suites.
 *
 * Covered:
 *   G_RegionContains  — point-in-region containment (empty, hit, miss,
 *                       multi-rect, exclusive upper boundary)
 *   G_FreeEdict       — entity lifecycle: inuse cleared, freetime stamped
 *   M_IsDead          — health-based liveness check
 *   compress_stat     — 8-bit health/mana encoding
 *   FindEnumValue     — NULL-terminated string-enum lookup
 *   unit_runwait      — per-frame wait counter and callback dispatch
 *   unit_issuetargetorder — attack and unknown-order paths
 *   unit_learnability — hero ability slot management
 *   Alliance types    — ALLIANCE_SHARED_VISION and independent flags
 *   Player resources  — PLAYERSTATE_RESOURCE_GOLD / LUMBER set/get
 */

#include "test_framework.h"
#include "test_harness.h"

/* Forward declarations for internal functions not exposed in any header. */
BOOL  M_IsDead(LPEDICT ent);
DWORD FindEnumValue(LPCSTR value, LPCSTR values[]);
void  unit_runwait(LPEDICT self, void (*callback)(LPEDICT));

/* =========================================================================
 * Helpers
 * ========================================================================= */

static LPPLAYER game_player(int idx) {
    game.clients[idx].ps.number = (DWORD)idx;
    return &game.clients[idx].ps;
}

static LPEDICT make_test_unit(void) {
    reset_entities();
    LPEDICT ent = alloc_test_unit(UNIT_ID("hpea"), 0.0f, 0.0f);
    ent->health.value     = 250.0f;
    ent->health.max_value = 250.0f;
    ent->stand            = unit_stand;
    ent->movetype         = MOVETYPE_STEP;
    unit_stand(ent);
    return ent;
}

/* =========================================================================
 * G_RegionContains
 * ========================================================================= */

static void test_region_contains_empty_region_false(void) {
    REGION r = { .num_rects = 0 };
    VECTOR2 p = { 5.0f, 5.0f };
    ASSERT(!G_RegionContains(&r, &p));
}

static void test_region_contains_point_inside(void) {
    REGION r = {
        .rects[0] = { { 0.0f, 0.0f }, { 100.0f, 100.0f } },
        .num_rects = 1
    };
    VECTOR2 p = { 50.0f, 50.0f };
    ASSERT(G_RegionContains(&r, &p));
}

static void test_region_contains_point_outside(void) {
    REGION r = {
        .rects[0] = { { 0.0f, 0.0f }, { 100.0f, 100.0f } },
        .num_rects = 1
    };
    VECTOR2 p = { 200.0f, 200.0f };
    ASSERT(!G_RegionContains(&r, &p));
}

static void test_region_contains_multirect_hits_second(void) {
    /* Two non-overlapping rects; the point is in the second one. */
    REGION r = {
        .rects[0] = { {   0.0f,   0.0f }, {  50.0f,  50.0f } },
        .rects[1] = { { 200.0f, 200.0f }, { 300.0f, 300.0f } },
        .num_rects = 2
    };
    VECTOR2 p = { 250.0f, 250.0f };
    ASSERT(G_RegionContains(&r, &p));
}

static void test_region_contains_max_boundary_exclusive(void) {
    /* Box2_containsPoint uses x < max.x (exclusive upper bound). */
    REGION r = {
        .rects[0] = { { 0.0f, 0.0f }, { 100.0f, 100.0f } },
        .num_rects = 1
    };
    VECTOR2 p = { 100.0f, 50.0f };   /* exactly at max.x */
    ASSERT(!G_RegionContains(&r, &p));
}

/* =========================================================================
 * G_FreeEdict
 * ========================================================================= */

static void test_free_edict_clears_inuse(void) {
    LPEDICT ent = make_test_unit();
    ASSERT(ent->inuse);
    G_FreeEdict(ent);
    ASSERT(!ent->inuse);
}

static void test_free_edict_stamps_freetime(void) {
    LPEDICT ent = make_test_unit();
    level.time = 9876;
    G_FreeEdict(ent);
    ASSERT_EQ_INT((int)ent->freetime, 9876);
}

/* =========================================================================
 * M_IsDead
 * ========================================================================= */

static void test_is_dead_alive_unit_false(void) {
    LPEDICT ent = make_test_unit();
    ent->health.value = 100.0f;
    ASSERT(!M_IsDead(ent));
}

static void test_is_dead_zero_hp_true(void) {
    LPEDICT ent = make_test_unit();
    ent->health.value = 0.0f;
    ASSERT(M_IsDead(ent));
}

static void test_is_dead_negative_hp_true(void) {
    LPEDICT ent = make_test_unit();
    ent->health.value = -1.0f;
    ASSERT(M_IsDead(ent));
}

/* =========================================================================
 * compress_stat
 * ========================================================================= */

static void test_compress_stat_full_health_is_255(void) {
    EDICTSTAT s = { 250.0f, 250.0f };
    ASSERT_EQ_INT((int)compress_stat(&s), 255);
}

static void test_compress_stat_zero_health_is_0(void) {
    EDICTSTAT s = { 0.0f, 250.0f };
    ASSERT_EQ_INT((int)compress_stat(&s), 0);
}

static void test_compress_stat_half_health(void) {
    EDICTSTAT s = { 125.0f, 250.0f };
    /* 255 * 125 / 250 = 127 (integer truncation). */
    ASSERT_EQ_INT((int)compress_stat(&s), 127);
}

static void test_compress_stat_zero_max_is_0(void) {
    EDICTSTAT s = { 0.0f, 0.0f };
    ASSERT_EQ_INT((int)compress_stat(&s), 0);
}

/* =========================================================================
 * FindEnumValue
 * ========================================================================= */

static LPCSTR test_attack_types[] = {
    "none", "normal", "pierce", "siege", "chaos", NULL
};

static void test_find_enum_first_value(void) {
    ASSERT_EQ_INT((int)FindEnumValue("none", test_attack_types), 0);
}

static void test_find_enum_later_value(void) {
    ASSERT_EQ_INT((int)FindEnumValue("pierce", test_attack_types), 2);
}

static void test_find_enum_null_input_returns_0(void) {
    ASSERT_EQ_INT((int)FindEnumValue(NULL, test_attack_types), 0);
}

static void test_find_enum_unknown_returns_0(void) {
    ASSERT_EQ_INT((int)FindEnumValue("magic", test_attack_types), 0);
}

/* =========================================================================
 * unit_runwait
 * ========================================================================= */

static int _runwait_cb_count = 0;

static void runwait_cb(LPEDICT ent) {
    (void)ent;
    _runwait_cb_count++;
}

static void test_runwait_zero_wait_no_callback(void) {
    LPEDICT ent = make_test_unit();
    ent->wait = 0.0f;
    _runwait_cb_count = 0;
    unit_runwait(ent, runwait_cb);
    ASSERT_EQ_INT(_runwait_cb_count, 0);
}

static void test_runwait_large_wait_decrements(void) {
    /* FRAMETIME = 100 ms → FRAMETIME/1000.f = 0.1 s. */
    LPEDICT ent = make_test_unit();
    ent->wait = 1.0f;
    _runwait_cb_count = 0;
    unit_runwait(ent, runwait_cb);
    /* wait should decrease by 0.1. */
    ASSERT_EQ_FLOAT(ent->wait, 0.9f, 0.01f);
    ASSERT_EQ_INT(_runwait_cb_count, 0);
}

static void test_runwait_small_wait_triggers_callback(void) {
    /* wait == 0.05 < FRAMETIME/1000.f (0.1) → callback fires. */
    LPEDICT ent = make_test_unit();
    ent->wait = 0.05f;
    _runwait_cb_count = 0;
    unit_runwait(ent, runwait_cb);
    ASSERT_EQ_INT(_runwait_cb_count, 1);
    ASSERT_EQ_FLOAT(ent->wait, 0.0f, 0.0001f);
}

/* =========================================================================
 * unit_issuetargetorder
 * ========================================================================= */

static void test_issuetargetorder_attack_returns_true(void) {
    LPEDICT unit   = make_test_unit();
    LPEDICT target = alloc_test_unit(UNIT_ID("hfoo"), 50.0f, 0.0f);
    /* order_attack is the real implementation from s_attack.c — just verify return value. */
    BOOL result = unit_issuetargetorder(unit, "attack", target);
    ASSERT(result);
}

static void test_issuetargetorder_unknown_returns_false(void) {
    LPEDICT unit   = make_test_unit();
    LPEDICT target = alloc_test_unit(UNIT_ID("hfoo"), 50.0f, 0.0f);
    BOOL result = unit_issuetargetorder(unit, "heal", target);
    ASSERT(!result);
}

/* =========================================================================
 * unit_learnability
 * ========================================================================= */

static void test_learnability_first_ability_fills_slot0(void) {
    LPEDICT ent = make_test_unit();
    DWORD code = UNIT_ID("AHbz");
    unit_learnability(ent, code);
    ASSERT_EQ_INT((int)ent->heroabilities[0].code,  (int)code);
    ASSERT_EQ_INT((int)ent->heroabilities[0].level, 1);
}

static void test_learnability_same_code_increments_level(void) {
    LPEDICT ent = make_test_unit();
    DWORD code = UNIT_ID("AHbz");
    unit_learnability(ent, code);
    unit_learnability(ent, code);
    ASSERT_EQ_INT((int)ent->heroabilities[0].level, 2);
    /* Should still be in slot 0, not duplicated in slot 1. */
    ASSERT_EQ_INT((int)ent->heroabilities[1].code, 0);
}

static void test_learnability_different_codes_fill_consecutive_slots(void) {
    LPEDICT ent = make_test_unit();
    DWORD code1 = UNIT_ID("AHbz");
    DWORD code2 = UNIT_ID("AHtb");
    unit_learnability(ent, code1);
    unit_learnability(ent, code2);
    ASSERT_EQ_INT((int)ent->heroabilities[0].code, (int)code1);
    ASSERT_EQ_INT((int)ent->heroabilities[1].code, (int)code2);
    ASSERT_EQ_INT((int)ent->heroabilities[1].level, 1);
}

/* =========================================================================
 * Alliance type variations
 * ========================================================================= */

static void test_alliance_shared_vision_set_get(void) {
    LPPLAYER p0 = game_player(0);
    LPPLAYER p1 = game_player(1);
    /* Clear alliance table. */
    memset(level.alliances, 0, sizeof(level.alliances));
    G_SetPlayerAlliance(p0, p1, ALLIANCE_SHARED_VISION, true);
    ASSERT(G_GetPlayerAlliance(p0, p1, ALLIANCE_SHARED_VISION));
}

static void test_alliance_shared_vision_does_not_set_passive(void) {
    LPPLAYER p0 = game_player(0);
    LPPLAYER p1 = game_player(1);
    memset(level.alliances, 0, sizeof(level.alliances));
    G_SetPlayerAlliance(p0, p1, ALLIANCE_SHARED_VISION, true);
    /* Setting SHARED_VISION must not accidentally set PASSIVE. */
    ASSERT(!G_GetPlayerAlliance(p0, p1, ALLIANCE_PASSIVE));
}

static void test_alliance_multiple_types_independent(void) {
    LPPLAYER p0 = game_player(0);
    LPPLAYER p1 = game_player(1);
    memset(level.alliances, 0, sizeof(level.alliances));
    G_SetPlayerAlliance(p0, p1, ALLIANCE_PASSIVE,       true);
    G_SetPlayerAlliance(p0, p1, ALLIANCE_SHARED_VISION, true);
    ASSERT(G_GetPlayerAlliance(p0, p1, ALLIANCE_PASSIVE));
    ASSERT(G_GetPlayerAlliance(p0, p1, ALLIANCE_SHARED_VISION));
}

static void test_alliance_revoke_one_type_keeps_other(void) {
    LPPLAYER p0 = game_player(0);
    LPPLAYER p1 = game_player(1);
    memset(level.alliances, 0, sizeof(level.alliances));
    G_SetPlayerAlliance(p0, p1, ALLIANCE_PASSIVE,       true);
    G_SetPlayerAlliance(p0, p1, ALLIANCE_SHARED_VISION, true);
    G_SetPlayerAlliance(p0, p1, ALLIANCE_SHARED_VISION, false);
    ASSERT( G_GetPlayerAlliance(p0, p1, ALLIANCE_PASSIVE));
    ASSERT(!G_GetPlayerAlliance(p0, p1, ALLIANCE_SHARED_VISION));
}

/* =========================================================================
 * Player resource stats — GOLD and LUMBER
 * ========================================================================= */

static void test_player_gold_default_zero(void) {
    LPPLAYER p = game_player(0);
    ASSERT_EQ_INT((int)p->stats[PLAYERSTATE_RESOURCE_GOLD], 0);
}

static void test_player_gold_set_get(void) {
    LPPLAYER p = game_player(0);
    p->stats[PLAYERSTATE_RESOURCE_GOLD] = 500;
    ASSERT_EQ_INT((int)p->stats[PLAYERSTATE_RESOURCE_GOLD], 500);
}

static void test_player_lumber_set_get(void) {
    LPPLAYER p = game_player(0);
    p->stats[PLAYERSTATE_RESOURCE_LUMBER] = 200;
    ASSERT_EQ_INT((int)p->stats[PLAYERSTATE_RESOURCE_LUMBER], 200);
}

static void test_player_gold_lumber_independent(void) {
    LPPLAYER p = game_player(1);
    p->stats[PLAYERSTATE_RESOURCE_GOLD]   = 300;
    p->stats[PLAYERSTATE_RESOURCE_LUMBER] = 150;
    ASSERT_EQ_INT((int)p->stats[PLAYERSTATE_RESOURCE_GOLD],   300);
    ASSERT_EQ_INT((int)p->stats[PLAYERSTATE_RESOURCE_LUMBER], 150);
}

/* =========================================================================
 * Suite runner
 * ========================================================================= */

BEGIN_SUITE(game)
    /* G_RegionContains */
    RUN_TEST(test_region_contains_empty_region_false);
    RUN_TEST(test_region_contains_point_inside);
    RUN_TEST(test_region_contains_point_outside);
    RUN_TEST(test_region_contains_multirect_hits_second);
    RUN_TEST(test_region_contains_max_boundary_exclusive);

    /* G_FreeEdict */
    RUN_TEST(test_free_edict_clears_inuse);
    RUN_TEST(test_free_edict_stamps_freetime);

    /* M_IsDead */
    RUN_TEST(test_is_dead_alive_unit_false);
    RUN_TEST(test_is_dead_zero_hp_true);
    RUN_TEST(test_is_dead_negative_hp_true);

    /* compress_stat */
    RUN_TEST(test_compress_stat_full_health_is_255);
    RUN_TEST(test_compress_stat_zero_health_is_0);
    RUN_TEST(test_compress_stat_half_health);
    RUN_TEST(test_compress_stat_zero_max_is_0);

    /* FindEnumValue */
    RUN_TEST(test_find_enum_first_value);
    RUN_TEST(test_find_enum_later_value);
    RUN_TEST(test_find_enum_null_input_returns_0);
    RUN_TEST(test_find_enum_unknown_returns_0);

    /* unit_runwait */
    RUN_TEST(test_runwait_zero_wait_no_callback);
    RUN_TEST(test_runwait_large_wait_decrements);
    RUN_TEST(test_runwait_small_wait_triggers_callback);

    /* unit_issuetargetorder */
    RUN_TEST(test_issuetargetorder_attack_returns_true);
    RUN_TEST(test_issuetargetorder_unknown_returns_false);

    /* unit_learnability */
    RUN_TEST(test_learnability_first_ability_fills_slot0);
    RUN_TEST(test_learnability_same_code_increments_level);
    RUN_TEST(test_learnability_different_codes_fill_consecutive_slots);

    /* Alliance type variations */
    RUN_TEST(test_alliance_shared_vision_set_get);
    RUN_TEST(test_alliance_shared_vision_does_not_set_passive);
    RUN_TEST(test_alliance_multiple_types_independent);
    RUN_TEST(test_alliance_revoke_one_type_keeps_other);

    /* Player resource stats */
    RUN_TEST(test_player_gold_default_zero);
    RUN_TEST(test_player_gold_set_get);
    RUN_TEST(test_player_lumber_set_get);
    RUN_TEST(test_player_gold_lumber_independent);
END_SUITE()
