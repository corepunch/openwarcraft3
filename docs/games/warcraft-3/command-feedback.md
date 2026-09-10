# Command feedback

## Point confirmation

Warcraft point-order acknowledgement uses `UI\Feedback\Confirmation\Confirmation.mdx` as a transient client-local effect. The server emits only the accepted world point and whether the accepted command is an attack-point command; the client reuses the loaded model, grounds it to the WC3 support surface at that point, advances the authored animation by elapsed time, and drops it after the existing one-second confirmation lifetime. Ordinary terrain uses the heightmap; live walkable destructables such as bridges reuse the renderer's `RF_GROUND_CONFORM` path so authored MDX deck geometry can raise the marker above the terrain below it.

- ordinary Move/SmartPoint, Rally point, Patrol, and successful point-target spells use green `(0,255,0)` tint;
- explicit Attack-to-point / attack-move uses red `(255,0,0)` tint;
- entity-target Smart clicks use the separate widget-indicator path described below;
- the confirmation is independent of persistent Rally/waypoint presentation.

The generic temporary-event IDs append `TE_ATTACK_CONFIRMATION` after the pre-existing event values so existing missile/impact protocol numbers do not change.

## Entity-target Smart confirmation

Retail Warcraft III briefly flashes the normal selection-circle ring on a unit/building widget after a successful Smart/right-click target order. This is not `TargetPointConfirm`: OpenRealm reuses the existing `TE_ENTITY_INDICATOR` path already used by JASS `AddIndicator`, so the marker follows the target and owns no simulation state. The current client timing is the existing one-second, two-flash indicator lifetime; the two-flash behavior is established, while the exact 250 ms on/off duty cycle remains an OpenRealm approximation rather than a measured 1.29.2 constant.

The issuing player's relationship selects the ring colour. `G_SmartTargetIndicatorColor` reads `[SelectionCircle]` from the already-loaded `UI\MiscData.txt` cache. Warcraft stores these fields as `A,R,G,B`:

| Relationship | MiscData key | Stock 1.29 colour |
|---|---|---|
| own / shared control | `ColorFriend` | `255,0,255,0` -> green `(0,255,0,255)` |
| passive ally without shared control | `ColorNeutral` | `255,255,255,0` -> yellow `(255,255,0,255)` |
| hostile | `ColorEnemy` | `255,255,0,0` -> red `(255,0,0,255)` |

Minimal/test data sets that omit the section use those stock values as the explicit hardcoded fallback. Relationship classification stays centralized in `G_SelectionRelation`; Smart feedback must not maintain a second alliance policy.

The acknowledgement is emitted once per accepted player click, even when several selected units consume the same Smart command or the command is Shift-queued. Rejected Smart target orders emit no indicator. The current implementation deliberately limits automatic Smart confirmation to `SVF_MONSTER` targets (units and buildings). Retail colours/semantics for items and destructables have not been established strongly enough to invent them here; their existing behavior is unchanged.

## Persistent command-state markers

Rally uses the generic world-indicator protocol documented in [server-selected-effects.md](../../architecture/server-selected-effects.md) and the gameplay/presentation contract in [rally-points.md](rally-points.md). It is rebuilt from authoritative Rally state instead of being kept alive by the original mouse click.

Warsmash-style queued `WaypointIndicator` flags remain unimplemented because the current OpenRealm order model does not yet expose an equivalent queued player-command target list. Internal movement route waypoints must not be rendered as a substitute.

## Validation

When validating manually:

1. Move/SmartPoint should still show the green confirmation.
2. Attack-to-point should show the same model tinted red.
3. Rally-to-point should show a green transient confirmation plus the persistent Rally marker.
4. Patrol and a successful point-target spell should show green confirmation.
5. A successful Smart right-click on an own/shared-control unit should blink its ring green twice.
6. A successful Smart right-click on a passive ally without shared control should blink yellow twice.
7. A successful Smart right-click on a hostile unit should blink red twice.
8. Rejected Smart target orders should show no unit indicator.
9. Unit-target feedback should follow the target and must not spawn `Confirmation.mdx` at a fixed point.
10. Item/destructable Smart clicks retain their existing behavior until their retail indicator rules are verified.

This implementation change was prepared without compiling or running tests, per the caller constraint.
