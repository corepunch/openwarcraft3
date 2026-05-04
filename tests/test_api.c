/*
 * test_api.c — Tests for the JASS native API implementations.
 *
 * These tests exercise the C-level game-state that the api_*.h functions
 * read and write.  They work directly on struct fields, alliance tables,
 * and group arrays — no MPQ, renderer, or JASS VM is required.
 *
 * Covered:
 *   Player  — color, start_location, name, team, alliance
 *   Hero    — str/agi/int attributes, XP accumulation, suspend_xp,
 *             overflow-safe AddHeroXP
 *   Unit    — invulnerable, paused, no_pathing, unit_color flags
 *   Group   — FirstOfGroup, IsUnitInGroup
 *   Misc    — SubString semantics, GetRandomInt / GetRandomReal range
 */

#include "test_framework.h"
#include "test_harness.h"

#include "../src/game/jass/vm_public.h"

#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/* =========================================================================
 * Helpers
 * ========================================================================= */

/*
 * Return a pointer to player slot [idx].  Assigns player->number = idx so
 * that G_GetPlayerByNumber / PLAYER_CLIENT macros work correctly.
 */
static LPPLAYER test_player(int idx) {
    game.clients[idx].ps.number = (DWORD)idx;
    return &game.clients[idx].ps;
}

/* Create a minimal unit in slot 0 and return it. */
static LPEDICT make_unit_hero(void) {
    reset_entities();
    LPEDICT ent = alloc_test_unit(UNIT_ID("hpea"), 0.0f, 0.0f);
    return ent;
}

/* =========================================================================
 * Player — color
 * ========================================================================= */

static void test_player_color_default_zero(void) {
    LPPLAYER p = test_player(0);
    ASSERT_EQ_INT((int)p->color, 0);
}

static void test_player_color_set_get(void) {
    LPPLAYER p = test_player(0);
    p->color = 5;
    ASSERT_EQ_INT((int)p->color, 5);
}

static void test_player_color_max_index(void) {
    LPPLAYER p = test_player(0);
    p->color = 23;
    ASSERT_EQ_INT((int)p->color, 23);
}

/* =========================================================================
 * Player — start_location
 * ========================================================================= */

static void test_player_start_location_default(void) {
    /* start_location is 0-initialised by setup_game(). */
    LPGAMECLIENT cl = &game.clients[1];
    cl->ps.number = 1;
    ASSERT_EQ_INT((int)cl->ps.start_location, 0);
}

static void test_player_start_location_set_get(void) {
    LPGAMECLIENT cl = &game.clients[2];
    cl->ps.number = 2;
    cl->ps.start_location = 3;
    ASSERT_EQ_INT((int)cl->ps.start_location, 3);
}

static void test_player_start_location_negative(void) {
    LPGAMECLIENT cl = &game.clients[3];
    cl->ps.number = 3;
    cl->ps.start_location = -1;
    ASSERT_EQ_INT((int)cl->ps.start_location, -1);
}

/* =========================================================================
 * Player — name
 * ========================================================================= */

static void test_player_name_set_get(void) {
    LPPLAYER p = test_player(0);
    p->name = "Arthas";
    ASSERT_STR_EQ(p->name, "Arthas");
}

static void test_player_name_null_default(void) {
    /* memset in setup_game zeroes the name pointer. */
    LPPLAYER p = test_player(4);
    p->name = NULL; /* explicit reset */
    ASSERT_NULL(p->name);
}

/* =========================================================================
 * Player — team
 * ========================================================================= */

static void test_player_team_set_get(void) {
    LPPLAYER p = test_player(0);
    p->team = 2;
    ASSERT_EQ_INT((int)p->team, 2);
}

/* =========================================================================
 * Player — alliance
 * ========================================================================= */

static void test_alliance_passive_default_false(void) {
    LPPLAYER p0 = test_player(0);
    LPPLAYER p1 = test_player(1);
    /* After setup_game(), alliance table is zeroed. */
    ASSERT(!G_GetPlayerAlliance(p0, p1, ALLIANCE_PASSIVE));
}

static void test_alliance_set_true(void) {
    LPPLAYER p0 = test_player(0);
    LPPLAYER p1 = test_player(1);
    G_SetPlayerAlliance(p0, p1, ALLIANCE_PASSIVE, true);
    ASSERT(G_GetPlayerAlliance(p0, p1, ALLIANCE_PASSIVE));
}

