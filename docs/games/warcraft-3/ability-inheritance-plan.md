# Flat C Ability System Plan

Status: flat definitions and flag-driven command dispatch implemented on 2026-09-12.
`spell_info_t`, `ability_t.spell`, runtime `.parent`, and `S_WireAbilityParents()` are removed.
This is a representation/dispatch migration; it preserves existing cast timing and gameplay rules.
Further retail lifecycle parity work requires separate behavioral regressions.

## Decision

Use the Quake 2 item/weapon pattern: one flat ability definition, capability/policy flags, small shared processors,
and explicit callbacks for specific behavior. No constructor chaining or initialization-time callback inheritance is used.

Keep the extracted TFT registry as an offline behavioral/reference source. Its hierarchy explains useful shared
operations, but is not the specification for our C memory layout or runtime dispatch.

## Quake 2 References

Inspected the local `data/Quake-2-master/game/` sources and id Software's public repository:

| Source | Relevant pattern |
|---|---|
| `g_local.h:234`, `gitem_t` | One definition holds flags, metadata, and `pickup`, `use`, `drop`, `weaponthink` callbacks |
| `g_items.c:1408`, `ammo_grenades` | Combines `IT_AMMO | IT_WEAPON` with `Pickup_Ammo`, `Use_Weapon`, `Drop_Ammo`, `Weapon_Grenade` |
| `g_items.c:764`, `Touch_Item` | Runs common eligibility/presentation/lifetime work around the item's explicit pickup callback |
| `p_weapon.c:380`, `Weapon_Generic` | Runs activation/ready/firing/dropping state and invokes the supplied fire callback at configured frames |
| `p_weapon.c:799`, `Weapon_RocketLauncher` | Supplies frame data and `Weapon_RocketLauncher_Fire` to the common processor |
| `m_move.c:128`, `SV_movestep` | `FL_FLY`/`FL_SWIM` alter shared locomotion rules |
| `g_phys.c:932`, `G_RunEntity` | An exclusive `movetype` enum chooses a physics processor; flags do not replace every discriminator |

