# Ability Reverse Engineering

This is the repeatable workflow for extracting a retail Warcraft III
`CAbility*` implementation from `Game.dll` and reimplementing that behavior in
OpenWarcraft3. Recovering the C/C++ control flow is the primary task. RTTI names,
`AbilityData.slk`, tooltips, and third-party engines corroborate the recovered
code; they do not replace it. If the relevant virtual overrides cannot be
identified and disassembled, the ability remains blocked rather than being
implemented from inference.

The repository uses `r2` for binary inspection. The optional Ghidra-backed
plugin is named **r2ghidra**; `r2ghydra` is a common misspelling.

## Prerequisites

Install the tools on macOS with Homebrew and radare2's package manager:

```sh
brew install radare2
r2pm -ci r2ghidra
r2 -q -c 'pdg?' /path/to/Game.dll
```

The last command should print the `pdg` help. `r2ghidra` is a native radare2
plugin and does not require the full Ghidra GUI. `r2ghidra-sleigh` is only
needed when the target processor is not already supported by the plugin.

Check the repository wrapper with the demo binary:

```sh
tools/r2_ability.sh -c CAbilityBash -f AHbh data/Warcraft3demo/Game.dll
tools/r2_ability.sh -a 0x00458040 -o /tmp/ability-r2.txt data/Warcraft3demo/Game.dll
```

The wrapper uses `pdg` when available. Without it, class and rawcode searches
still work and the command exits with a visible setup message; no decompiler
output should be treated as authoritative until `pdg` is present.

## Preserve the Evidence

Use a known binary and record its digest before analysis:

```sh
shasum -a 256 data/Warcraft3demo/Game.dll
rabin2 -I data/Warcraft3demo/Game.dll
```

The demo DLL is useful for structure and calling-convention evidence, but it
may differ from ROC/TFT retail builds. Record the binary, build, address, and
the exact r2 commands in the implementation change or its test fixture. Do
not copy an address from one DLL build into another.

## Recover the C++ Class

Start with RTTI and the rawcode. FourCCs in the executable are packed DWORDs,
so search both the readable bytes and little-endian forms when needed:

```sh
r2 -q -e bin.cache=true -A \
  -c 'izz~CAbilityBash' -c 'izz~CAbilityWarStomp' \
  -c 'q' data/Warcraft3demo/Game.dll

# AHbh is stored in memory as the bytes A H b h on the usual little-endian PE.
tools/r2_ability.sh -f AHbh data/Warcraft3demo/Game.dll
```

For a class name, inspect nearby references and recover its vtable:

```text
r2 -e bin.cache=true -A data/Warcraft3demo/Game.dll
[0x000000]> izz~CAbilityBash
[0x000000]> axt <rtti-string-address>
[0x000000]> px 64 @ <vtable-address>
[0x000000]> pdf @ <constructor-or-method-address>
[0x000000]> pdg @ <constructor-or-method-address>
```

Warcraft III's MSVC RTTI makes the class recoverable even when symbols are
stripped:

1. Find the `.?AVCAbility...@@` type descriptor string with `izz`.
2. Follow its xrefs to the Complete Object Locator and then the vtable.
3. Find constructor xrefs that store the vtable address into `[this]`.
4. Dump the derived and base vtables side by side. Matching slots are inherited;
   differing slots are the ability-specific virtual overrides to extract.
5. Disassemble every differing slot with `pdf`, follow thunks and direct calls,
   and inspect callers with `axt`. Do not stop at the constructor or RTTI name.
6. Recover parameters from calling convention and call sites, then map object
   offsets by comparing reads/writes across constructors and sibling classes.

Useful commands while following that chain:

```text
aaa                         # analyze functions, references, and calls
izz~CAbility                # locate MSVC RTTI names
axt @ <address>             # callers/data references to RTTI, vtable, or method
pxw 32 @ <vtable>           # inspect 32-bit virtual function slots
af @ <method>               # define a missed function before disassembly
pdf @ <method>              # authoritative function disassembly
pdr @ <method>              # recursive disassembly when blocks are fragmented
pdg @ <method>              # optional pseudocode for orientation only
```

Rename recovered methods locally as their role becomes clear (`setData`, target
validation, cast start, channel tick, impact, attack hook, death hook). Compare
the same virtual slot across sibling classes such as `CAbilityStomp` and
`CAbilityWhirlwind`: shared callees reveal the engine contract, while differing
branches reveal the specialization that must be reproduced.

`pdf` is the source of truth. `pdg` is only a hypothesis generator: verify its
types, signedness, field offsets, loop bounds, vtable slots, and indirect calls
against the instructions and call sites. Save the relevant `pdf` output with
`tools/r2_ability.sh -o` so another developer can reproduce the conclusion.

## Trace Runtime Behavior

Start from the derived virtual overrides and follow the complete behavior path:

- Factory/constructor code identifies the class but usually does not implement
  the spell. Continue into overridden target checks, cast methods, channel
  updates, projectile callbacks, attack hooks, and death hooks.