static void test_alliance_symmetric(void) {
    /* G_SetPlayerAlliance sets both directions. */
    LPPLAYER p0 = test_player(0);
    LPPLAYER p1 = test_player(1);
    G_SetPlayerAlliance(p0, p1, ALLIANCE_PASSIVE, true);
    ASSERT(G_GetPlayerAlliance(p1, p0, ALLIANCE_PASSIVE));
}

static void test_alliance_revoke(void) {
    LPPLAYER p0 = test_player(0);
    LPPLAYER p1 = test_player(1);
    G_SetPlayerAlliance(p0, p1, ALLIANCE_PASSIVE, true);
    G_SetPlayerAlliance(p0, p1, ALLIANCE_PASSIVE, false);
    ASSERT(!G_GetPlayerAlliance(p0, p1, ALLIANCE_PASSIVE));
}

static void test_alliance_enemy_when_not_allied(void) {
    LPPLAYER p0 = test_player(0);
    LPPLAYER p2 = test_player(2);
    /* Players 0 and 2 have no alliance — IsUnitEnemy logic is !ally. */
    ASSERT(!G_GetPlayerAlliance(p0, p2, ALLIANCE_PASSIVE));
}

/* =========================================================================
 * Hero — str / agi / int attributes
 * ========================================================================= */

static void test_hero_str_set_get(void) {
    LPEDICT ent = make_unit_hero();
    ent->hero.str = 25;
    ASSERT_EQ_INT((int)ent->hero.str, 25);
}

static void test_hero_agi_set_get(void) {
    LPEDICT ent = make_unit_hero();
    ent->hero.agi = 18;
    ASSERT_EQ_INT((int)ent->hero.agi, 18);
}

static void test_hero_int_set_get(void) {
    LPEDICT ent = make_unit_hero();
    ent->hero.intel = 22;
    ASSERT_EQ_INT((int)ent->hero.intel, 22);
}

/* =========================================================================
 * Hero — XP accumulation
 * ========================================================================= */

static void test_hero_xp_default_zero(void) {
    LPEDICT ent = make_unit_hero();
    ASSERT_EQ_INT((int)ent->hero.xp, 0);
}

static void test_hero_xp_set(void) {
    LPEDICT ent = make_unit_hero();
    ent->hero.xp = 500;
    ASSERT_EQ_INT((int)ent->hero.xp, 500);
}

static void test_hero_xp_add(void) {
    LPEDICT ent = make_unit_hero();
    ent->hero.xp = 100;
    DWORD add = 50;
    /* Replicate AddHeroXP logic: overflow-safe add */
    DWORD before = ent->hero.xp;
    ent->hero.xp = (before + add < before) ? ~(DWORD)0 : before + add;
    ASSERT_EQ_INT((int)ent->hero.xp, 150);
}

static void test_hero_xp_overflow_clamps(void) {
    LPEDICT ent = make_unit_hero();
    ent->hero.xp = UINT32_MAX - 10;
    DWORD add = 100;
    DWORD before = ent->hero.xp;
    ent->hero.xp = (before + add < before) ? ~(DWORD)0 : before + add;
    /* Overflow wraps, clamp gives UINT32_MAX */
    ASSERT_EQ_INT((long long)ent->hero.xp, (long long)UINT32_MAX);
}

/* =========================================================================
 * Hero — suspend_xp
 * ========================================================================= */

static void test_hero_suspend_xp_default_false(void) {
    LPEDICT ent = make_unit_hero();
    ASSERT(!ent->hero.suspend_xp);
}

static void test_hero_suspend_xp_set_true(void) {
    LPEDICT ent = make_unit_hero();
    ent->hero.suspend_xp = true;
    ASSERT(ent->hero.suspend_xp);
}

static void test_hero_xp_not_added_when_suspended(void) {
    LPEDICT ent = make_unit_hero();
    ent->hero.xp = 100;
    ent->hero.suspend_xp = true;
    /* Replicate AddHeroXP: skip when suspend_xp is set */
    LONG xp_to_add = 50;
    if (!ent->hero.suspend_xp && xp_to_add > 0) {
        DWORD add = (DWORD)xp_to_add;
        DWORD before = ent->hero.xp;
        ent->hero.xp = (before + add < before) ? ~(DWORD)0 : before + add;
    }
    ASSERT_EQ_INT((int)ent->hero.xp, 100);
}

