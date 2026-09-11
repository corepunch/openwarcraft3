# Warcraft III Unit Animation Properties And Transformation Forms

## Required Animation Names

Warcraft unit profile data may provide `animProps` (`uani`, Required Animation Names). OpenRealm copies that authored value into the per-unit `animation_props` set when `SP_SpawnUnit()` initializes a unit. `SetUnitAnimation` retains the logical animation request separately in `animation_request`; animation lookup then combines the request tags with the unit's active Required Animation Names.

`AddUnitAnimationProperties` mutates the active per-unit tag set and immediately reselects the retained logical animation family. This state is inline edict data, so ordinary WC3 save/load persists it with the unit record.

The stock Medivh model/data combination is unusual: the raven-form unit requests `alternateex`, while the shared model exposes `Alternate` sequences. Animation selection therefore falls back from a required `alternateex` tag to `alternate` only when no matching `AlternateEx` sequence exists. A genuine `AlternateEx` model remains distinct.

## Raven Form Orders

WC3 exposes two stock abilities that use the `ravenform` / `unravenform` order pair: Medivh Crow Form (`Amrf`) and Druid of the Talon Storm Crow Form (`Arav`). Each ability's object data owns its own transformation endpoints:

- AbilityData Data A (`DataA1`) is the base unit type;
- AbilityData `UnitID1` is the raven/alternate unit type.

The immediate orders `ravenform` and `unravenform` first choose the stock transform ability whose authored Data A / UnitID endpoints contain the current unit, then transform the existing edict between those two authored types. The edict/JASS handle is retained rather than replacing the unit with a new entity. Rebinding the type reruns the normal unit-data initialization, including model, movement layer, collision, attacks, authored Required Animation Names, and other presentation state, while preserving current health/mana ratios and temporary combat bonuses.

Preplaced alternate-form campaign units are valid transformation endpoints even if they did not pass through OpenRealm's runtime ability-add path before the map script issues `unravenform`. This is required by the Prologue campaign Medivh raven: the map starts with the alternate unit and later orders that same unit to return to the base form.

The type rebind still happens on the script-issued order so the existing edict immediately owns the destination unit data, but presentation then runs the shared model's authored morph sequence as an active `umove_t`. Human-to-raven temporarily removes alternate-form animation properties to select the untagged `Morph` clip, restores the raven `alternateex` requirement while that clip is active, and enters the raven stand family when it completes. Raven-to-human plays `Morph Alternate` and clears the alternate-form properties at completion before entering the ordinary stand family. The morph move explicitly starts at the selected sequence's first frame, including for cinematic units that may otherwise be paused.

`M_MoveFrame()` restarts the animation that is active *after* an end callback returns. This matters for morph completion: an end callback can replace `Morph Alternate` with `Stand`, and resetting the frame to the completed morph's start would make the newly rebound human model sample an unrelated frame for one tick, producing a visible blank between forms.

Full `Amrf`/`Arav` cast time, takeoff/landing interpolation, transformation effects/sounds, duration/buff-driven automatic reversion, and command-card ability behavior remain separate compatibility work.

## Verification

Focused unit tests install both synthetic `Amrf` and unrelated `Arav` object data and verify that the Medivh-style endpoint resolves through `Amrf`, then verify that:

- `ravenform` changes the same unit from Data A to UnitID and adopts the alternate type's `animProps`;
- `unravenform` restores the Data A unit type and clears the alternate type's authored properties;
- a preplaced alternate-form endpoint can execute `unravenform` without first being transformed by OpenRealm.