- Record each virtual slot, address, inputs, side effects, called helpers, and
  termination conditions. A rawcode comparison alone is dispatch evidence, not
  behavior evidence.
- Search callers of shared helpers to identify semantics such as unit filters,
  area enumeration, damage, healing, buffs, summons, and cooldown transitions.
- Compare ROC and TFT binaries when an override or vtable layout differs. Never
  transplant an address between builds.
- Use `AbilityData.slk` only after the code shows which object field or data slot
  is consumed. Then use the normalized row to determine the authored value and
  its per-level units.

## Build the Behavior Record

For each ability, fill this record before writing C:

```text
rawcode:       AHbh
class:         CAbilityBash
binary:        data/Warcraft3demo/Game.dll
sha256:        ...
class/vtable:  ...
constructor:   ...
virtual slots: slot, address, inherited/overridden
cast/attack:   recovered control flow and called helpers
target filter: ...
data columns:  AbilityData.slk fields and level indices
state:         passive, toggle, autocast, channel, or instant
effects:       damage, stun, mana, movement, aura, summon, or projectile
timing:        duration, cooldown, tick, wave, or proc chance
uncertainty:   unresolved fields and the check that would resolve each one
```

An implementation record without concrete override addresses and disassembly is
incomplete. Keep unresolved abilities as TODOs; do not promote an inferred
handler merely because its tooltip or data row resembles an existing spell.

Resolve data through the authoritative `AbilityData.slk` row and the existing
helpers (`S_SpellData`, `S_SpellNumber`, `S_SpellDuration`) rather than adding
hardcoded level tables. Resolve `uberAlias` before comparing canonical ability
classes; this is the same rule used by `tools/ability_map`.

Dump one normalized ROC/TFT row before assigning field semantics:

```sh
make build/bin/ability_audit
build/bin/ability_audit -data 'data/Warcraft III' -tft -raw AOcr
```

The output includes `Area`, `Rng`, `Dur`, `HeroDur`, `UnitID`, `BuffID`, and
`DataA-I` for every authored level. This avoids interpreting raw SYLK cell
coordinates by hand. Use `-roc` and `-tft` separately when archive overrides
may replace the row.

## AbilityData to Runtime Mapping

`Units\\AbilityData.slk` is loaded from the active MPQ through the DDX/SLK
schema in `games/warcraft-3/game/g_metadata.c`. The schema fills one normalized
`AbilityData_t` row (`g_unitrow.h`) per rawcode. The important mappings are:

| SLK data | `AbilityData_t` | Runtime accessor |
| --- | --- | --- |
| `Cast`, `Dur`, `HeroDur`, `Cool`, `Cost`, `Area`, `Rng` | `AbilityData_t.level[4]` records | `S_SpellNumber` / `S_SpellDuration` |
| `DataA` through `DataI` | `level[].data[9].number` | `S_SpellData(code, level, index)` |
| Data fields containing rawcodes | `level[].data[9].id` | `S_SpellDataId` |
| `BuffID`, `EffectID`, and unit IDs | fields inside each `abilityLevel_t` | `G_AbilityLevel` or the corresponding helper |

Levels are 1-based at the public helper boundary and clamped to four
`abilityLevel_t` records; data indices are 1-based and clamped to nine slots.
This matters when reading
retail documentation that calls `DataA` the first field: use index `1`, not `0`.
The row is authoritative for ROC/TFT values; do not replace it with a C table.

The five implemented rows use the following fields:

| Rawcode | Runtime behavior | Authored fields |
| --- | --- | --- |
| `AOws` | no-target enemy-ground AoE | `Area`, `DataA` damage, `Dur` stun |
| `AEmb` | unit-target mana drain | `DataA` maximum mana drained |
| `AHbh` | physical attack proc | `DataA` chance, `DataC` bonus damage, `HeroDur`/`Dur` stun |
| `AOae` | passive allied aura | `Area`, `DataA` movement bonus, `DataB` attack-speed bonus |
| `AOwk` | timed hidden movement state | `Dur`/`HeroDur`, `DataA` movement bonus, `DataC` first-hit bonus |

Verified examples from the hero block before `AEer`:

| Rawcode | Fields consumed |
| --- | --- |
| `AHab` | `Area`, `DataA` flat mana regeneration |
| `AOcr` | `DataA` percent proc chance, `DataB` damage multiplier |
| `AUts` | `DataA` reflected fraction, `DataB` minimum reflection, `DataC` armor |
| `AUau` | `Area`, `DataA` fractional movement bonus, `DataB` flat life regeneration |
| `AEev` | `DataA` fractional evasion chance |
| `AUav` | `Area`, `DataA` fractional lifesteal |
| `AOcl` | `DataA` initial damage, `DataB` target count, `DataC` reduction per jump |

