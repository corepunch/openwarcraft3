/*
 * test_movement.c — Unit movement and pathfinding tests.
 *
 * Tests cover the complete move-order pipeline:
 *
 *  order_move / ai_walk integration
 *    - order_move wires up goalentity and switches to "walk" animation
 *    - unit advances toward goal each frame  (via currentmove->think)
 *    - unit transitions to "stand" once it reaches the goal
 *    - unit_movedistance matches speed × 10 / FRAMETIME
 *
 *  Waypoint helpers
 *    - Waypoint_add places a waypoint at the requested 2-D location
 *
 *  Goal-distance helper
 *    - M_DistanceToGoal returns the 2-D Euclidean distance to goalentity
 *
 * All tests use the test harness mock gi; no actual map or MPQ is needed.
 * Units are given collision = 0 for these movement tests so they don't
 * interact with each other; collision behaviour is covered in
 * test_collision.c.
 */

#include <math.h>
#include "test_framework.h"
#include "test_harness.h"

/* NAVI_THRESHOLD is the distance below which ai_walk uses direct
 * vector math rather than the heatmap flow field.  It is defined as a
 * #define in g_ai.c; we replicate the value here so test waypoints can
 * be placed within the threshold and avoid the BuildHeatmap mock path. */
#define NAVI_THRESHOLD 50.0f

/* -----------------------------------------------------------------------
 * Helpers
 * --------------------------------------------------------------------- */

/* Create a unit at (x, y) with the lifecycle callbacks and zero collision
 * (movement tests don't want unintended push-apart).  Resets entity pool
 * so each test starts from a clean slate. */
static LPEDICT make_moving_unit(FLOAT x, FLOAT y) {
    reset_entities();
    LPEDICT ent = alloc_test_unit(UNIT_ID("hpea"), x, y);
    ent->movetype  = MOVETYPE_STEP;
    ent->stand     = unit_stand;
    ent->birth     = unit_birth;
    ent->die       = unit_die;
    ent->collision = 0.0f;
    ent->health.value     = 250.0f;
    ent->health.max_value = 250.0f;
    unit_stand(ent);
    return ent;
}

/* -----------------------------------------------------------------------
 * order_move tests
 * --------------------------------------------------------------------- */

static void test_order_move_sets_goalentity(void) {
    LPEDICT unit = make_moving_unit(0.0f, 0.0f);
    LPEDICT wp   = alloc_test_unit(0, 30.0f, 0.0f); /* reuse edict as waypoint */
    order_move(unit, wp);
    ASSERT(unit->goalentity == wp);
}

static void test_order_move_sets_walk_animation(void) {
    LPEDICT unit = make_moving_unit(0.0f, 0.0f);
    LPEDICT wp   = alloc_test_unit(0, 30.0f, 0.0f);
    order_move(unit, wp);
    ASSERT_NOT_NULL(unit->currentmove);
    ASSERT_ANIM(unit, "walk");
}

/* -----------------------------------------------------------------------
 * Waypoint_add tests
 * --------------------------------------------------------------------- */

static void test_waypoint_add_sets_origin(void) {
    VECTOR2 dest = {128.0f, 256.0f};
    LPEDICT wp = Waypoint_add(&dest);
    ASSERT_NOT_NULL(wp);
    ASSERT_EQ_FLOAT(wp->s.origin.x, 128.0f, 0.01f);
    ASSERT_EQ_FLOAT(wp->s.origin.y, 256.0f, 0.01f);
}

/* -----------------------------------------------------------------------
 * unit_movedistance tests
 * --------------------------------------------------------------------- */

static void test_unit_movedistance_matches_formula(void) {
    /* unit_movedistance = 10 * speed / FRAMETIME */
    LPEDICT unit = make_moving_unit(0.0f, 0.0f);
    FLOAT expected = 10.0f * UNIT_SPEED(UNIT_ID("hpea")) / (FLOAT)FRAMETIME;
    ASSERT_FLOAT_EQ(unit_movedistance(unit), expected);
}

/* -----------------------------------------------------------------------
 * M_DistanceToGoal tests
 * --------------------------------------------------------------------- */

static void test_distance_to_goal_along_x_axis(void) {
    LPEDICT unit = make_moving_unit(0.0f, 0.0f);
    LPEDICT wp   = alloc_test_unit(0, 100.0f, 0.0f);
    unit->goalentity = wp;
    ASSERT_FLOAT_EQ(M_DistanceToGoal(unit), 100.0f);
}

