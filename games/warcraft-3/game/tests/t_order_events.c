#ifdef BZ_TESTS
#include "test.h"
#include "../g_local.h"
#include "jass/jass.h"

LPEDICT alloc_test_unit(DWORD class_id, FLOAT x, FLOAT y);
void setup_test_world(void);
BOOL run_test_jass(LPCSTR src);

TEST(wc3_order_events, accepted_target_order_publishes_jass_context) {
    LPGAMECLIENT client = &game.clients[0];
    LPEDICT peon;
    LPEDICT target;

    setup_test_world();
    client->ps.number = 0;
    peon = alloc_test_unit(MAKEFOURCC('o','p','e','o'), 0.0f, 0.0f);
    target = alloc_test_unit(MAKEFOURCC('h','f','o','o'), 128.0f, 0.0f);
    peon->s.player = 0;
    target->s.player = 0;

    T_ASSERT(run_test_jass(
        "function order_condition takes nothing returns boolean\n"
        "  return GetIssuedOrderId() == OrderId(\"smart\") and GetOrderTargetUnit() != null\n"
        "endfunction\n"
        "function order_action takes nothing returns nothing\n"
        "  call SetPlayerState(Player(0), PLAYER_STATE_RESOURCE_GOLD, GetPlayerState(Player(0), PLAYER_STATE_RESOURCE_GOLD) + 1)\n"
        "endfunction\n"
        "function main takes nothing returns nothing\n"
        "  local trigger t = CreateTrigger()\n"
        "  call TriggerRegisterPlayerUnitEvent(t, Player(0), EVENT_PLAYER_UNIT_ISSUED_TARGET_ORDER, null)\n"
        "  call TriggerAddCondition(t, Condition(function order_condition))\n"
        "  call TriggerAddAction(t, function order_action)\n"
        "endfunction\n"));

    T_ASSERT(G_IssueUnitTargetOrder(peon, "smart", target, false, 0));
    G_RunEvents();
    jass_runevents(level.vm);
    T_EQ(client->ps.stats[PLAYERSTATE_RESOURCE_GOLD], 1);
}
#endif
