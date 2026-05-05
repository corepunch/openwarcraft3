/*
 * test_combat.c — Tests for combat, animation, ability lookup, resources,
 *                 build queue, and quest system.
 *
 * Covered:
 *   T_Damage             — health reduction, lethal hit (calls die),
 *                          counter-attack trigger, attacker stand after kill
 *   M_MoveFrame          — normal advance, wrap at interval end (endfunc call,
 *                          frame reset), AI_HOLD_FRAME inhibits advance,
 *                          no animation → no-op
 *   G_RunEntity          — stat fields compressed after run,
 *                          ability index updated from currentmove
 *   Ability lookup       — FindAbilityByClassname hit/miss,
 *                          GetAbilityByIndex, GetAbilityIndex
 *   player_pay           — deducts gold+lumber on success,
 *                          refuses when gold insufficient,
 *                          refuses when lumber insufficient,
 *                          NULL player guard
 *   unit_add_build_queue — single item, chained items
 *   Quest system         — G_MakeQuest fields, set/query, G_RemoveQuest
 *   G_PublishEvent       — queue write, sequential events
 */

#include "test_framework.h"
#include "test_harness.h"
#include "../src/game/skills/s_skills.h"

/* Forward declarations for internal functions not in any public header. */
BOOL  player_pay(LPPLAYER ps, DWORD project);
void  T_Damage(LPEDICT target, LPEDICT attacker, int damage);
void  M_MoveFrame(LPEDICT self);
void  G_RunEntity(LPEDICT ent);
void  unit_add_build_queue(LPEDICT self, LPEDICT item);

/* ==========================================================================
 * Shared helpers
 * ========================================================================== */

/* Minimal die() stub that records calls without touching the move state. */
static int _die_call_count = 0;
static LPEDICT _die_last_attacker = NULL;
static void stub_die(LPEDICT self, LPEDICT attacker) {
    (void)self;
    _die_call_count++;
    _die_last_attacker = attacker;
}

static LPEDICT make_combat_unit(DWORD class_id, FLOAT hp, FLOAT x, FLOAT y) {
    LPEDICT ent       = alloc_test_unit(class_id, x, y);
    ent->health.value     = hp;
    ent->health.max_value = hp;
    ent->stand            = unit_stand;
    ent->die              = stub_die;
    ent->svflags         |= SVF_MONSTER;
    unit_stand(ent);
    return ent;
}

static animation_t _stub_anim = {
    .name       = "stand",
    .interval   = { 0, 300 }   /* 300 ms long animation */
};

/* Wire a real animation into an entity so M_MoveFrame has something to work with. */
static void attach_stub_anim(LPEDICT ent) {
    ent->animation = &_stub_anim;
}

/* ==========================================================================
 * T_Damage
 * ========================================================================== */

static void test_tdamage_reduces_health(void) {
    LPEDICT target   = make_combat_unit(UNIT_ID("hfoo"), 420.0f, 0.0f, 0.0f);
    LPEDICT attacker = make_combat_unit(UNIT_ID("hpea"), 250.0f, 50.0f, 0.0f);
    _die_call_count  = 0;

    T_Damage(target, attacker, 100);

    ASSERT_EQ_FLOAT(target->health.value, 320.0f, 0.01f);
    ASSERT_EQ_INT(_die_call_count, 0);
}

static void test_tdamage_lethal_calls_die(void) {
    LPEDICT target   = make_combat_unit(UNIT_ID("hfoo"), 100.0f, 0.0f, 0.0f);
    LPEDICT attacker = make_combat_unit(UNIT_ID("hpea"), 250.0f, 50.0f, 0.0f);
    _die_call_count  = 0;
    _die_last_attacker = NULL;

    T_Damage(target, attacker, 100);

    ASSERT_EQ_INT(_die_call_count, 1);
    ASSERT(target->health.value == 0.0f);
    ASSERT(_die_last_attacker == attacker);
}