static void test_distance_to_goal_diagonal(void) {
    LPEDICT unit = make_moving_unit(0.0f, 0.0f);
    LPEDICT wp   = alloc_test_unit(0, 30.0f, 40.0f); /* 3-4-5 right triangle → 50 */
    unit->goalentity = wp;
    ASSERT_EQ_FLOAT(M_DistanceToGoal(unit), 50.0f, 0.1f);
}

static void test_distance_to_goal_zero_when_at_goal(void) {
    LPEDICT unit = make_moving_unit(10.0f, 10.0f);
    LPEDICT wp   = alloc_test_unit(0, 10.0f, 10.0f);
    unit->goalentity = wp;
    ASSERT_FLOAT_EQ(M_DistanceToGoal(unit), 0.0f);
}

/* -----------------------------------------------------------------------
 * ai_walk / movement frame tests
 *
 * ai_walk is static inside s_move.c; it is accessed via the think
 * function-pointer stored in move_move_walk.  After calling order_move
 * we invoke ent->currentmove->think() to simulate one game frame.
 * ===================================================================== */

static void test_unit_moves_closer_to_goal_after_one_frame(void) {
    LPEDICT unit = make_moving_unit(0.0f, 0.0f);
    /* Place waypoint within NAVI_THRESHOLD so direct vector math is used
     * and we don't need the heatmap mock to return a meaningful direction. */
    VECTOR2 dest = {40.0f, 0.0f};
    unit_issueorder(unit, "move", &dest);
    ASSERT_NOT_NULL(unit->currentmove);
    ASSERT_NOT_NULL(unit->currentmove->think);

    FLOAT dist_before = M_DistanceToGoal(unit);
    unit->currentmove->think(unit);
    FLOAT dist_after = M_DistanceToGoal(unit);

    ASSERT(dist_after < dist_before);
}

static void test_unit_reaches_goal_and_transitions_to_stand(void) {
    LPEDICT unit = make_moving_unit(0.0f, 0.0f);
    /* Distance = 40, move_distance ≈ 27.  After two frames the unit
     * should have arrived (40 - 27 = 13 < 27) and called stand(). */
    VECTOR2 dest = {40.0f, 0.0f};
    unit_issueorder(unit, "move", &dest);

    /* Run up to 10 frames — should arrive well within that. */
    for (int i = 0; i < 10; i++) {
        if (!unit->currentmove || !unit->currentmove->think) break;
        if (strcmp(unit->currentmove->animation, "walk") != 0) break;
        unit->currentmove->think(unit);
    }

    ASSERT_ANIM(unit, "stand");
}

static void test_unit_position_changes_after_move_frame(void) {
    LPEDICT unit = make_moving_unit(0.0f, 0.0f);
    VECTOR2 dest = {40.0f, 0.0f};
    unit_issueorder(unit, "move", &dest);

    FLOAT x0 = unit->s.origin2.x;
    unit->currentmove->think(unit);

    /* Unit must have moved in the X direction. */
    ASSERT(unit->s.origin2.x > x0);
}

static void test_unit_does_not_overshoot_goal(void) {
    LPEDICT unit = make_moving_unit(0.0f, 0.0f);
    VECTOR2 dest = {40.0f, 0.0f};
    unit_issueorder(unit, "move", &dest);

    /* Run frames until the unit stands. */
    for (int i = 0; i < 20; i++) {
        if (!unit->currentmove || !unit->currentmove->think) break;
        if (strcmp(unit->currentmove->animation, "walk") != 0) break;
        unit->currentmove->think(unit);
    }

    /* After reaching the goal the unit should be roughly at the waypoint,
     * not far past it. */
    FLOAT dist = M_DistanceToGoal(unit);
    FLOAT move_dist = unit_movedistance(unit);
    ASSERT(dist <= move_dist + 1.0f);
}

/* -----------------------------------------------------------------------
 * Suite runner
 * --------------------------------------------------------------------- */

BEGIN_SUITE(movement)
    RUN_TEST(test_order_move_sets_goalentity);
    RUN_TEST(test_order_move_sets_walk_animation);
    RUN_TEST(test_waypoint_add_sets_origin);
    RUN_TEST(test_unit_movedistance_matches_formula);
    RUN_TEST(test_distance_to_goal_along_x_axis);
    RUN_TEST(test_distance_to_goal_diagonal);
    RUN_TEST(test_distance_to_goal_zero_when_at_goal);
    RUN_TEST(test_unit_moves_closer_to_goal_after_one_frame);
    RUN_TEST(test_unit_reaches_goal_and_transitions_to_stand);
    RUN_TEST(test_unit_position_changes_after_move_frame);
    RUN_TEST(test_unit_does_not_overshoot_goal);
END_SUITE()
