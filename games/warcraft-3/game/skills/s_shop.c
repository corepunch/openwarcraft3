#include "s_skills.h"

/* Shop system — minimal implementation.
 * WarSmash: CAbilityNeutralBuilding auto-selects heroes, CAbilitySellItems
 * handles purchase. For now, these are marker abilities that can be extended
 * when the inventory/shop UI is wired up. */

/* TODO: Shop Purchase Item (Apit) must read the sold-item list, charge the
 * player, and create the item through the authoritative lifecycle. */
static void shop_stub_command(LPEDICT clent) {
    UI_AddCancelButton(clent);
}

BZ_COMMAND_PROC(AbilityPurchaseItem, shop_stub_command)

/* Inventory is also the move identity for pickup and drop orders. */
intptr_t CAbilityInventory(LPEDICT ent, abilityMsg_t msg, abilityCall_t const *call) {
    return CAbilityPassive(ent, msg, call);
}
