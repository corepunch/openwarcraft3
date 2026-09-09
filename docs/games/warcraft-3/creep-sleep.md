# Neutral Creep Sleep

OpenRealm models Warcraft III's **natural neutral-creep sleep** as simulation
state owned by the unit.  It is deliberately separate from the Dreadlord Sleep
spell (`AUsl` / `BUsL`), which continues to use the timed-status system.

## Authoritative data

`UnitData.canSleep` (`usle`) is already parsed from Warcraft unit data and is the
authored default for whether a unit may use natural night sleep.  Runtime spawn
initialization copies that value into `edict_s.sleep.can_sleep`; `UnitAddSleep`
may then change the runtime value for neutral units without mutating the shared
SLK row.

Asset inspection of the retail data supplied for this work confirms that the
Ogre family (`noga`, `nogl`, `nogm`, `nogn`, `nomg`) has `canSleep = 1`, and the
corresponding Ogre MDX models contain a looping `Sleep` sequence.  The runtime
therefore requests the model's authored `Sleep` animation instead of treating
natural sleep as stun or freezing a `Stand` frame.

## Runtime contract

Natural automatic sleep currently applies only to `PLAYER_NEUTRAL_AGGRESSIVE`.
An eligible creep enters sleep when it is idle at night and all of these are
true:

- the unit is alive;
- its mutable `can_sleep` flag is true;
- it is not currently affecting combat;
- `PLAYER_STATE_NO_CREEP_SLEEP` (playerstate 25) is zero for Neutral Hostile.

The existing authoritative time-of-day clock owns the night decision through
`G_IsNight()`.  There is no second creep-only clock.

While naturally sleeping:

- the unit's active move requests the `Sleep` model sequence;
- ordinary automatic enemy acquisition ignores it, so player units do not
  auto-attack a camp merely because it is in acquisition range;
- a direct attack order remains valid;
- positive damage wakes it before normal combat/counterattack handling;
- dawn wakes it;
- `UnitWakeUp` wakes it;
- `UnitAddSleep(unit, false)` disables future natural sleep and wakes it now.

Any other runtime behavior that replaces the special creep-sleep move also
clears the natural sleeping flag.  This prevents an ordered/moving unit from
remaining logically asleep after leaving the sleep animation.

## JASS natives

The following natives now consume the natural-sleep state:

- `UnitAddSleep(unit, boolean)` changes neutral-unit night-sleep eligibility;
- `UnitCanSleep(unit)` reports that mutable neutral-unit eligibility;
- `UnitIsSleeping(unit)` reports **natural creep sleep only**;
- `UnitWakeUp(unit)` wakes **natural creep sleep only**;
- `UnitCanSleepPerm(unit)` reports whether the unit currently owns the authored
  `Sleep Always` ability (`Asla`).

`UnitIsSleeping` and `UnitWakeUp` intentionally do not inspect or remove the
Dreadlord Sleep buff (`BUsL`).  Spell sleep still wakes from damage through its
existing timed-status cleanup.

`UnitAddSleepPerm` remains conservative.  Warcraft's editor metadata ties this
native to `Sleep Always` (`Asla`), whose `Sleep Once` and `Allow On Any Player
Slot` fields need an ability-owned runtime model.  Aliasing it to ordinary night
sleep would make those semantics wrong, so the native stays a placeholder until
`Asla` is implemented.

## Player-wide creep sleep switch

Warcraft's `EnableCreepSleepBJ` writes
`PLAYER_STATE_NO_CREEP_SLEEP` on `PLAYER_NEUTRAL_AGGRESSIVE`.  OpenRealm already
allows that playerstate index through the generic 32-slot player-state storage;
natural sleep now consumes it when deciding whether an idle hostile creep may
enter sleep.

The Blizzard.j disable path also explicitly wakes Neutral Hostile units after
setting the state.  That wake-all behavior is supplied by the script's
`WakePlayerUnits` loop and `UnitWakeUp`; changing the playerstate alone only
prevents new natural sleep.

## Save/load

The mutable natural-sleep fields live on `edict_s`, so they cross the ordinary
raw-edict save boundary together with the unit.  Save format version 15 marks the
`edict_t` layout change.  `currentmove` already uses the existing `F_MMOVE`
relocation contract, so a unit saved while using the static creep-sleep move can
restore that move without serializing an address.

## Deliberately unresolved

The patch does **not** guess the following retail details:

- camp-level wake propagation when one member is provoked;
- the exact guard/camp association and radius used for propagation;
- wake behavior caused by nearby building construction;
- wake behavior caused by using a guarded neutral building;
- `ACsp`-owned visual/audio details such as any sleep effect or sound;
- `Sleep Always` (`Asla`) state transitions and `UnitAddSleepPerm`.

Those should be implemented only after the corresponding retail data or
behavioral contract is traced.  In particular, a guessed proximity radius would
make campaign/melee camp behavior less reliable than leaving propagation as a
known gap.

## Verification

Focused in-engine coverage is under `wc3_unit` and the save/load round trip.
After building the test binary, useful filters are:

```bash
build/bin/openwarcraft3-tests -data tests +dedicated 1 +test 'wc3_unit.*sleep*'
build/bin/openwarcraft3-tests -data tests +dedicated 1 +test 'wc3_save.*'
```

For a retail-data manual check, use a map with one of the Ogre rows above,
advance the authoritative clock across dusk/dawn, and verify that the model
enters/leaves its `Sleep` sequence and that direct damage wakes it.

## See also

- [Time of Day](time-of-day.md)
- [JASS Native Coverage](jass-native-coverage.md)
- [Attack and Damage](attack-damage.md)
- [Save / Load](save-load.md)
