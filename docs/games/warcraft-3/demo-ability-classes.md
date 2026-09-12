# Warcraft III Demo Ability Class Registry

## Extracting the reference

```sh
python3 tools/extract_wc3_ability_classes.py data/warcraft3demo/game.dll -o docs/games/warcraft-3/demo-ability-classes.txt
python3 tools/extract_wc3_ability_classes.py data/warcraft3demo/game.dll --all-classes -o /tmp/demo-all-classes.txt
python3 tests/test_wc3_ability_classes.py
```

The Python tool needs no third-party packages and does not execute the DLL. The
[generated reference](demo-ability-classes.txt) contains all **197 registered
`CAbility*` classes**, sorted by case-sensitive FOURCC, in `s_skills.c`-style rows.
Each row also records the parent type and file offsets of the registration call,
factory, destruction callback, pool cleanup callback, and RTTI name. `--all-classes`
exports all **526 static class registrations**, including buffs and other objects.

This is the September 2002 demo, not TFT or a complete retail ability inventory.
The extractor requires SHA-256
`286823c37a1083e91f07d040e46a9df7af4c4952e01fcbba460589bd4e297654`
(6,205,503 bytes), rejecting other builds before interpreting any offsets.
The local DLL is not redistributed as a fixture. Integration checks require it;
they verify the entire exposed ability RTTI inventory, known associations/offsets,
reproducible output, the all-class mode, and rejection without output corruption.

## Abilities versus buffs

A **buff** is an effect attached to a unit that changes its state or stats, often
for a limited duration. Beneficial effects are commonly called buffs and harmful
effects debuffs; Warcraft III's `CBuff*` classes represent both. A buff is gameplay
state, separate from the icon or visual effect used to present it.

An ability owns the action that applies an effect; a buff represents the applied
effect on the affected unit. Bloodlust illustrates the distinction with two
separate registrations in this demo:

| FOURCC | Class | Role |
|---|---|---|
| `Ablo` | `CAbilityBloodlust` | The ability that casts Bloodlust |
| `Bblo` | `CBuffBloodlust` | The Bloodlust effect applied to the target |

An aura follows the same general pattern: the source unit's ability affects
nearby eligible units, while buffs represent its effects on those units. The
ability and its applied buffs have separate lifetimes; an aura need not have a
fixed duration.

The default extractor output selects names beginning with `CAbility`, so it
includes `Ablo` but excludes `Bblo`. `--all-classes` includes both, along with
other registered game objects. These retail class names describe the demo's
organization; they do not require matching C++ classes in our C implementation.
See [ability, buff, and item presentation effects](ability-and-item-effects.md)
for the separation between gameplay state and its visual presentation.

## Registration mechanism and offsets

There is no flat, on-disk `abilitylist[]`. Startup code builds a runtime class
registry by calling **VA `0x6F073450`, file offset `0x00073450`**. In this build,
the preferred image base is `0x6F000000`. `.text`, `.rdata`, and file-backed `.data`
have equal RVAs and file offsets; this is not true of every PE section or DLL.
The extractor scans `.text` at file offsets `[0x1000, 0x4EB0E8)` and reads names
from file-backed `.data` at `[0x546000, 0x59C000)`.

The 25 bytes immediately before each static registration call encode:

```asm
push pool_cleanup
push destroy
push create
mov  edx, parent_fourcc
mov  ecx, type_fourcc
call 0x6F073450
```

The register function hashes the type key, adds/updates its record, links it to
the parent record, and stores the three stack arguments. The registry owner
pointer is at VA `0x6F5B0344`; its type hash table begins at owner `+0x04`.
The type record uses `+0x18` for the key, `+0x70` for the factory, `+0x74` for
destruction, `+0x78` for pool cleanup, and `+0x7C` for the parent type ID.
These are runtime record offsets, not a serialized table in `.data`.

All 526 pool cleanup callbacks begin `push -2; push rtti_name`. Their factories
also pass that identical RTTI name to the allocator within the first 24 bytes.
The extractor cross-checks both references and reads `.?AV<Class>@@` directly.
The two other calls to the register function (file offsets `0x712BF`, `0x712DF`)
obtain agent-root type IDs through `0x6F07EBB0` (returns `+aga`); the registered
types are `+aga` and `+agr`, not abilities.
Every call to the register function is accounted for: 528 total, 526 static, two
explicit dynamic root calls. Unknown call shapes or incomplete counts are errors.

For example, the Aura registration starts at file `0x221C70`:

```asm
push 0x6F221D70        ; pool cleanup
push 0x6F221D50        ; destruction
push 0x6F221C90        ; factory
mov  edx, 0x41426F6E  ; ABon (CBonusBase)
mov  ecx, 0x61757261  ; aura
call 0x6F073450       ; call instruction at file 0x221C89
```

Both factory and pool cleanup reference `.?AVCAbilityAura@@` at file `0x56DE30`.
The adjacent plain name `CAbilityAura` is at `0x56DE44`, returned by the small
function at VA `0x6F221D40`. FOURCC immediates have reversed bytes on disk:
`aura` is numeric `0x61757261`, stored as bytes `61 72 75 61` (`arua`).

## Limits and misleading approaches

- **`ACat` is absent from this DLL**, in both byte orders. The actual registration
  is `"aura" -> CAbilityAura`. The output is a type registry: internal base types
  such as `abil`, `AAsp`, and `aura` are included. Expanding object-data ability
  IDs requires separately establishing their relationship from the matching game
  data; this tool does not guess aliases or substitute a superclass.
- Names alone do not determine the registered ID. This DLL registers
  `AOws -> CAbilityStomp` and `Awar -> CAbilityWarStomp`.
- `CAbility*` is an explicit output filter, not the complete inheritance subtree:
  `CBonusBase`, `CPower`, buffs, and fire effects also descend from abilities.
  Use `--all-classes` to inspect them and their parent IDs.
- `tools/ability_map.c` has a manually transcribed `classmap[]`; it only validates
  class-name presence against RTTI. Its mappings are not extracted call-site
  evidence. This extractor does not use that table or modify gameplay handlers.
- RTTI descriptors at `name - 8` need not have conventional MSVC RTTI cross
  references here. The allocator callbacks reference the name bytes themselves.

See [ability implementation](ability-implementation-plan.md#disassembly-optional-evidence)
for using disassembly as version-specific behavior evidence, and
[diagnostic tools](../../diagnostic-tools.md#warcraft-iii-demo-ability-class-extraction)
for the tool index.