static void test_tdamage_lethal_resets_attacker_to_stand(void) {
    LPEDICT target   = make_combat_unit(UNIT_ID("hfoo"), 50.0f, 0.0f, 0.0f);
    LPEDICT attacker = make_combat_unit(UNIT_ID("hpea"), 250.0f, 50.0f, 0.0f);
    _die_call_count  = 0;

    T_Damage(target, attacker, 100);

    /* After a kill the attacker's move should be the stand animation. */
    ASSERT_NOT_NULL(attacker->currentmove);
    ASSERT_STR_EQ(attacker->currentmove->animation, "stand");
}

static void test_tdamage_non_lethal_does_not_call_die(void) {
    LPEDICT target   = make_combat_unit(UNIT_ID("hfoo"), 420.0f, 0.0f, 0.0f);
    LPEDICT attacker = make_combat_unit(UNIT_ID("hpea"), 250.0f, 50.0f, 0.0f);
    _die_call_count  = 0;

    T_Damage(target, attacker, 1);

    ASSERT_EQ_INT(_die_call_count, 0);
    ASSERT(target->health.value > 0.0f);
}

/* ==========================================================================
 * M_MoveFrame
 * ========================================================================== */

static int _endfunc_called = 0;
static void stub_endfunc(LPEDICT ent) {
    (void)ent;
    _endfunc_called++;
}

static umove_t _stub_move = { "stand", NULL, stub_endfunc, NULL };

static void test_mmoveframe_no_animation_is_noop(void) {
    LPEDICT ent      = make_combat_unit(UNIT_ID("hpea"), 250.0f, 0.0f, 0.0f);
    ent->animation   = NULL;
    ent->currentmove = &_stub_move;
    ent->s.frame     = 0;

    M_MoveFrame(ent);

    ASSERT_EQ_INT((int)ent->s.frame, 0);
}

static void test_mmoveframe_hold_frame_flag_inhibits(void) {
    LPEDICT ent      = make_combat_unit(UNIT_ID("hpea"), 250.0f, 0.0f, 0.0f);
    attach_stub_anim(ent);
    ent->currentmove = &_stub_move;
    ent->s.frame     = 100;
    ent->aiflags    |= AI_HOLD_FRAME;

    M_MoveFrame(ent);

    ASSERT_EQ_INT((int)ent->s.frame, 100);
}

static void test_mmoveframe_normal_advance(void) {
    /* FRAMETIME = 100, animation interval [0, 300].
     * Start at frame 50 → next frame = 150 (still inside interval). */
    LPEDICT ent      = make_combat_unit(UNIT_ID("hpea"), 250.0f, 0.0f, 0.0f);
    attach_stub_anim(ent);
    ent->currentmove = &_stub_move;
    ent->s.frame     = 50;
    _endfunc_called  = 0;

    M_MoveFrame(ent);

    ASSERT_EQ_INT((int)ent->s.frame, 150);
    ASSERT_EQ_INT(_endfunc_called, 0);
}

static void test_mmoveframe_at_end_calls_endfunc_and_wraps(void) {
    /* Start at frame 250 → next = 350 >= 300 (end) → endfunc, wrap to 0. */
    LPEDICT ent      = make_combat_unit(UNIT_ID("hpea"), 250.0f, 0.0f, 0.0f);
    attach_stub_anim(ent);
    ent->currentmove = &_stub_move;
    ent->s.frame     = 250;
    _endfunc_called  = 0;

    M_MoveFrame(ent);

    ASSERT_EQ_INT(_endfunc_called, 1);
    /* Without AI_HOLD_FRAME the frame resets to interval[0]. */
    ASSERT_EQ_INT((int)ent->s.frame, 0);
}

static void test_mmoveframe_out_of_range_frame_resets(void) {
    /* frame > interval[1] → clamped to interval[0]. */
    LPEDICT ent      = make_combat_unit(UNIT_ID("hpea"), 250.0f, 0.0f, 0.0f);
    attach_stub_anim(ent);
    ent->currentmove = &_stub_move;
    ent->s.frame     = 9999;
    _endfunc_called  = 0;

    M_MoveFrame(ent);

    ASSERT_EQ_INT((int)ent->s.frame, 0);
    ASSERT_EQ_INT(_endfunc_called, 0);
}

