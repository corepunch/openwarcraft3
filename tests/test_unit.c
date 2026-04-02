/*
 * test_unit.c — Unit lifecycle and order tests.
 *
 * All tests create raw edict_t structs (no full SP_SpawnUnit path) and
 * exercise the behaviour functions from m_unit.c directly.  The test
 * harness supplies mock gi callbacks so no MPQ or renderer is needed.
 *
 * Covered scenarios:
 *   birth    — currentmove set to "birth" animation, wait > 0
 *   stand    — currentmove set to "stand" animation, no renderfx flag
 *   die      — currentmove set to "death", SVF_DEADMONSTER raised,
 *              UNIT_DEATH event published to level.events.queue
 *   issueorder "move"   — goalentity created, currentmove "walk"
 *   issueorder "stop"   — unit returns to stand
 *   issueimmediateorder — "stop" triggers order_stop
 *   additem / additemtoslot — inventory management
 */

#include "test_framework.h"
#include "test_harness.h"

/* -----------------------------------------------------------------------
 * Helpers
 * --------------------------------------------------------------------- */

static DWORD hpea_id(void) {
    DWORD id; memcpy(&id, "hpea", 4); return id;
}

/*
 * Create a minimal unit edict with the lifecycle callbacks wired up, as
 * SP_monster_unit would do — but without the full SP_SpawnUnit data-load.
 * Resets the entity pool so each test function starts from a clean slate.
 */
static LPEDICT make_unit(FLOAT x, FLOAT y) {
    reset_entities();
    LPEDICT ent = alloc_test_unit(hpea_id(), x, y);
    ent->movetype  = MOVETYPE_STEP;
    ent->stand     = unit_stand;
    ent->birth     = unit_birth;
    ent->die       = unit_die;
    ent->health.value     = 250.0f;
    ent->health.max_value = 250.0f;
    /* Put the unit in stand state initially. */
    unit_stand(ent);
    return ent;
}

/* -----------------------------------------------------------------------
 * Birth tests
 * --------------------------------------------------------------------- */

static void test_unit_birth_sets_birth_animation(void) {
    LPEDICT ent = make_unit(0, 0);
    unit_birth(ent);
    ASSERT_NOT_NULL(ent->currentmove);
    ASSERT_STR_EQ(ent->currentmove->animation, "birth");
}

static void test_unit_birth_sets_wait_from_build_time(void) {
    /* UNIT_BUILD_TIME(hpea) == 45 (set up in test harness).
     * unit_birth stores the build time directly in ent->wait. */
    LPEDICT ent = make_unit(0, 0);
    unit_birth(ent);
    ASSERT(ent->wait > 0);
    ASSERT_EQ_INT((int)ent->wait, UNIT_BUILD_TIME(hpea_id()));
}

static void test_unit_birth_sets_no_ubersplat_flag(void) {
    LPEDICT ent = make_unit(0, 0);
    unit_birth(ent);
    ASSERT(ent->s.renderfx & RF_NO_UBERSPLAT);
}

/* -----------------------------------------------------------------------
 * Stand tests
 * --------------------------------------------------------------------- */

static void test_unit_stand_sets_stand_animation(void) {
    LPEDICT ent = make_unit(0, 0);
    unit_stand(ent);
    ASSERT_NOT_NULL(ent->currentmove);
    ASSERT_STR_EQ(ent->currentmove->animation, "stand");
}

static void test_unit_stand_clears_build_pointer(void) {
    LPEDICT ent   = make_unit(0, 0);
    /* Pretend the unit was building something. */
    ent->build = ent; /* non-NULL sentinel */
    unit_stand(ent);
    ASSERT_NULL(ent->build);
}

static void test_unit_stand_clears_no_ubersplat_flag(void) {
    LPEDICT ent = make_unit(0, 0);
    ent->s.renderfx |= RF_NO_UBERSPLAT;
    unit_stand(ent);
    ASSERT(!(ent->s.renderfx & RF_NO_UBERSPLAT));
}

/* -----------------------------------------------------------------------
 * Die tests
 * --------------------------------------------------------------------- */

static void test_unit_die_sets_death_animation(void) {
    LPEDICT ent = make_unit(0, 0);
    unit_die(ent, NULL);
    ASSERT_NOT_NULL(ent->currentmove);
    ASSERT_STR_EQ(ent->currentmove->animation, "death");
}

static void test_unit_die_raises_dead_monster_flag(void) {
    LPEDICT ent = make_unit(0, 0);
    unit_die(ent, NULL);
    ASSERT(ent->svflags & SVF_DEADMONSTER);
}

static void test_unit_die_publishes_death_event(void) {
    LPEDICT ent = make_unit(0, 0);
    /* Clear any existing events. */
    memset(level.events.queue, 0, sizeof(level.events.queue));
    level.events.handlers = NULL;

    unit_die(ent, NULL);
    /* G_PublishEvent stores events in level.events.queue. */
    BOOL found = false;
    for (int i = 0; i < MAX_EVENT_QUEUE; i++) {
        if (level.events.queue[i].type == EVENT_UNIT_DEATH) {
            found = true;
            break;
        }
    }
    ASSERT(found);
}

