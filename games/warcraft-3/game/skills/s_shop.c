#include "s_skills.h"

/* Shop system — minimal implementation.
 * WarSmash: CAbilityNeutralBuilding auto-selects heroes, CAbilitySellItems
 * handles purchase. For now, these are marker abilities that can be extended
 * when the inventory/shop UI is wired up. */

/* TODO: Shop Purchase Item (Apit) must read the sold-item list, charge the
 * player, and create the item through the authoritative lifecycle. */
BZ_COMMAND_PROC(AbilityPurchaseItem) {
    UI_AddCancelButton(clent);
}

/* Inventory is also the move identity for pickup and drop orders. */
BZ_ABILITY_PROC(CAbilityInventory) {
    return CAbilityPassive(ent, msg, call);
}