Online primary sources: [item definitions and pickup](https://github.com/id-Software/Quake-2/blob/master/game/g_items.c),
[flat item definition](https://github.com/id-Software/Quake-2/blob/master/game/g_local.h),
[weapon state machine](https://github.com/id-Software/Quake-2/blob/master/game/p_weapon.c), and
[physics dispatch](https://github.com/id-Software/Quake-2/blob/master/game/g_phys.c).

The precise analogy is flags + enum state + explicit callbacks. Quake 2 does not select each weapon's fire function
from `IT_WEAPON`, and it does not need a `CRocketLauncher` parent chain. Our flag selects the shared casting path;
our callback supplies Holy Light's effect. No new general-purpose component or object framework is needed.

## Holy Light Definition

The implemented flat definition uses the agreed macro and callback naming:

```c
ability_t CAbilityHolyBolt = {
    .flags = AB_SPELL,
    .name = "Holy Light",
    .target_type = SPELL_TARGET_UNIT,
    .validate = CAbilityHolyBolt_Validate,
    .execute = CAbilityHolyBolt_Execute,
};
```

`AB_SPELL` is one bit: the TFT registry has `CAbilitySimpleSpell` but no independent Simple family.
There is no `AB_SIMPLE` bit or redundant `AB_SPELL` requirement. Combine independent policies normally, for example
`AB_SPELL | AB_AUTOCAST` or `AB_SPELL | AB_CHANNEL`.

`g_local.h` defines one flags field: `AB_PASSIVE` (bit 0), `AB_TOGGLE` (1), `AB_CHANNEL` (2),
`AB_AUTOCAST` (3), `AB_SPELL` (4), `AB_NO_SMART` (5), and `AB_SEPARATE_OFF` (16).
All literal definitions and the SPELL/HUMAN_SPELL/CAMPAIGN_SPELL macros store their data directly in `ability_t`.
Holy Bolt has no `.cmd` wrapper or separate descriptor. `InitAbilities` rejects shared-cast entries with no
execute callback or a conflicting custom command.

Do not encode every ancestor as an additional bit. `CAbility`, `Interfaced`, `Power`, `Button`, `Spell`, and
`SimpleSpell` are not six independently configurable features. Universal plumbing needs no bit; a policy needs a bit
only when a consumer actually makes a different decision. Do not encode the original tree as nested mask aliases.

Keep target shape as the existing enum. Keep the mutable cast phase as a separate enum, not combinations of
CASTING/FINISHING/CANCELLED booleans. Keep generic readiness checks always on for shared casts, rather than creating
flags for mana checks, cooldown checks, every ancestor, or every spell's unique targeting predicate.

## Where Retail Responsibilities Go

| Retail layer | Flat C owner |
|---|---|
| Ability: identity, data, membership/lifecycle | `ability_t`, existing rawcode registry and normalized DDX data, explicit enable/disable callbacks |
| Interfaced/Button: command registration and presentation | Existing command/HUD code consumes target metadata, policy flags and authored UI data |
| Power: availability/order checks | Common order-entry helpers returning a result enum |
| Spell: cast readiness, phases, effect timing, termination | Shared `skills/s_spell.c` cast processor |
| SimpleSpell: target selection/revalidation and effect dispatch | Same cast subsystem using target enum, cast context and explicit `validate`/`execute` callbacks |
| HolyBolt: specialized rules and effect | `skills/s_holylight.c` callbacks |

The [binary evidence](ability-inheritance-binary.md) still establishes behaviors we must preserve: common checks before
specialized checks, distinct failure codes, target revalidation, and a separate effect phase. It does not require
those operations to be owned by runtime objects named after retail base classes.

## Dispatch and Ownership

1. Resolve the issued rawcode to the ability definition through the existing alias-aware lookup. Keep the actual
   rawcode in the cast context so custom Holy Bolt variants use their own costs, level data, cooldown and art.
2. For `flags & AB_SPELL`, enter the shared cast command/target path. The leaf no longer needs `.cmd`.
   Abilities outside this path continue through explicit `.cmd`, `.order`, item-use and lifecycle callbacks.
   During migration, flag-driven casting and a bespoke command path are mutually exclusive entry strategies;
   reject ambiguous registrations instead of inventing precedence. A special command can deliberately invoke the
   shared cast helper without selecting the flag-driven command route.
3. For shared casts, perform common caster/resource/target checks, then call the optional extra `validate` callback.
   Return a result enum with a reason; NULL `validate` means no extra restriction. No ancestor lookup is involved.
4. Accept the order and run the shared approach/cast/revalidation/effect/termination sequence. Call `execute` only
   at the established effect phase. Route animation/think completion through the owning cast subsystem.
5. Use additional orthogonal flags only for supported policy differences, such as channeling, autocast or toggle
   behavior. Those policies call existing explicit acquisition/state/cleanup hooks where ability-specific work is
   needed. A flag alone does not implement channel ticks, target acquisition or reversible transformations.

The shared spell processor belongs in WC3 `skills/`. Keep `g_monster.c`, `m_unit.c` and `g_ai.c` as generic dispatch;
Move still owns reusable steering/routing. Attack, construction, harvest, morph and aura behaviors keep their owning
modules and explicit hooks. Shared processors must not grow `if HolyBolt`, `if Polymorph`, or per-spell flag branches.
Concrete relatives such as Fire Bolt and Thunder Bolt can explicitly reference the same callback/helper and distinct
rawcode data. Their relationship does not require a new flag or implicit inheritance.

Use a compact cast input struct for actual rawcode, caster/target, level and ability definition as needed by the
existing API. Mutable target references, phase and timer belong on the existing entity/thinker state owner, never
on the shared definition. Cooldowns, toggle state and acquired targets remain independent across units/abilities.
Any state-layout changes must update save/load fields and callback registration in the same implementation.

## Implemented Integration

`ability_t` owns name, canonical code, target type, validation/effect callbacks and policy flags, alongside
its existing membership, item, order, autocast and persistent-update hooks. Every effect callback takes
`ability_t const *`; no parallel spell descriptor or inherited function table remains.

`S_AbilityHasCommand` and `S_AbilityCommand` serve `g_unit_ui.c`, `g_commands.c`, and `g_items.c`.
`AB_SPELL` selects `spell_cmd`; bespoke commands still use `.cmd`. `S_SpellAbilityForCode`
resolves a rawcode through the existing alias-aware registry and returns the same flat definition only when
it has the shared-cast flag and an effect. AI/script and autocast paths keep their existing targeting entry points.
`InitAbilities` assigns canonical codes from non-alias registry rows. Aliases never overwrite them.

Runtime parent pointers and wiring are removed. `s_ability_classes.c` keeps inert zero-initialized definitions
in the existing registry slots, preserving concrete ability indices used by HUD/network identity. They expose
no command or shared-cast capability. This deliberately avoids compacting the registry or inventing inherited
behavior for unimplemented classes. The offline audit's `--format=parents` prints retail reference relationships,
not C wiring code. Registration coverage includes inert entries and is not gameplay coverage.

Mutable cast/channel/target state still belongs to units and thinkers. This migration does not change the
save format or cast timing. Actual issued rawcodes and canonical definition codes retain their existing semantics;
it does not claim to fix custom-variant effect data parity or implement additional retail cast phases.

The old append-only offset assertion guarded against stale objects, not a public binary ABI. `ability_t` is a
private game-module struct, so the migration rebuilds the game and test modules against the new layout rather
than retaining dead descriptor storage to preserve private offsets.

## Verification

Passed: 49 spell tests in each of ROC and TFT, five callback-renamer tests, and the full `make test` suite.
The full suite requires local UDP socket access for its networking tests.

The spell suite checks canonical registration and aliases, direct callback contracts, flag-selected commands,
channel/autocast policies, player target selection/approach, and script-issued unit/point orders. The Holy Light
command regression exercises self-target rejection without resource spend, then friendly healing with mana and
cooldown consumption through the real `button`/`select` commands. Registry checks require every shared cast to
have an effect and no `.cmd`; inert class and passive definitions remain non-commandable.

```sh
make test-wc3-engine WC3_PATTERN='wc3_spell.*'
python3 tests/test_rename_ability_callbacks.py
make test
```

See also [ability implementation](ability-implementation-plan.md) and [binary behavior evidence](ability-inheritance-binary.md).

## Callback Rename Tool

`tools/rename_ability_callbacks.py` renames the selected ability's `.validate` and `.execute` functions to
`<CAbilityName>_Validate` and `<CAbilityName>_Execute`. It reads those callback fields directly from the flat ability initializer. Choose the owner explicitly, including when several
abilities share a callback: all identifier references in the file are renamed together, without duplicating behavior.

```sh
python3 tools/rename_ability_callbacks.py games/warcraft-3/game/skills/s_holylight.c --ability CAbilityHolyBolt
python3 tools/rename_ability_callbacks.py games/warcraft-3/game/skills/s_holylight.c --ability CAbilityHolyBolt --write
python3 tests/test_rename_ability_callbacks.py
```

Default mode prints a diff; `--write` applies it. Running it again after a rename does nothing. Comments, quoted
strings and line endings are preserved. The deliberately small tool requires flat explicit initializers and local
static callback definitions; it rejects macro-generated definitions, external callbacks and destination-name
collisions before writing. It leaves other callbacks such as `holylight_done` unchanged.