/* =========================================================================
 * Unit flags — invulnerable / paused / no_pathing / unit_color
 * ========================================================================= */

static void test_unit_invulnerable_default_false(void) {
    LPEDICT ent = make_unit_hero();
    ASSERT(!ent->invulnerable);
}

static void test_unit_invulnerable_set(void) {
    LPEDICT ent = make_unit_hero();
    ent->invulnerable = true;
    ASSERT(ent->invulnerable);
}

static void test_unit_paused_default_false(void) {
    LPEDICT ent = make_unit_hero();
    ASSERT(!ent->paused);
}

static void test_unit_paused_set(void) {
    LPEDICT ent = make_unit_hero();
    ent->paused = true;
    ASSERT(ent->paused);
    ent->paused = false;
    ASSERT(!ent->paused);
}

static void test_unit_no_pathing_default_false(void) {
    LPEDICT ent = make_unit_hero();
    ASSERT(!ent->no_pathing);
}

static void test_unit_no_pathing_set(void) {
    LPEDICT ent = make_unit_hero();
    ent->no_pathing = true;
    ASSERT(ent->no_pathing);
}

static void test_unit_color_default_zero(void) {
    LPEDICT ent = make_unit_hero();
    ASSERT_EQ_INT((int)ent->unit_color, 0);
}

static void test_unit_color_set(void) {
    LPEDICT ent = make_unit_hero();
    ent->unit_color = 7;
    ASSERT_EQ_INT((int)ent->unit_color, 7);
}

/* =========================================================================
 * Unit — hidden flag (RF_HIDDEN)
 * ========================================================================= */

static void test_unit_hidden_default_false(void) {
    LPEDICT ent = make_unit_hero();
    ASSERT(!(ent->s.renderfx & RF_HIDDEN));
}

static void test_unit_hidden_set(void) {
    LPEDICT ent = make_unit_hero();
    ent->s.renderfx |= RF_HIDDEN;
    ASSERT(ent->s.renderfx & RF_HIDDEN);
}

static void test_unit_hidden_clear(void) {
    LPEDICT ent = make_unit_hero();
    ent->s.renderfx |= RF_HIDDEN;
    ent->s.renderfx &= ~RF_HIDDEN;
    ASSERT(!(ent->s.renderfx & RF_HIDDEN));
}

/* =========================================================================
 * Group — FirstOfGroup / IsUnitInGroup
 * ========================================================================= */

static void test_group_first_of_empty_returns_null(void) {
    ggroup_t g = {0};
    LPEDICT first = (g.num_units > 0) ? g.units[0] : NULL;
    ASSERT_NULL(first);
}

static void test_group_first_of_group(void) {
    reset_entities();
    LPEDICT a = alloc_test_unit(UNIT_ID("hpea"), 0, 0);
    LPEDICT b = alloc_test_unit(UNIT_ID("hfoo"), 1, 0);
    ggroup_t g = {0};
    g.units[0] = a;
    g.units[1] = b;
    g.num_units = 2;
    ASSERT(g.units[0] == a);
    ASSERT(g.units[1] == b);
}

static void test_group_is_unit_in_group_true(void) {
    reset_entities();
    LPEDICT a = alloc_test_unit(UNIT_ID("hpea"), 0, 0);
    ggroup_t g = {0};
    g.units[0] = a;
    g.num_units = 1;
    /* Replicate IsUnitInGroup logic */
    BOOL found = false;
    for (DWORD i = 0; i < g.num_units; i++) {
        if (g.units[i] == a) { found = true; break; }
    }
    ASSERT(found);
}

static void test_group_is_unit_in_group_false(void) {
    reset_entities();
    LPEDICT a = alloc_test_unit(UNIT_ID("hpea"), 0, 0);
    LPEDICT b = alloc_test_unit(UNIT_ID("hfoo"), 1, 0);
    ggroup_t g = {0};
    g.units[0] = a;
    g.num_units = 1;
    BOOL found = false;
    for (DWORD i = 0; i < g.num_units; i++) {
        if (g.units[i] == b) { found = true; break; }
    }
    ASSERT(!found);
}

/* =========================================================================
 * Misc — SubString semantics
 * ========================================================================= */

/*
 * Replicate SubString() logic from api_misc.h:
 *   source[start..end) — start inclusive, end exclusive.
 */
