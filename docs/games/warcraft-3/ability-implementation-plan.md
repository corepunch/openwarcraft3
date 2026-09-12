# Warcraft III Ability Implementation Plan

This project implements Warcraft III abilities from the authoritative game data and
observable gameplay contract. The goal is useful, testable compatibility. TFT's extracted
class hierarchy supplies behavioral evidence; the [flat C ability plan](ability-inheritance-plan.md)
uses Quake 2-style flags, enums, shared processors and message procedures without runtime object inheritance.

`AbilityStrings.txt` is the starting point because it states what the player is
supposed to observe. It is not a complete specification, so every implementation
record separates confirmed facts, reasonable deductions, and unresolved behavior.

Ability-owned behavior is the default architecture: keep the full behavior in its ability module and reach it through
generic dispatch. See [ownership](#ability-owned-orders-and-persistent-behavior) and [testing](#testing) before adding code.

## Sources Of Truth

Use sources in this order, choosing the smallest set that answers the question:

1. Active ROC/TFT archive data.
2. Existing OpenWarcraft3 runtime contracts and neighboring implementations.
3. Controlled observation in the retail game.
4. Independent references such as Liquipedia, Warcraft III wikis, and Warsmash.
5. `Game.dll` disassembly for details that remain ambiguous or cannot be observed.

Retail data is authoritative for authored values. External references are useful for
explaining behavior, but they must not override an active ROC/TFT row without a reason.
Game.dll is evidence about one binary build, not a portable source file.

## Data Flow

The normal ability path is:

```text
MPQ AbilityStrings.txt
  -> name, tooltip, hotkey, visible behavior
MPQ AbilityData.slk
  -> area, range, duration, cost, cooldown, targets, DataA-I, rawcode IDs
DDX/SLK loaders
  -> AbilityData_t and abilityLevel_t
ability registry
  -> ability_t
runtime consumers
  -> damage, status, movement, combat, summons, morphs, or presentation
```

Resolve values through the existing accessors rather than copying level tables into C:

- `S_SpellData(code, level, index)` for `DataA` through `DataI`.
- `S_SpellDataId(code, level, index)` for rawcode-valued data fields.
- `S_SpellNumber(code, field, level)` for `Area`, `Rng`, and related numbers.
- `S_SpellDuration(code, level, hero)` for `Dur` and `HeroDur`.
- `G_AbilityLevel(FS_SLKKey(classname), level)` when the complete normalized row is needed.

Levels and data indices are 1-based at the public helper boundary. Use `ability_audit`
to inspect ROC and TFT rows separately:

```sh
make build/bin/ability_audit
build/bin/ability_audit -data 'data/Warcraft III' -roc -raw AOws
build/bin/ability_audit -data 'data/Warcraft III' -tft -raw AOws
```

## Reading AbilityStrings

Ability strings provide a compact behavioral sketch:

| Field | Use | Limitation |
| --- | --- | --- |
| `Name`, `Tip`, `Hotkey` | command-card presentation | does not prove order registration |
| `Ubertip` | visible effect, target class, level fields | may omit edge cases and implementation details |
| `Untip`, `Unubertip` | toggle/autocast inverse | does not define status cleanup |
| `Researchtip`, `Researchubertip` | hero learning presentation | may describe a related campaign variant |
| `Bufftip`, `Buffubertip` | player-visible status | does not define stacking or dispel rules |
| `<rawcode,Field>` | authored value dependency | field units still come from the normalized SLK row |
| `Globalmessage`, `Globalsound` | visible cast/death feedback | empty values are meaningful and should remain empty |

Do not treat a tooltip as a complete target predicate. For example, “enemy land
units” establishes the ordinary War Stomp filter, but immunity, summoned units,
collision radius, stun replacement, and timing still require a runtime contract or
observation.

Campaign entries can be separate object-data variants. `AOw2` in the campaign table
is not automatically the same implementation record as standard Orc `AOws`; confirm
the rawcode and active `AbilityData.slk` row before sharing a handler.

## Implementation Triage

Start with abilities whose behavior maps directly to an existing runtime contract:

| Category | Typical contract | First implementation strategy |
| --- | --- | --- |
| direct damage/heal | `ability_t` with `AB_SPELL` | apply the authored amount to validated targets |
| timed buff/debuff | `unit_addtimedstatus` | add and expire the status through normal lifecycle code |
| toggle | `AB_TOGGLE` plus status | execute the same command to add/remove the state |
| passive aura | ability modifier hooks consumed by movement/combat | calculate from active nearby owners; avoid stale recipient state |
| attack proc | ability procedure at attack resolution | evaluate chance and effect in the owning ability at damage time |
| autocast | autocast messages | acquire targets, then issue the ordinary spell order |
| channel/wave | `AB_CHANNEL` and thinker | schedule ticks and cancellation explicitly |
| summon | summon helpers and unit rawcode | use authored `UnitID`/`Data` values and duration |
| morph/transform | unit-type rebinding contract | preserve the edict and restore all affected unit state |
| cargo/inventory | cargo or item lifecycle | use the existing handle and ownership rules |

Do not force passive, attack, aura, or autocast abilities into a visible cast handler
just because their object-data class is named `CAbility*`. The runtime contract owns
where the effect is evaluated.

Ability procedures use TFT `CAbility*` class names. Look up the FourCC in
`tft-ability-classes.txt` and name the function after its class: `CAbilityDoom`,
`CAbilityWarStomp`, `CAbilityFrostArmor`. Do not invent names or derive them
from rawcode abbreviations. Procedure macros prepend `C` to a PascalCase argument;
`BZ_SIMPLE_SPELL_PROC(AbilityDoom) { ... }` defines `CAbilityDoom` and its `AbilityDoom_Execute` body,
whose parameters are `caster`, `st`, and `spell`.
The rawcode-to-procedure mapping and all static flags/target/order data live only in
`abilitylist` in `s_skills.c`. `S_AbilityItem(actual_code)` produces an `abilityitem_t` with the requested
rawcode and resolved registry row. Validation and execution receive this item, so two aliases can share a
procedure while reading different authored data. There is no canonical code assignment or `.alias = true` rule.
Display text is fetched through authored ability profiles/strings and map overrides; gameplay values use
`G_AbilityData` / `G_AbilityLevel` by the actual rawcode.

Register concrete `AbilityData.alias` row IDs and `AbilityData.code` implementation IDs, plus necessary internal
commands. Do not register the abstract TFT class tree. See [behavior and identity](ability-inheritance-plan.md#behavior-and-identity)
and [data-backed registry](ability-inheritance-plan.md#data-backed-registry) for lookup and verification details.


The active rawcode portion of `games/warcraft-3/game/skills/s_skills.c` is generated
from the `*AbilityStrings.txt` files in `data/strings`. It is grouped by source file,
uses the text entry's `Name` as its block comment, and preserves the existing handler
mapping. Regenerate and check it with:

```sh
python3 tools/generate_ability_registry.py --write
python3 tools/generate_ability_registry.py
```

The generator uses the first sorted text file when a rawcode is defined more than
once. Entries without a text definition remain in the final `No AbilityStrings source
file` group. The commented TODO entries are also generated, remain disabled, and are
delimited by their own generated markers.

## Adding a New Ability

### 1. Identify the ability

Look up the FourCC in `tft-ability-classes.txt` to find its TFT class name and parent:

```
{ "ANdo", "CAbilityDoom" },  /* parent="AAsm" */
```

Run the audit tool to see where this ability stands:

```sh
python3 tools/wc3_ability_class_audit.py --format=todo | grep ANdo
```

### 2. Identify shared behavior

The retail parent is evidence for behavior to reproduce through flags and explicit procedure delegation.
It does not determine our runtime memory layout. Common retail families:

| Parent | Category | Shared behavior |
| --- | --- | --- |
| `CAbilitySimpleSpell` (AAsm) | Most hero/unit spells | mana cost, cooldown, targeting, `spell_cmd` pipeline |
| `CAbilityAutoTargetSpell` (AAat) | Autocast spells (Heal, Slow) | autocast hooks + spell pipeline |
| `CAbilityRangerArrow` (AHRa) | Toggle attack arrows | autocast + toggle status |
| `CAbilityPassive` (APas) | Passive abilities | no command handler; consumed by combat/movement |
| `CAbilityMorph` (Amor) | Morph abilities | unit-type rebinding |
| `CAbilityAura` (aura) | Aura abilities | persistent nearby-unit effects |
| `CAbilitySpell` (AAsp) | Channel/toggle/complex spells | mana + cooldown but not simple-spell targeting |
| `CAbilityPersistentBonus` (APbo) | Item stat bonuses | passive bonus applied/removed on equip |
| `CPower` (powr) | Fundamental abilities (Move, Attack) | always present on units |

### 3. Define the ability

For a spell that uses the unified pipeline, define its procedure with the smallest applicable procedure macro:

```c
/* Name=Doom
 * Ubertip="Curses a target enemy unit, dealing damage over time."
 */
BZ_SIMPLE_SPELL_PROC(AbilityDoom) { target_status_execute(caster, st, spell); }
```

Put single-use execution logic directly in the body; call a shared helper when multiple abilities share it.
The macro forward-declares `AbilityDoom_Execute` and routes `A_EXECUTE` to it. Flags and target shape belong
in the registry row. For behavior that needs more messages, use `BZ_ABILITY_PROC` (which supplies `ent`, `msg`,
and `call`), write the switch directly, and delegate unhandled messages to the TFT parent procedure:

```c
BZ_ABILITY_PROC(CAbilityDoom) {
    switch (msg) {
    case A_VALIDATE: return doom_validate(ent, call->target, call->item);
    case A_EXECUTE: doom_execute(ent, call->target, call->item); return true;
    default: return CAbilitySimpleSpell(ent, msg, call);
    }
}
```

Command abilities use the same body syntax. `BZ_COMMAND_PROC(AbilityMove) { ... }` forward-declares
`AbilityMove_Command(LPEDICT clent)`, routes `A_COMMAND` to it, and supplies its function header.
`clent` is `call->client` when supplied, otherwise the procedure's `ent`. Other messages return false.
Use the generated `AbilityMove_Command` name for direct calls or menu callbacks.

Item use follows `BZ_ITEM_PROC(AbilityItemHeal) { ... }`. It forward-declares
`BOOL AbilityItemHeal_ItemUse(LPEDICT clent)` and supplies that function header. `A_ITEM_USE` passes the
procedure's `ent` as `clent`; the body returns whether use succeeded so inventory can consume a charge.
Other messages return false without running the body.

### 4. Declare and register

Add an extern declaration to `s_skills.h`:

```c
BZ_ABILITY_PROC(CAbilityDoom);
```

Uncomment or add the entry in the abilitylist in `s_skills.c`:

```c
{ "ANdo", CAbilityDoom, AB_SPELL, SPELL_TARGET_UNIT },  /* Doom */
```

### 5. Select command policy

Use `AB_SPELL` for the shared cast processor and handle `A_EXECUTE` plus optional `A_VALIDATE` in the procedure.
Combine independent policies (`AB_CHANNEL`, `AB_TOGGLE`, `AB_AUTOCAST`) in the registry row. Bespoke orders use
`AB_COMMAND`/`A_COMMAND`; lifecycle and persistent behavior use their corresponding messages.
`S_AbilityHasCommand` and `S_AbilityCommand` provide the common HUD, player and item entry contract.

Concrete relatives can call the same helper or parent procedure: Fire Bolt and Thunder Bolt share effect code
while the per-use `abilityitem_t` carries the actual rawcode. There is no runtime parent wiring.

### 6. Write tests

Write focused tests before launching the game. See [Testing](#testing) for the required
coverage: registration lookup, authored data mapping, positive path, nearest negative
path, duration/expiry, and save/load.

### 7. Verify coverage

```sh
python3 tools/wc3_ability_class_audit.py --format=coverage
python3 tools/wc3_ability_class_audit.py --format=hierarchy | grep -A2 ANdo
```

The audit tool should show the ability as implemented (not TODO or missing).

## Behavior Record

Before writing the handler, create a short record in the relevant source or test:

For an implemented handler, put the matching string-table contract immediately
above the function or ability registration as a comment. Use the exact `Name` and
`Ubertip` text when it is short enough to remain readable; preserve placeholders
such as `<AOws,DataA1>` and `<AOws,Dur1>` so the comment stays tied to authored
data rather than hardcoding a value. For toggle/autocast abilities, also include
`Untip` and `Unubertip` above the inverse command path. These comments are an
implementation index, not a replacement for the source strings or a claim that
every tooltip sentence is fully implemented.

Example:

```c
/* Name=War Stomp
 * Ubertip="Slams the ground, dealing <AOws,DataA1> damage to nearby enemy land units and stunning them for <AOws,Dur1> seconds."
 */
static void war_stomp_execute(...)
```

For an inverse/toggle path:

```c
/* Untip=Stop Defend
 * Unubertip=""
 */
```

Do not infer toggle semantics from `Untip` alone. Retail Avatar is a timed
one-shot spell whose `BHav` buff owns its lifetime.

If a handler is shared by several rawcodes, list each rawcode's string contract
or state explicitly which entries share the implementation. If the entry is not
implemented, annotate the registry entry with `TODO` instead of copying a
tooltip above an unrelated function.

```text
rawcode:       AOws
archive:       ROC/TFT row checked separately
strings:       OrcAbilityStrings.txt entry and relevant tooltip fields
class:         authored class or unresolved
state:         instant, toggle, passive, autocast, channel, or morph
inputs:        Area, DataA, Dur, and any other consumed fields
relationship:  target ownership, alive state, unit type, immunity rules
side effects:  damage, status, movement, summon, item, or presentation changes
timing:        cast point, impact, tick, duration, cooldown, inverse
confidence:    confirmed, observed, inferred, or unresolved
next check:    the smallest test or observation that could disprove it
```

An inferred field is acceptable when the record says it is inferred and a focused
test covers both the expected path and the nearest negative path. Do not silently
turn a tooltip guess into a general engine rule.

## Presentation And Animation

Ability strings do not normally specify model animation. Resolve presentation in
layers:

1. Check the unit model's available sequences.
2. Check `UnitProfile.uani` / `animProps` Required Animation Names.
3. Reuse the logical animation request system (`spell`, `morph`, `stand`, `attack`)
   instead of hardcoding a model-specific sequence.
4. Check authored ability art, effect, sound, and `AnimSounds.slk` data.
5. Observe timing in retail if the effect must line up with an animation event.

A missing animation string in `Game.dll` does not prove that no animation is played.
The choice may be made by shared casting code, an animation enum, unit data, or the
model itself. Disassembly is useful here only when observation and model/data inspection
cannot establish the required timing or state transition.

## Example: War Stomp

The standard Orc War Stomp entry describes an instant self-centered effect:

- damage nearby enemy land units;
- stun those units;
- use `DataA` for damage and `Dur` for stun duration;
- use `Area` for the radius;
- expose the normal War Stomp name and hotkey.

The local `AOws` handler in `games/warcraft-3/game/skills/s_ability_stubs.c` already
implements that gameplay core through `spell_cmd`, `S_SpellData`, `S_SpellNumber`,
`S_SpellDuration`, relationship checks, and the timed stun status.

## Current Campaign Audit

The campaign strings are not interchangeable with standard ability rawcodes. A
name match is not enough: the registry lookup uses the rawcode, and the campaign
rows must be checked separately in `AbilityData.slk`.

Confirmed from `games/warcraft-3/game/skills/s_skills.c` and
`data/strings/CampaignAbilityStrings.txt`:

| Campaign entry | Registry status | Result |
| --- | --- | --- |
| `AOw2` War Stomp | registered as unsupported | `AOws` is implemented, but the Cairne campaign variant needs its own spell descriptor |
| `ANsh` / `AOs2` Shockwave | registered as unsupported | campaign variants need code-specific spell descriptors |
| `ANcf` Breath of Fire | registered as unsupported | `ANbf` is implemented, but the campaign row needs its own spell descriptor |

## Human Ability Audit

The Human registry entries audited from `HumanAbilityStrings.txt` are owned by
`skills/s_human_abilities.c`. ROC and TFT currently provide identical normalized rows
for the selected entries. Active spells use the shared `spell_cmd` path, and Heal,
Inner Fire, Slow, and Spell Steal use the generic one-enabled-ability autocast state in
`edict_t.autocast_code`.

| Runtime owner | Abilities | Authored inputs consumed |
| --- | --- | --- |
| unit-target spell | Aerial Shackles, Control Magic, Cloud, Inner Fire, Heal, Slow, Invisibility | `Rng`, `Dur`/`HeroDur`, `BuffID`, `DataA-C` |
| timed morph | Polymorph (`Aply`) | `Ply1`/`DataA`, `Ply2`-`Ply5`/`DataB-E`, `Rng`, `Dur`, `BuffID`, target mask |
| point spell/thinker | Flare, Dispel Magic | `Area`, `Dur`, `DataB` |
| toggle/status | Defend, Magic Defense | `DataA-F`, `Dur`, `HeroDur` |
| timed transformation | Avatar | `BHav`, `DataA-C`, `Dur` |
| attack resolution | Feedback, Flak Cannons, Fragmentation Shards, Barrage, Storm Hammers | `DataA-E`, `Area`, unit attack splash fields |
| capability/presentation marker | Sphere, Phoenix Morphing, Flying Machine Bombs, True Sight, Magic Sentry | registry presence; no invented cast behavior |

Central consumers apply Slow/Defend movement factors, Inner Fire armor, attack
locks, Defend piercing reduction/reflection,
Feedback mana burn, and attack splash. Invisibility clears on attacks, spell commits,
and status expiry. `human_ability_think` owns Flare reveal updates and Aerial Shackles
damage ticks and is appended to the save callback roster.

Polymorph keeps the original edict/class/stat identity and stores only reversible model, scale, and movement-speed presentation state. The configured `Ply2`-`Ply5` form supplies the temporary model/movement speed, `Ply1` gates Neutral Hostile creep level, and the configured timed buff owns expiry/dispel restoration. See [Polymorph](polymorph.md).

Avatar stores its applied armor, maximum-health, and attack-damage deltas on the
edict. `BHav` owns the lifetime; expiration, death, and ability removal subtract the
stored values, clamp current health, remove the `alternate` animation property, and
invalidate the info panel. Hero stat recomputation includes
`temporary_health_bonus`, so attribute changes cannot erase an active Avatar bonus.
Spell effects call `S_SpellDamage`, which rechecks `BHav` at impact time, while
physical attack damage continues through `T_Damage`.

Detection is a per-player visibility concern; True Sight and Magic Sentry must be
implemented in the snapshot/FOW visibility contract rather than by globally clearing
`RF_HIDDEN`.

Useful checks:

```sh
build/bin/ability_audit -data 'data/Warcraft III' -roc -raw Adef
build/bin/ability_audit -data 'data/Warcraft III' -tft -raw Adef
build/bin/openwarcraft3-tests +dedicated 1 +test 'wc3_spell.*' +com_frame_limit 100
build/bin/openwarcraft3-tests +dedicated 1 +test 'wc3_save.*' +com_frame_limit 100
```
| `Acdh` Drunken Haze | registered as unsupported | `ANdh` is implemented, but the campaign row needs its own spell descriptor |
| `ANhw` Healing Wave | registered as unsupported | `AOhw` is implemented, but the campaign row needs its own spell descriptor |
| `ANhx` Hex | registered as unsupported | `AOhx` is implemented, but the campaign row needs its own spell descriptor |
| `ACs7`, `ACs8`, `Arsq`, `Arsg`, `Arsp` | registered as unsupported | campaign summon behavior is not yet implemented |
| `ANbr`, `ANsb`, `Acef`, `Arsw`, `AOr2`, `AOr3`, `AOls` | registered as unsupported | no matching implementation exists yet |

This is a rawcode coverage result, not proof that the standard handlers are
behaviorally complete. For example, the campaign tooltip for Breath of Fire
requires Drunken Haze ignition, while the current `ANbf` handler applies only
the initial area damage. Each such gap needs a focused behavior test before the
campaign rawcode is aliased to a standard handler.

The remaining questions should be handled independently:

- Does damage occur at command acceptance, cast point, or impact?
- Which units are excluded besides air units, allies, dead units, and the caster?
- Which logical caster animation is requested?
- Which effect and sound are authored, and when are they emitted?
- Does stun replace, refresh, or combine with an existing stun?

Use a focused test or retail observation for each question. Do not block the gameplay
implementation on unrelated questions.

## Disassembly: Optional Evidence

For the demo's actual FOURCC-to-class registrations, use the
[Python extractor and offset reference](demo-ability-classes.md). Its generated
mapping includes internal type IDs; it does not guess object-data aliases.

Use `Game.dll` when it answers a specific unresolved question, not as the default
implementation workflow. Good questions include:

- whether a cast applies an effect at cast start or at an animation callback;
- which shared helper enforces an unusual target predicate;
- how a morph preserves handles, inventory, buffs, or health;
- whether a campaign variant has a distinct lifecycle;
- which logical animation or sound event is selected when data is inconclusive.

Keep the binary digest and exact command when recording a result:

```sh
shasum -a 256 data/Warcraft3demo/Game.dll
rabin2 -I data/Warcraft3demo/Game.dll
r2 -q -e bin.cache=true -A \
  -c 'izz~CAbilityWarStomp' -c 'izz~AOws' -c 'q' \
  data/Warcraft3demo/Game.dll
```

Prefer call-site and behavior evidence over class names. If using radare2, verify
`pdg` against `pdf`; decompiler output is orientation only. Do not transplant an
address from the demo DLL into another build. Preserve unresolved binary findings as
notes attached to the specific ability rather than making them a prerequisite for
all ability work.

## Testing

Recreate the gameplay situation in automated tests before launching the game. Use production order/ability dispatch,
the real animation and update callbacks, deterministic simulation time, and representative archive fixtures. If the
intended reproducer is a sequence of player or script actions, test that sequence rather than only the final helper.
Extend the harness or fixture generator if it cannot yet exercise the path. See the
[test-first workflow](../../../CONTRIBUTING.md#test-first-behavior-verification).

Each new ability needs focused tests for:

- registration and rawcode lookup;
- the authored level and field mapping;
- the positive target/effect path;
- the nearest invalid target or inverse state;
- duration, cancellation, expiry, and repeated casts where applicable;
- save/load when the ability adds persistent entity state;
- ROC and TFT rows when the archives differ.

For Raven Form, reproduce both completion of Morph and replacement by Move before its end callback, followed by ascent,
pause, and reversal. Assert the retained order, form, animation interval, height, and cleanup at the relevant steps.
Exercise the generic scheduler as well as the ability callback so a missing registration or lifecycle hook cannot hide
behind a passing helper test. Use the generated morph model to select actual sequences; do not require a campaign replay
to prove these transitions. A save/load or parser bug belongs in the corresponding fixture-backed regression suite.

Run the affected WC3 tests first, then the full suite:

```sh
make test-wc3-engine WC3_PATTERN='wc3_unit.*'
make test-wc3-engine WC3_PATTERN='wc3_spell.*'
make test
```

Only use a bounded game run for a named property the tests and tools cannot measure, such as the final rendered pixels.
State that gap before launching and exercise it directly. Animation selection, effect lifetime, status cleanup, and
return to the normal state should already have automated assertions. Once covered, rerun the tests instead of the game.

## Known Pitfalls

- Do not read units from tooltip prose; use `ability_audit` and the normalized row.
- Do not assume a campaign rawcode is interchangeable with its standard counterpart.
- Do not expose a passive ability as a command merely because it has a `CAbility*`
  class name.
- Do not hardcode per-level values that already exist in `AbilityData.slk`.
- Do not silently ignore an unresolved effect, sound, target rule, or animation;
  record it and add the smallest check that will resolve it.
- Do not copy disassembly addresses between DLL builds.
- Do not treat Warsmash or another engine as recovered Blizzard source. Use it as an
  independent behavioral reference.

See also:

- [Ability coverage](architecture/ability-coverage.md)
- [Unit animation properties](unit-animation-properties.md)
- [Ability and item effects](ability-and-item-effects.md)
- [Warcraft III data model](../../wc3-data-model.md)

## Ability-owned orders and persistent behavior

This is the direction for new gameplay work and for refactoring behavior encountered in general-purpose files.
Split behavior by the ability that owns it; do not grow `g_monster.c`, `m_unit.c`, or `g_ai.c` with individual spell rules.
Keep an ability's order strings, validation, state transitions, animation moves, completion functions, and timed effects in its
`skills/s_*.c` owner. For immediate orders outside the spell pipeline, register `ability_t.orders` and `.order`;
for effects that outlive an active order, use `.update`. `s_skills.c` owns generic dispatch and deduplicates shared
update handlers at initialization. Do not add a spell-name branch or direct spell update to `m_unit.c`/`g_monster.c`.
See [Raven Form](unit-animation-properties.md) for the order/update contract and persistence tests.

Encapsulation includes setup, interruption, cancellation, inverse orders, expiry/death/removal cleanup, and restoration
after save/load where applicable. Moving a command function into `skills/` while leaving its timer or cleanup in a generic
unit loop is incomplete. Use existing lifecycle hooks or add a small generic hook when necessary. The shared mechanism
dispatches or applies an ability's result; the ability decides its own rules. Keep this flat and data-oriented.

`skills/s_move.c` owns reusable locomotion (steering, steps, collision policy, route goals, and support height).
Attack, Follow, Harvest, and Build call that ability's movement operations while owning their own goals and arrival
conditions. `g_ai.c` owns acquisition/behavior transitions, and `g_monster.c` owns initialization and generic animation dispatch.
Shared math, routing algorithms, collision queries, serialization, and in-place type rebinding remain reusable services;
ability ownership does not mean duplicating these mechanisms in each ability.