/* -----------------------------------------------------------------------
 * Order tests
 * --------------------------------------------------------------------- */

static void test_issueorder_move_creates_waypoint(void) {
    LPEDICT ent = make_unit(0, 0);
    VECTOR2 dest = {100.0f, 0.0f};
    BOOL result = unit_issueorder(ent, "move", &dest);
    ASSERT(result);
    ASSERT_NOT_NULL(ent->goalentity);
}

static void test_issueorder_move_sets_walk_animation(void) {
    LPEDICT ent = make_unit(0, 0);
    VECTOR2 dest = {100.0f, 0.0f};
    unit_issueorder(ent, "move", &dest);
    ASSERT_NOT_NULL(ent->currentmove);
    ASSERT_STR_EQ(ent->currentmove->animation, "walk");
}

static void test_issueorder_unknown_returns_false(void) {
    LPEDICT ent = make_unit(0, 0);
    VECTOR2 dest = {100.0f, 0.0f};
    BOOL result = unit_issueorder(ent, "patrol", &dest);
    ASSERT(!result);
}

static void test_issueimmediateorder_stop(void) {
    LPEDICT ent = make_unit(0, 0);
    /* Put unit in walk state first. */
    VECTOR2 dest = {100.0f, 0.0f};
    unit_issueorder(ent, "move", &dest);
    ASSERT_STR_EQ(ent->currentmove->animation, "walk");

    BOOL result = unit_issueimmediateorder(ent, "stop");
    ASSERT(result);
    ASSERT_STR_EQ(ent->currentmove->animation, "stand");
}

static void test_issueimmediateorder_unknown_returns_false(void) {
    LPEDICT ent = make_unit(0, 0);
    BOOL result = unit_issueimmediateorder(ent, "patrol");
    ASSERT(!result);
}

/* -----------------------------------------------------------------------
 * Inventory tests
 * --------------------------------------------------------------------- */

static void test_additemtoslot_fills_empty_slot(void) {
    LPEDICT ent = make_unit(0, 0);
    DWORD item; memcpy(&item, "ratf", 4);
    BOOL ok = unit_additemtoslot(ent, item, 0);
    ASSERT(ok);
    ASSERT_EQ_INT(ent->inventory[0], (long long)item);
}

static void test_additemtoslot_rejects_occupied_slot(void) {
    LPEDICT ent = make_unit(0, 0);
    DWORD item; memcpy(&item, "ratf", 4);
    unit_additemtoslot(ent, item, 0);
    BOOL ok = unit_additemtoslot(ent, item, 0); /* same slot */
    ASSERT(!ok);
}

static void test_additem_fills_first_free_slot(void) {
    LPEDICT ent = make_unit(0, 0);
    DWORD a; memcpy(&a, "ratf", 4);
    DWORD b; memcpy(&b, "rde2", 4);
    unit_additemtoslot(ent, a, 0);
    BOOL ok = unit_additem(ent, b);
    ASSERT(ok);
    ASSERT_EQ_INT(ent->inventory[1], (long long)b);
}

static void test_additem_fails_when_inventory_full(void) {
    LPEDICT ent = make_unit(0, 0);
    DWORD dummy; memcpy(&dummy, "ratf", 4);
    for (int i = 0; i < MAX_INVENTORY; i++)
        unit_additemtoslot(ent, dummy, i);
    DWORD extra; memcpy(&extra, "rde2", 4);
    BOOL ok = unit_additem(ent, extra);
    ASSERT(!ok);
}

/* -----------------------------------------------------------------------
 * Suite runner
 * --------------------------------------------------------------------- */

void run_unit_tests(void) {
    setup_game();

    RUN_TEST(test_unit_birth_sets_birth_animation);
    RUN_TEST(test_unit_birth_sets_wait_from_build_time);
    RUN_TEST(test_unit_birth_sets_no_ubersplat_flag);

    RUN_TEST(test_unit_stand_sets_stand_animation);
    RUN_TEST(test_unit_stand_clears_build_pointer);
    RUN_TEST(test_unit_stand_clears_no_ubersplat_flag);

    RUN_TEST(test_unit_die_sets_death_animation);
    RUN_TEST(test_unit_die_raises_dead_monster_flag);
    RUN_TEST(test_unit_die_publishes_death_event);

    RUN_TEST(test_issueorder_move_creates_waypoint);
    RUN_TEST(test_issueorder_move_sets_walk_animation);
    RUN_TEST(test_issueorder_unknown_returns_false);
    RUN_TEST(test_issueimmediateorder_stop);
    RUN_TEST(test_issueimmediateorder_unknown_returns_false);

    RUN_TEST(test_additemtoslot_fills_empty_slot);
    RUN_TEST(test_additemtoslot_rejects_occupied_slot);
    RUN_TEST(test_additem_fills_first_free_slot);
    RUN_TEST(test_additem_fails_when_inventory_full);

    teardown_game();
}