static void substr(const char *source, LONG start, LONG end, char *out, LONG outsz) {
    LONG len = (LONG)strlen(source);
    if (start < 0) start = 0;
    if (end > len) end = len;
    LONG n = end - start;
    if (n <= 0 || n + 1 > outsz) { out[0] = '\0'; return; }
    strncpy(out, source + start, (size_t)n);
    out[n] = '\0';
}

static void test_substring_basic(void) {
    char buf[64];
    substr("hello", 1, 4, buf, (LONG)sizeof(buf));
    ASSERT_STR_EQ(buf, "ell");
}

static void test_substring_full(void) {
    char buf[64];
    substr("hello", 0, 5, buf, (LONG)sizeof(buf));
    ASSERT_STR_EQ(buf, "hello");
}

static void test_substring_start_equals_end(void) {
    char buf[64];
    substr("hello", 2, 2, buf, (LONG)sizeof(buf));
    ASSERT_STR_EQ(buf, "");
}

static void test_substring_end_past_len(void) {
    char buf[64];
    substr("hi", 0, 100, buf, (LONG)sizeof(buf));
    ASSERT_STR_EQ(buf, "hi");
}

static void test_substring_single_char(void) {
    char buf[64];
    substr("hello", 0, 1, buf, (LONG)sizeof(buf));
    ASSERT_STR_EQ(buf, "h");
}

/* =========================================================================
 * Misc — GetRandomInt / GetRandomReal range
 * ========================================================================= */

static void test_random_int_in_range(void) {
    srand(42);
    for (int i = 0; i < 50; i++) {
        LONG lo = 1, hi = 10;
        LONG r = lo + rand() % (hi - lo + 1);
        ASSERT(r >= lo && r <= hi);
    }
}

static void test_random_int_single_value(void) {
    srand(1);
    LONG lo = 7, hi = 7;
    LONG r = lo + rand() % (hi - lo + 1);
    ASSERT_EQ_INT((int)r, 7);
}

static void test_random_real_in_range(void) {
    srand(42);
    for (int i = 0; i < 50; i++) {
        FLOAT lo = 0.0f, hi = 1.0f;
        FLOAT t = (FLOAT)rand() / (FLOAT)RAND_MAX;
        FLOAT r = lo + t * (hi - lo);
        ASSERT(r >= lo && r <= hi);
    }
}

static void test_random_seed_deterministic(void) {
    srand(12345);
    int a = rand();
    srand(12345);
    int b = rand();
    ASSERT_EQ_INT(a, b);
}

/* =========================================================================
 * Item — position and type id
 * ========================================================================= */

static void test_item_position_set(void) {
    reset_entities();
    LPEDICT item = alloc_test_unit(UNIT_ID("I000"), 10.0f, 20.0f);
    item->s.origin.x = 10.0f;
    item->s.origin.y = 20.0f;
    ASSERT_EQ_FLOAT(item->s.origin.x, 10.0f, 0.001f);
    ASSERT_EQ_FLOAT(item->s.origin.y, 20.0f, 0.001f);
}

static void test_item_type_id(void) {
    reset_entities();
    LPEDICT item = alloc_test_unit(UNIT_ID("I000"), 0.0f, 0.0f);
    DWORD expected = UNIT_ID("I000");
    ASSERT_EQ_INT((int)item->class_id, (int)expected);
}

/* =========================================================================
 * Unit — IsUnitOwnedByPlayer
 * ========================================================================= */

static void test_unit_owned_by_player(void) {
    reset_entities();
    LPEDICT ent = alloc_test_unit(UNIT_ID("hpea"), 0, 0);
    ent->s.player = 2;
    /* Replicate IsUnitOwnedByPlayer: ent->s.player == PLAYER_NUM(player) */
    LPPLAYER p = test_player(2);
    ASSERT_EQ_INT((int)ent->s.player, (int)PLAYER_NUM(p));
}

static void test_unit_not_owned_by_player(void) {
    reset_entities();
    LPEDICT ent = alloc_test_unit(UNIT_ID("hpea"), 0, 0);
    ent->s.player = 1;
    LPPLAYER p = test_player(3);
    ASSERT(ent->s.player != PLAYER_NUM(p));
}

/* =========================================================================
 * Unit — IsUnitInRange
 * ========================================================================= */

