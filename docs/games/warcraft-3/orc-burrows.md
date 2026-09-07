# Warcraft III Orc Burrows

## Runtime contract

OpenRealm models an Orc Burrow as a normal building attack plus a data-driven
cargo hold (`Abun`). The loaded Peons remain real units; cargo state controls
whether the Burrow may attack and which HUD controls are exposed.

### Empty vs occupied state

| State | Attack | Stop | Stand Down | Info panel |
|---|---|---|---|---|
| `cargo.count == 0` | hidden/disabled | hidden | hidden | normal Burrow stats + portrait |
| `cargo.count > 0` | visible/enabled | visible | visible | Burrow portrait + cargo slots |

Every gameplay attack entry point must also check `S_CargoAttacksEnabled()` so
hiding Attack is presentation, not authority.

### Cargo and attack timing

`S_CargoCapacity()` resolves the holder's `Abun`/`Acar`/`Aenc` alias and reads
its authored capacity. Standard Burrow data yields four slots, but UI slot count
must follow the ability data rather than hard-coding four.

For `N > 0` loaded Peons, the current Warsmash-parity rule is:

```text
runtime cooldown = authored cooldown / 2^N
```

If that scaled cooldown is less than or equal to the weapon damage point there
is no post-shot recovery. The attack state must begin the next swing/shot
immediately; leaving `wait == 0` in a `unit_runwait()` phase stalls after one
attack. Stop remains the normal way to terminate a persistent attack order.

### Cargo UI

When cargo is non-empty, the ordinary stat subsection is replaced by capacity-
driven cargo slots while the Burrow portrait remains visible. Each occupied
slot references the actual loaded Peon; clicking it unloads that exact unit.
Empty capacity slots keep their backdrop but have no unit icon/action.

Cargo transitions invalidate the info panel, portrait, and command card so the
first load and final unload immediately switch presentation state.

### Stand Down

`Astd` unloads all occupants through the normal safe-placement/unpause path.
Stand Down is a state command and must be visible whenever an Orc Burrow has
cargo, even when a particular Warcraft data path does not list `Astd` in the
Burrow's `UnitAbilities.abilList`. The command-card builder therefore
synthesizes the stock `Astd` button for occupied Burrows and deduplicates it if
it is also authored normally.

Individual cargo-slot unload is distinct from Stand Down. Stand Down is the
place to restore worker work policy; individual slot unload only ejects the
selected occupant.

### Battle Stations and boarding

OpenRealm implements `Abtl` as a data-driven nearby-worker call: eligible Peons
inside its area are selected up to free capacity and receive the normal board-
transport movement. Smart right-click on a compatible Burrow uses the same
boarding movement. Actual loading happens only after the Peon reaches cargo
range; it is then hidden and paused.

The bundled Warsmash checkout directly confirms generic Load/Smart boarding,
Burrow cargo combat, Stand Down, and cargo-slot UI. Its checkout does not expose
an `Abtl` implementation, so OpenRealm's nearby-worker auto-call is Warcraft
behavior layered on those confirmed cargo mechanics rather than copied from a
Warsmash `Abtl` class.

## Relevant files

- `games/warcraft-3/game/skills/s_cargo.c`
- `games/warcraft-3/game/skills/s_attack.c`
- `games/warcraft-3/game/g_unit_ui.c`
- `games/warcraft-3/game/hud/hud_infopanel.c`
- `games/warcraft-3/game/skills/s_stop.c`
- `games/warcraft-3/game/m_unit.c`