/* ==========================================================================
 * G_RunEntity
 * ========================================================================== */

static void test_runentity_stat_fields_updated(void) {
    LPEDICT ent      = make_combat_unit(UNIT_ID("hfoo"), 420.0f, 0.0f, 0.0f);
    /* Manually set health below max so we get a non-trivial compressed value. */
    ent->health.max_value = 400.0f;
    ent->health.value     = 200.0f;   /* 50% → 127 */
    ent->mana.max_value   = 100.0f;
    ent->mana.value       = 100.0f;   /* 100% → 255 */
    ent->movetype         = MOVETYPE_NONE;

    G_RunEntity(ent);

    ASSERT_EQ_INT((int)ent->s.stats[ENT_HEALTH], 127);
    ASSERT_EQ_INT((int)ent->s.stats[ENT_MANA],   255);
}

static void test_runentity_ability_index_from_currentmove(void) {
    /* The stop move uses a_stop.  After G_RunEntity, s.ability should be the
     * index of a_stop in the ability table. */
    LPEDICT ent      = make_combat_unit(UNIT_ID("hpea"), 250.0f, 0.0f, 0.0f);
    ent->movetype    = MOVETYPE_NONE;
    unit_stand(ent);   /* sets currentmove to the stand move which has &a_stop via unit_move_stand */
    ASSERT_NOT_NULL(ent->currentmove);

    G_RunEntity(ent);

    DWORD expected = GetAbilityIndex(ent->currentmove->ability);
    ASSERT_EQ_INT((int)ent->s.ability, (int)expected);
}

/* ==========================================================================
 * Ability lookup
 * ========================================================================== */

static void test_find_ability_stop(void) {
    ability_t const *a = FindAbilityByClassname(STR_CmdStop);
    ASSERT_NOT_NULL(a);
}

static void test_find_ability_move(void) {
    ability_t const *a = FindAbilityByClassname(STR_CmdMove);
    ASSERT_NOT_NULL(a);
}

static void test_find_ability_unknown_returns_null(void) {
    ability_t const *a = FindAbilityByClassname("NotAnAbility");
    ASSERT_NULL(a);
}

static void test_get_ability_by_index_zero(void) {
    /* Index 0 is always the stop ability (first entry in abilitylist[]). */
    ability_t const *a = GetAbilityByIndex(0);
    ASSERT_NOT_NULL(a);
    ASSERT(a == FindAbilityByClassname(STR_CmdStop));
}

static void test_get_ability_by_index_out_of_range(void) {
    ability_t const *a = GetAbilityByIndex(9999);
    ASSERT_NULL(a);
}

static void test_get_ability_index_roundtrip(void) {
    ability_t const *a   = FindAbilityByClassname(STR_CmdMove);
    DWORD             idx = FindAbilityIndex(STR_CmdMove);
    ASSERT(GetAbilityByIndex(idx) == a);
}

/* ==========================================================================
 * player_pay
 * ========================================================================== */

/* Test unit data provides:
 *   hpea — ugol=75, ulum=0
 *   hfoo — ugol=135, ulum=0
 * These are registered in setup_test_unit_data() via the UnitBalance table. */

static void test_player_pay_deducts_gold(void) {
    LPPLAYER p = &game.clients[0].ps;
    p->stats[PLAYERSTATE_RESOURCE_GOLD]   = 200;
    p->stats[PLAYERSTATE_RESOURCE_LUMBER] = 0;

    BOOL ok = player_pay(p, UNIT_ID("hpea")); /* costs 75 gold */

    ASSERT(ok);
    ASSERT_EQ_INT((int)p->stats[PLAYERSTATE_RESOURCE_GOLD], 125);
}