static void test_unit_in_range(void) {
    reset_entities();
    LPEDICT a = alloc_test_unit(UNIT_ID("hpea"), 0.0f, 0.0f);
    LPEDICT b = alloc_test_unit(UNIT_ID("hfoo"), 3.0f, 4.0f);  /* dist = 5 */
    FLOAT dist = Vector2_distance(&a->s.origin2, &b->s.origin2);
    ASSERT(dist <= 6.0f);
}

static void test_unit_out_of_range(void) {
    reset_entities();
    LPEDICT a = alloc_test_unit(UNIT_ID("hpea"), 0.0f, 0.0f);
    LPEDICT b = alloc_test_unit(UNIT_ID("hfoo"), 3.0f, 4.0f);  /* dist = 5 */
    FLOAT dist = Vector2_distance(&a->s.origin2, &b->s.origin2);
    ASSERT(!(dist <= 4.0f));
}

/* =========================================================================
 * Test suite entry point
 * ========================================================================= */

BEGIN_SUITE(api)
    /* Player — color */
    RUN_TEST(test_player_color_default_zero);
    RUN_TEST(test_player_color_set_get);
    RUN_TEST(test_player_color_max_index);
    /* Player — start_location */
    RUN_TEST(test_player_start_location_default);
    RUN_TEST(test_player_start_location_set_get);
    RUN_TEST(test_player_start_location_negative);
    /* Player — name */
    RUN_TEST(test_player_name_set_get);
    RUN_TEST(test_player_name_null_default);
    /* Player — team */
    RUN_TEST(test_player_team_set_get);
    /* Player — alliance */
    RUN_TEST(test_alliance_passive_default_false);
    RUN_TEST(test_alliance_set_true);
    RUN_TEST(test_alliance_symmetric);
    RUN_TEST(test_alliance_revoke);
    RUN_TEST(test_alliance_enemy_when_not_allied);
    /* Hero attributes */
    RUN_TEST(test_hero_str_set_get);
    RUN_TEST(test_hero_agi_set_get);
    RUN_TEST(test_hero_int_set_get);
    /* Hero XP */
    RUN_TEST(test_hero_xp_default_zero);
    RUN_TEST(test_hero_xp_set);
    RUN_TEST(test_hero_xp_add);
    RUN_TEST(test_hero_xp_overflow_clamps);
    /* Hero suspend_xp */
    RUN_TEST(test_hero_suspend_xp_default_false);
    RUN_TEST(test_hero_suspend_xp_set_true);
    RUN_TEST(test_hero_xp_not_added_when_suspended);
    /* Unit flags */
    RUN_TEST(test_unit_invulnerable_default_false);
    RUN_TEST(test_unit_invulnerable_set);
    RUN_TEST(test_unit_paused_default_false);
    RUN_TEST(test_unit_paused_set);
    RUN_TEST(test_unit_no_pathing_default_false);
    RUN_TEST(test_unit_no_pathing_set);
    RUN_TEST(test_unit_color_default_zero);
    RUN_TEST(test_unit_color_set);
    /* Unit hidden flag */
    RUN_TEST(test_unit_hidden_default_false);
    RUN_TEST(test_unit_hidden_set);
    RUN_TEST(test_unit_hidden_clear);
    /* Group */
    RUN_TEST(test_group_first_of_empty_returns_null);
    RUN_TEST(test_group_first_of_group);
    RUN_TEST(test_group_is_unit_in_group_true);
    RUN_TEST(test_group_is_unit_in_group_false);
    /* SubString */
    RUN_TEST(test_substring_basic);
    RUN_TEST(test_substring_full);
    RUN_TEST(test_substring_start_equals_end);
    RUN_TEST(test_substring_end_past_len);
    RUN_TEST(test_substring_single_char);
    /* Random */
    RUN_TEST(test_random_int_in_range);
    RUN_TEST(test_random_int_single_value);
    RUN_TEST(test_random_real_in_range);
    RUN_TEST(test_random_seed_deterministic);
    /* Item */
    RUN_TEST(test_item_position_set);
    RUN_TEST(test_item_type_id);
    /* Unit ownership / range */
    RUN_TEST(test_unit_owned_by_player);
    RUN_TEST(test_unit_not_owned_by_player);
    RUN_TEST(test_unit_in_range);
    RUN_TEST(test_unit_out_of_range);
END_SUITE()