Do not infer units from display text. In stock rows, Critical Strike stores
chance as whole percent (`15`), while Evasion stores it as a fraction (`0.10`).
The normalized dump is the discriminating check.

## Implementation Gotchas

- `spell_cmd` is appropriate for War Stomp and Mana Burn because it owns mana,
  cooldown, target selection, and relationship validation. War Stomp is a
  `SPELL_TARGET_NONE` callback; it must not manufacture a point target.
- Bash is passive even though its generator entry has a `CAbilityBash` class.
  Its proc belongs at the attack damage point. Applying it in a command handler
  would expose a button and would miss ordinary attacks.
- Endurance Aura is also passive. The current implementation evaluates nearby
  allied aura owners in the central movement-speed and attack-timing consumers.
  It intentionally does not create one status slot per recipient, avoiding
  stale aura state when units move out of range.
- Wind Walk uses the existing `RF_HIDDEN` presentation flag and a timed `BOwk`
  status. Expiry currently clears the timed status through the normal status
  lifecycle; the first physical hit consumes the status, adds `DataC` damage,
  and reveals the attacker.
- `unit_addtimedstatus` takes a four-character string such as `"Bstu"` or
  `"BOwk"`, not a numeric FourCC. Numeric FourCCs are used for ability/status
  comparisons after the string has been packed.
- The demo `Game.dll` is useful evidence for RTTI and method ownership, but its
  addresses and decompiler types are build-specific. `AbilityData.slk` remains
  the source of truth for values and level semantics.
- `r2ghidra` may report missing format databases and fail to recover pseudocode.
  Continue with `pdf`/`pdr`, manually recover the virtual methods, and use call
  sites to establish parameters. If usable disassembly still cannot be
  recovered, record the blocker and stop. Never substitute another engine's
  implementation for retail code or present inferred pseudocode as disassembly.

## Translate to the Runtime

Choose the smallest existing contract that matches the evidence:

| Retail behavior | OpenWarcraft3 contract |
| --- | --- |
| Unit/point spell | `spell_info_t` + `spell_cmd` |
| Self toggle | `SPELL_TOGGLE` and a unit status |
| Timed buff/debuff | `unit_addtimedstatus` |
| Permanent/passive state | `unit_addstatus` or an existing passive consumer |
| AoE burst | `FILTER_EDICTS` with `S_SpellIsAliveTarget` and relationship checks |
| Channel/waves | `SPELL_CHANNEL` plus a thinker entity |
| Autocast | the `autocast_*` hooks on `ability_t` |
| Attack proc | the existing combat attack hook; do not model it as a cast |

Do not implement a passive attack ability as a normal command merely because
the generated map labels it with a `CAbility*` class. Bash, critical strike,
auras, and orb/attack modifiers need a combat or status consumer. If that
consumer is missing, document the missing boundary and implement a lower-risk
verified ability first instead of silently making the spell button do the
wrong thing.

## Implement and Test

1. Save the recovered virtual methods, addresses, and behavior record.
2. Add `a_<name>` declaration in `s_skills.h` and its smallest source module.
3. Use a named FourCC constant and the existing data/status helpers.
4. Add the rawcode to `abilitylist[]`; remove only the matching generated TODO.
5. Test positive and negative paths: registration, level data, target filter,
   effect, duration/state, and cancellation or inverse behavior.
6. Run a focused test, then the full suite:

```sh
make test-wc3-engine WC3_PATTERN='spell|ability'
make test
```

For runtime validation, load a map with the relevant unit, issue the ability,
and use a bounded run. A menu startup does not validate gameplay dispatch:

```sh
build/bin/openwarcraft3 -data 'data/Warcraft III' -roc \
  +map 'Maps/(2)Rivercross.w3m' +com_frame_limit 100
```

Repeat archive-sensitive checks with `-tft` when the ability data or FDF path
can differ between ROC and TFT.

## Regenerate Coverage

After adding an implementation, regenerate the ability audit so aliases and
class names remain consistent:

```sh
build/bin/ability_map -dll data/Warcraft3demo/Game.dll \
  -data 'data/Warcraft III' > /tmp/ability-map.c
```

Use the repository's normal generator/build target if it has changed since the
last audit. Never hand-edit the generated block except to remove an entry
whose implementation is now present.

## Evidence and Review Checklist

- [ ] Binary SHA-256 and build recorded.
- [ ] RTTI class and vtable/reference chain recorded.
- [ ] `pdf` checked against every `pdg` conclusion.
- [ ] Rawcode, alias, and `AbilityData.slk` row confirmed.
- [ ] Target relationship and alive/dead filters match the retail behavior.
- [ ] Timing, chance, and level data use runtime helpers.
- [ ] Passive/attack behavior is not incorrectly exposed as a cast.
- [ ] Registry and generated TODO coverage are updated.
- [ ] Focused tests cover the new path and its inverse.
- [ ] ROC and TFT runtime checks completed where applicable.
- [ ] No investigative per-frame logs remain in production code.