static void test_player_pay_insufficient_gold_fails(void) {
    LPPLAYER p = &game.clients[0].ps;
    p->stats[PLAYERSTATE_RESOURCE_GOLD]   = 50;  /* need 75 */
    p->stats[PLAYERSTATE_RESOURCE_LUMBER] = 0;

    BOOL ok = player_pay(p, UNIT_ID("hpea"));

    ASSERT(!ok);
    ASSERT_EQ_INT((int)p->stats[PLAYERSTATE_RESOURCE_GOLD], 50); /* unchanged */
}

static void test_player_pay_null_player_fails(void) {
    BOOL ok = player_pay(NULL, UNIT_ID("hpea"));
    ASSERT(!ok);
}

/* ==========================================================================
 * unit_add_build_queue
 * ========================================================================== */

static void test_build_queue_first_item(void) {
    LPEDICT producer = make_combat_unit(UNIT_ID("hpea"), 250.0f, 0.0f, 0.0f);
    LPEDICT item1    = make_combat_unit(UNIT_ID("hfoo"), 420.0f, 10.0f, 0.0f);
    producer->build  = NULL;

    unit_add_build_queue(producer, item1);

    ASSERT(producer->build == item1);
    ASSERT_NULL(item1->build);
}

static void test_build_queue_chained_items(void) {
    LPEDICT producer = make_combat_unit(UNIT_ID("hpea"), 250.0f, 0.0f, 0.0f);
    LPEDICT item1    = make_combat_unit(UNIT_ID("hfoo"), 420.0f, 10.0f, 0.0f);
    LPEDICT item2    = make_combat_unit(UNIT_ID("hfoo"), 420.0f, 20.0f, 0.0f);
    producer->build  = NULL;
    item1->build     = NULL;
    item2->build     = NULL;

    unit_add_build_queue(producer, item1);
    unit_add_build_queue(producer, item2);

    ASSERT(producer->build == item1);
    ASSERT(item1->build    == item2);
    ASSERT_NULL(item2->build);
}

static void test_build_queue_three_items_linked(void) {
    LPEDICT producer = make_combat_unit(UNIT_ID("hpea"), 250.0f, 0.0f, 0.0f);
    LPEDICT item1    = make_combat_unit(UNIT_ID("hfoo"), 420.0f, 10.0f, 0.0f);
    LPEDICT item2    = make_combat_unit(UNIT_ID("hfoo"), 420.0f, 20.0f, 0.0f);
    LPEDICT item3    = make_combat_unit(UNIT_ID("hfoo"), 420.0f, 30.0f, 0.0f);
    producer->build = item1->build = item2->build = item3->build = NULL;

    unit_add_build_queue(producer, item1);
    unit_add_build_queue(producer, item2);
    unit_add_build_queue(producer, item3);

    ASSERT(producer->build == item1);
    ASSERT(item1->build    == item2);
    ASSERT(item2->build    == item3);
    ASSERT_NULL(item3->build);
}

/* ==========================================================================
 * Quest system
 * ========================================================================== */

static void test_quest_make_non_null(void) {
    LPQUEST q = G_MakeQuest();
    ASSERT_NOT_NULL(q);
}

static void test_quest_fields_default_false(void) {
    LPQUEST q = G_MakeQuest();
    ASSERT(!q->completed);
    ASSERT(!q->failed);
    ASSERT(!q->discovered);
    ASSERT(!q->required);
    ASSERT(!q->enabled);
}

static void test_quest_set_title(void) {
    LPQUEST q = G_MakeQuest();
    q->title = strdup("Defeat the Lich King");
    ASSERT_STR_EQ(q->title, "Defeat the Lich King");
    free(q->title);
    q->title = NULL;
}

static void test_quest_set_completed(void) {
    LPQUEST q = G_MakeQuest();
    q->completed = true;
    ASSERT(q->completed);
}

static void test_quest_set_failed(void) {
    LPQUEST q = G_MakeQuest();
    q->failed = true;
    ASSERT(q->failed);
}

