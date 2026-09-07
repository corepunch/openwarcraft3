# Warcraft III Issued Target Order Events

OpenRealm publishes `EVENT_PLAYER_UNIT_ISSUED_TARGET_ORDER` and
`EVENT_UNIT_ISSUED_TARGET_ORDER` when an accepted target order is issued through
`G_IssueUnitTargetOrder`. The event subject is the ordered unit and the existing
JASS `source` context carries the target widget, so `GetOrderTarget`,
`GetOrderTargetUnit`, and `GetOrderTargetDestructable` resolve from the callback
that is already preserved by trigger conditions/actions.

For this compatibility slice, `GetIssuedOrderId` intentionally follows the
repository's existing `OrderId` contract, which currently converts the leading
four bytes of the string rather than Warcraft's retail numeric order table.
This keeps comparisons such as `GetIssuedOrderId() == OrderId("smart")`
consistent without broadening the patch into numeric order-ID compatibility.
The accepted order id is retained by entity number in game-side transient state.

`+set wc3_quest_debug 1` logs accepted target-order publication as
`WC3_QUEST_ORDER`. This is useful for campaign tutorial triggers such as
Prologue02's Harvest Gold step.

Point-order and immediate/no-target issued-order events, retail numeric order
IDs, and selecting-player semantics for foreign controlled units remain
separate compatibility work.

## Prologue02 Peon-stage diagnostics

With `wc3_quest_debug 1`, OpenRealm also traces the Prologue02 Town Hall / Peon
training tutorial band (trigger ordinals 95 through 106) as `WC3_QUEST_PEON`.
The trace records trigger definitions, registration event ids, enable/disable
transitions, direct `TriggerEvaluate` / `TriggerExecute` calls, dispatcher
matching, and whether `jass_calltrigger()` queued the authored actions.  A
matched, enabled trigger with `queued=0` means its authored conditions rejected
the event; `queued=1` means conditions passed and actions were scheduled.  This
is intentionally diagnostic-only and does not alter trigger behavior.