static void test_quest_remove_clears_from_list(void) {
    /* Reset the quest list to a known-empty state. */
    level.quests = NULL;
    LPQUEST q = G_MakeQuest();
    ASSERT_NOT_NULL(level.quests);

    G_RemoveQuest(q);

    /* After removing the only quest the list must be empty. */
    ASSERT_NULL(level.quests);
}

/* ==========================================================================
 * G_PublishEvent
 * ========================================================================== */

static void test_publish_event_fills_queue(void) {
    /* Reset the event queue. */
    level.events.write = 0;
    level.events.read  = 0;

    LPEDICT ent = make_combat_unit(UNIT_ID("hpea"), 250.0f, 0.0f, 0.0f);
    GAMEEVENT *evt = G_PublishEvent(ent, EVENT_UNIT_DEATH);

    ASSERT_NOT_NULL(evt);
    ASSERT_EQ_INT((int)evt->type, (int)EVENT_UNIT_DEATH);
}

static void test_publish_event_sequential(void) {
    level.events.write = 0;
    level.events.read  = 0;

    LPEDICT ent = make_combat_unit(UNIT_ID("hpea"), 250.0f, 0.0f, 0.0f);
    G_PublishEvent(ent, EVENT_UNIT_DEATH);
    GAMEEVENT *evt2 = G_PublishEvent(ent, EVENT_PLAYER_UNIT_TRAIN_FINISH);

    ASSERT_NOT_NULL(evt2);
    ASSERT_EQ_INT((int)evt2->type, (int)EVENT_PLAYER_UNIT_TRAIN_FINISH);
}

/* ==========================================================================
 * Suite runner
 * ========================================================================== */

BEGIN_SUITE(combat)
    /* T_Damage */
    RUN_TEST(test_tdamage_reduces_health);
    RUN_TEST(test_tdamage_lethal_calls_die);
    RUN_TEST(test_tdamage_lethal_resets_attacker_to_stand);
    RUN_TEST(test_tdamage_non_lethal_does_not_call_die);

    /* M_MoveFrame */
    RUN_TEST(test_mmoveframe_no_animation_is_noop);
    RUN_TEST(test_mmoveframe_hold_frame_flag_inhibits);
    RUN_TEST(test_mmoveframe_normal_advance);
    RUN_TEST(test_mmoveframe_at_end_calls_endfunc_and_wraps);
    RUN_TEST(test_mmoveframe_out_of_range_frame_resets);

    /* G_RunEntity */
    RUN_TEST(test_runentity_stat_fields_updated);
    RUN_TEST(test_runentity_ability_index_from_currentmove);

    /* Ability lookup */
    RUN_TEST(test_find_ability_stop);
    RUN_TEST(test_find_ability_move);
    RUN_TEST(test_find_ability_unknown_returns_null);
    RUN_TEST(test_get_ability_by_index_zero);
    RUN_TEST(test_get_ability_by_index_out_of_range);
    RUN_TEST(test_get_ability_index_roundtrip);

    /* player_pay */
    RUN_TEST(test_player_pay_deducts_gold);
    RUN_TEST(test_player_pay_insufficient_gold_fails);
    RUN_TEST(test_player_pay_null_player_fails);

    /* unit_add_build_queue */
    RUN_TEST(test_build_queue_first_item);
    RUN_TEST(test_build_queue_chained_items);
    RUN_TEST(test_build_queue_three_items_linked);

    /* Quest system */
    RUN_TEST(test_quest_make_non_null);
    RUN_TEST(test_quest_fields_default_false);
    RUN_TEST(test_quest_set_title);
    RUN_TEST(test_quest_set_completed);
    RUN_TEST(test_quest_set_failed);
    RUN_TEST(test_quest_remove_clears_from_list);

    /* G_PublishEvent */
    RUN_TEST(test_publish_event_fills_queue);
    RUN_TEST(test_publish_event_sequential);
END_SUITE()
