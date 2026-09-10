# Engine-Owned Persistence Plan

Status: migration implemented; historical investigation and remaining extensions follow. Investigation baseline: local `main` at `2c04b4c2` and
`refactor/wc3-campaign-save-records` at `4f679564` (2026-09-09). This is a source/history review; the concerns below are
structural observations, not runtime failures established through diagnostic logs.

## Implemented migration (2026-09-10)

- `common/state.h` defines the engine buffer/schema/store API. `shared/source/state.c` owns its implementation in
  `libshared`, ensuring a single process-wide store rather than a copy in each DLL. `Com_StateAcquire` qualifies logical keys
  through `FS_UserPath`; game and menu imports expose `StateAcquire` and `StateCommit`.
- Acquired profile/cache roots use native value blocks with explicit game revisions. Acquiring the same path/lifetime with
  another compatible definition returns the same allocation. The store copies paths and numeric contracts, retains no module
  schemas or validators, and frees allocations at filesystem shutdown. Validators run synchronously at acquisition; game
  mutations validate their inputs before commit. Same-sized semantic layout changes require a revision bump.
- `progress_def` supplies the shared `CAMPAIGNPROGRESS` contract. Both modules access the engine allocation. Profile events
  mutate it directly and commit; failed writes leave the live edit pending for retry. Menu refresh performs no disk reload.
- JASS cache handles remain private working copies. `SaveGameCache` publishes to engine-owned committed storage only after
  replacement succeeds. Disk, process-memory and disabled modes remain available. Basename/sanitized logical cache keys
  preserve the disk alias mapping. There is no longer a separate eight-slot cache store in the game DLL.
- `save_fields` walks both mapped payloads and copied native images, with explicit `child`/`ring` schema members. WC3 supplies
  one identity callback for both representations; ordinary edict/client values remain memcpy-shaped. Moves and C callbacks
  use checked symbol rosters. JASS continues using its semantic snapshot through bounded engine memory buffers.
- World exports take buffers, never filenames. Engine `SLOT` v1 metadata identifies the game and map without loading a game
  module; the mandatory `CheckSave` callback checks the game payload revision before teardown. The server owns slot transactions and retains validated bytes across map
  teardown. Native `SaveGame` queues an engine save command so the VM has returned; native `LoadGame` uses the existing
  deferred session transition. Restore bootstrap suppresses profile events and blocks cache commits. On success the engine
  rebuilds baselines after resource/reference restoration; a late failure stops the incomplete session.

### Formats and guarantees

New envelopes end in `STOK` plus the existing FNV-1a integrity checksum. Profile records write `W3PR` v2 and committed caches
write `W3GC` v4, each containing the whole pointer-free value block. Their v1/v3 mapped formats and `W3OK` envelopes are
read-only import contracts supplied by WC3; successful acquisition keeps the source intact until a new commit succeeds.
Main's `ORGCACHE` v1 also has a read-only importer: its exact two-word magic selects a bounded, table-driven decoder for
little-endian words, length-prefixed strings and tagged values. It preserves Hero attributes, learned abilities and inventory.
Invalid versions/tags, truncation and trailing data reject acquisition without publishing an empty replacement. These private native value layouts are
revisioned, not promised as portable retail formats. They use the supported engine ABI's DWORD/enum/float layout.

Live roots share a 256 MiB aggregate budget; individual serialization buffers are bounded to 256 MiB.

World format v16 adds a producer layout/byte-order signature, replaces relative move addresses with symbol identities,
and serializes client camera targets through the same image reference schema as entity pointers.
Older native world images are deliberately rejected. Keep both the explicit revision and layout signature: neither `sizeof`
nor the signature can identify a same-size semantic field change without a revision bump.

Every write builds bounded scratch bytes, writes and flushes a sibling temporary file, syncs/closes it, then replaces the
old path with POSIX rename or Windows replacement. A reported pre-replacement write failure preserves the old destination.
The single-threaded store does not support concurrent writers. POSIX parent directories are not fsynced, so this is not a
power-loss durability guarantee. Checksums are integrity checks, not authentication.

### Existing world-state inventory and remaining extensions

| State | Current contract |
|---|---|
| `game.clients`, active `g_edicts` | Native blocks, declared pointer/runtime fixups; resource pointers rebound after load |
| Level time/time-of-day, camera bounds, waypoint ring, weather, quests, triggers, timers, unread events | Existing mapped schema retained; no second DTO populated during save |
| Dynamic groups and JASS VM | Stable slot allocation followed by reference resolution; semantic VM byte payload |
| Profile / committed caches | Separate engine roots; loading a world does not replace them |
| Map metadata, spatial links, models/animations, HUD callbacks, network history | Runtime/rebound; server simulation time restored independently of transport counters |
| Bots, fog/exploration, alliances, some level presentation state and random state | Existing coverage gaps; this plumbing migration does not claim new world-state coverage |

Whole-world semantic staging before teardown remains an extension. Map/script resources and baseline quest identity are still
needed before complete decoding. No rollback to the previous live match is promised after a resource-dependent restore error.
Consolidation of all level value fields into a fully inventoried live root remains follow-up work.

### Verification

```sh
make test  # Includes common/tests/t_state.c in test-core, plus all game suites
make test-wc3-engine WC3_PATTERN='*'  # ROC and TFT: wc3_persistence.*, wc3_save.*, other game suites
make test-menu
make build openwow opensc2
```

`common/tests/t_state.c` is compiled into the umbrella target's core test executable. It covers slot metadata validation,
shared acquisition, restart, stale IDs, version conflicts, failed commit preservation,
private-copy publication, pending live edits, restore commit suppression and read-only legacy import. Campaign/menu fixtures
explicitly call `state_reset` when modelling a process/profile restart; deleting a file alone intentionally does not invalidate
a live root. Existing behavior and save tests remain in their game suites.

Runtime verification used `XDG_DATA_HOME=/tmp/persistence-verified-runtime` to keep player state isolated. A command file
containing 100 `wait` lines followed by `save engine-roc` (or `save engine-tft`) was executed after map loading;
runs used `-com_fast_forward +set vid_hidden 1 +set r_norefresh 1 +com_frame_limit 180`. Fresh processes loaded each slot with
`+load <slot> +com_frame_limit 100`. ROC `Maps/Campaign/Human02.w3m` restored 2,687 entities; TFT
`Maps/FrozenThrone/Campaign/HumanX01.w3x` restored 4,729. Both reached a subsequent `CL_SendBegin`/`G_ClientBegin`. Loading the older ROC slot after the TFT run left the shared
profile SHA-256 unchanged. Final `make test` passed, including 801 core assertions (158 tests), 23,698 assertions in 1,034
engine tests for each expansion, 1,258 menu assertions, and 1,227 server-network assertions; all three game executables built successfully.
Existing missing-art warnings and TFT `SetCampaignMenuRaceEx`/`SetAltMinimapIcon` unimplemented-native diagnostics remain.
SDL needs local display access, and network tests need loopback sockets; the restricted sandbox cannot provide those.

## Original design and review

## Recommendation

Give the engine responsibility for persistent storage and serialization. Games own typed, authoritative state and describe
its layout. Gameplay changes that state directly; saving does not assemble another copy of every gameplay field by hand.

Use binary blocks for ordinary value state and a small declarative schema for references, owned allocations, and transient
fields. Start with profile records, then use the same storage and serialization machinery for world snapshots. Keep profile
progress and a saved match separate because their lifetimes and commit rules differ.

Do not revert the entire branch to main. Preserve campaign behavior and tests, then replace the persistence plumbing in
bounded changes. A clean implementation branch can start from updated main and carry those behavior changes separately.

## What The Branch Actually Changed

The last two commits are the persistence refactor: `524fda93` and `4f679564`. The comparison with main also includes earlier
campaign-unlock work and unrelated fixes. Compare against `8f7b5c99` to isolate those two commits.

| Observation | Evidence and implication |
|---|---|
| The refactor does remove duplication. | `g_gamecache.c` removes its bespoke scalar/union encoding; shared checksum and record code moves to `common/wc3_save.c` under WC3. These are useful changes to retain conceptually. |
| The engine API is unchanged. | `server/game.h` still exports filename-based `SaveGame`, `LoadGame`, and `GetSaveMap`; `server/sv_init.c` delegates to them. The engine cannot serialize a game-described root. |
| Shared machinery remains game-owned. | `wc3_save.h` exposes `FILE *`, and its type enum includes JASS functions, triggers, and timers. Moving that header unchanged into engine common would cross the game boundary. |
| Two serialization approaches still coexist. | `g_save.c` uses `WriteField1`/`ReadField` on raw edict images and `GameSaveField` plus `save_fields` on mapped streams. Client camera targets have a separate path. The same reference concepts need several implementations. |
| Field metadata is overloaded. | `field_t.flags` holds either flags or a child-schema/ring pointer. `F_METADATA_*` macros encode that overload. Explicit schema members would make the contract easier to inspect. |
| Progress is already one struct, but files communicate its state. | `progress_change()` allocates, loads, changes, saves, and frees `CAMPAIGNPROGRESS` per update. `SinglePlayer_ReadProgress()` reads another copy from disk. Engine-backed storage would provide one owner across menu/map lifetimes. |
| Commit behavior differs between record types. | `save_record()` stages small records through temporary/backup paths; `WriteGame()` opens the destination with `w+b` and removes it on a later failure. Centralizing commit handling would make both use the same preservation guarantees. |
| A checksum is not complete load validation. | `ReadGame()` checks the envelope/header, then changes registries, level state, clients, and edicts while decoding. `SV_LoadGame()` loads the map before calling it and reloads a baseline on failure. Neither constitutes an atomic world restore. |

Useful history commands:

```sh
git diff main...4f679564 -- games/warcraft-3/common games/warcraft-3/game/g_save.c games/warcraft-3/game/g_gamecache.c
git diff 8f7b5c99..4f679564 -- games/warcraft-3/common games/warcraft-3/game/g_save.c games/warcraft-3/game/g_gamecache.c
git blame -L 30,49 4f679564 -- games/warcraft-3/common/wc3_save.h
git blame -L 32,72 4f679564 -- server/sv_init.c
```

## Quake References

Quake II's `WriteEdict`, `WriteClient`, and `WriteLevelLocals` copy runtime structs, replace pointer fields using `field_t`,
and append allocated strings. Ordinary scalar fields survive without another field-by-field save DTO. Its original callback
and movement identities use executable/data offsets, which are tied to the producing build.
[Source: Quake II game/g_save.c](https://github.com/id-Software/Quake-2/blob/master/game/g_save.c).

Quake II's engine manages slots, server state, map transitions, configstrings, and area portals, but passes filenames to the
game's `WriteGame`/`ReadGame` and `WriteLevel`/`ReadLevel`. It does not provide the complete engine serializer proposed here.
Keep its lifecycle/ownership split while moving byte encoding and filesystem handling behind the engine API.
[Source: Quake II server/sv_ccmds.c](https://github.com/id-Software/Quake-2/blob/master/server/sv_ccmds.c),
[game API](https://github.com/id-Software/Quake-2/blob/master/game/game.h).

Quake I is the closer reference for engine-managed serialization of game-defined data: `ED_Write` walks QuakeC field
definitions, and `ED_WriteGlobals` walks globals marked `DEF_SAVEGLOBAL` for its supported types. `Host_Savegame_f` writes
those records. C game modules would supply equivalent metadata explicitly; the engine cannot discover C pointer semantics.
[Source: Quake pr_edict.c](https://github.com/id-Software/Quake/blob/master/WinQuake/pr_edict.c),
[host_cmd.c](https://github.com/id-Software/Quake/blob/master/WinQuake/host_cmd.c).

Local reference trees, when available, are under `/Users/igor/Developer/idTech/Quake-2-master/` and `Quake-master/`.

## Options

| Option | Cost and fit |
|---|---|
| Engine stores an opaque fixed-size block. | Smallest implementation; excellent for pointer-free profile state. Size, version, validation, and lifetime must be declared. Insufficient for existing pointer-rich entities or the VM by itself. |
| Engine stores blocks and applies game-supplied schemas. **Recommended.** | Keeps ordinary state memcpy-shaped and preserves current C gameplay code. One generic walker handles references and arrays; game-specific identity and VM logic stays in the game. |
| Rewrite all live state into one relocatable arena with offsets/IDs. | Can approach one memory-image snapshot. Requires changing entity references, allocation, JASS handles, and runtime resource ownership throughout the game. Consider later for individual pools; making it a prerequisite expands this task substantially. |
| Per-field string/key-value storage like cvars. | Adds lookup/conversion and another state representation to ordinary gameplay. Keep the useful registration/lifetime model, but store typed binary objects. JASS game-cache keys remain because they are part of that scripting API. |

## Minimal Engine Contract

Implement shared storage in `common/` and world-save orchestration in `server/`. Declare types in a small engine header and
expose mandatory import callbacks through both `game_import` and `menu_import`. Game modules should no longer open save
files or calculate save paths/checksums. Schemas, campaign rules, and JASS codecs stay under `games/<game>/`.

A record definition contains a logical key, lifetime, explicit format revision, byte size/alignment, encoding mode, and any
schema/validator. The engine qualifies keys by game/profile. Keys identify records inside a container; they are not filenames.

Conceptual call shape (names are provisional):

```c
/* Once during module setup: returns status, a stable handle, and aligned engine-owned storage. */
gi.StateAcquire(&profile_def, &profile);

/* Normal gameplay writes the authoritative object; there is no save-only field gathering. */
LPCAMPAIGNPROGRESS state = profile.data;
state->campaigns[id].missions[mission] = PROGRESS_OPEN;
gi.StateCommit(profile.id, profile.data);
```

`StateAcquire` distinguishes loaded, newly initialized, incompatible/corrupt, and I/O failure. It must not hand callers a
writable empty replacement for an invalid existing record. `StateCommit` validates and writes the supplied typed object.
For direct profile edits, failure preserves the previous disk record and leaves the live object pending for retry. For a
separate cache working copy, publish its whole value into the engine's committed object only after the commit succeeds.
The same call accepts either source; individual fields are never gathered by the storage layer's caller.

Use the same game-owned definition from game and menu common code. Repeated acquisition resolves the same engine object,
checks matching contracts, and performs no repeated disk load. Menu uses a const view. Profile storage survives map teardown
and menu unload. Copy descriptor metadata into engine storage; module-owned validator/codec callbacks are bound only while
their owning module is loaded and detached at shutdown. Do not keep executable pointers into an unloaded module.

Initially commit only at explicit semantic boundaries, so no dirty-detection framework is necessary. Several script changes
may be batched at the next safe host boundary, but commit-before-transition/shutdown must be enforced and errors returned.
World saves requested inside script execution are queued until the VM is safe; a synchronous native may report success only
when its promised commit has actually completed. Do not silently turn a synchronous cache-save result into eventual success.

### Lifetime and authoritative state

| Lifetime | Root and behavior |
|---|---|
| Profile | One `CAMPAIGNPROGRESS` root. Loaded once per profile; game mutates it and menu reads it. Survives map changes and loading older world snapshots. |
| Committed campaign cache | Engine stores committed named `gameCache_t` value records. Each JASS handle retains its mutable working cache; only `SaveGameCache` publishes it. Another `InitGameCache` must not see uncommitted `Store*` edits. |
| World snapshot | One game-declared root/schema references persistent level state, clients, entity and handle pools, and VM state. Engine saves all required blocks in one slot transaction. |
| Runtime | Assets, metadata pointers, paths/flow fields, spatial links, renderer handles, network transport, and UI callback caches are rebuilt. They are not persistent ownership roots. |

The cache working/committed distinction is required behavior, unlike copying progress just to communicate between modules.
A loaded world may contain old cache working copies; it must not overwrite the durable cache until the script explicitly saves.
Preserve the configured disk/memory/disabled cache behavior when moving its backend into the engine.

The suggested 1 KiB is an interface example, not a world-size limit. Today the progress struct alone reserves 512 map records
with 256-byte paths plus flags, before campaign flags. Begin with `sizeof(CAMPAIGNPROGRESS)` rather than inventing a limit or
discarding history. Compact flag storage or a separate counted history block can follow measurements.

### Structs and schemas

The world root is the live ownership root, not a second `SAVEGAME` struct populated on demand. Group authoritative state by
lifetime as it is migrated. Contiguous entity/client arrays can remain separate allocations referenced by that root; the root
does not need to physically contain every possible entity. Existing `game`, `level`, and `g_edicts` can be bound directly in
the first migration, then consolidated without changing the storage API. Do not change network structs for this work.

One walker supports explicit block encoding contracts:

- **Native image:** copy a struct, clear full runtime/reference slots in the output, encode references and owned children,
  and preserve the remaining values. No scalar-by-scalar population is needed. This is the transitional mode for edicts.
- **Defined value layout:** pointer-free blocks with explicitly specified byte layout, or a field schema encoding fixed-width
  values. Prefer this for long-lived profile/cache formats that should survive compiler/platform changes.

These are declared compatibility contracts, never failure-triggered fallbacks. Reuse `bzFieldType_t` for scalar conversions;
add serialization operations for blocks, references, counted children, runtime exclusions, and a bounded custom payload.
Keep child schema, flags, count/stride, and reference-domain members distinct. Do not move WC3's `F_TRIGGER`/`F_TIMER` enum
into the engine: the engine sees a reference domain; WC3 supplies its table/identity rules.

One descriptor handles each concept in both image and field-encoded blocks. Entity references become slot IDs, static moves
and C callbacks become explicit roster IDs/names, and resource identities resolve from authoritative names/rawcodes. Avoid
new module-address offsets. Preserve null sentinels, unused pool holes, aliases, and cycles. Allocate all destination slots
before resolving references; validate domain, range, occupancy rules, and count/size arithmetic before publishing them.

A raw C struct is not automatically portable. Native images require an explicit producer ABI/build identity and record
revision; `sizeof` alone misses same-size field changes. Profile/cache layouts need stable byte order and fixed-width fields,
or an explicit migration. Initialize reserved bytes; never persist allocator metadata or padding as meaningful state.

The JASS VM remains a bounded semantic payload with game-owned handle codecs. Its existing `JASSSNAPSHOT` already accepts
a context plus byte-transfer callback; `WriteJassBytes`/`ReadJassBytes` currently adapt that to `FILE *`. Adapt those callbacks
to engine memory buffers instead. Do not dump VM parser pointers, stacks, or coroutine process state as a raw arena.

### Files and load lifecycle

Use one engine container implementation for profile records and save slots: generic header, version/game identity, bounded
block directory, payload, integrity data. Logical records do not each require a private game file format. The engine owns
enumeration, deletion, temporary files, flush/close checks, and platform-correct atomic replacement. Keep the previous slot
valid on every failed write; implement recovery and durability policy explicitly instead of assuming a checksum ensures it.

The engine reads map identity and content requirements from its slot header before map teardown. The game contributes these
as typed metadata; the engine should not parse a WC3 header via `GetSaveMap`. Record engine-owned semantic state such as the
simulation clock and necessary media identities; rebuild transport/session counters instead of rewinding network history.

Load sequence:

1. Read into bounded scratch memory and validate the whole envelope, required blocks, sizes, versions, and available content
   identities before disturbing the current map. Decode/check scalar values and references into staging storage.
2. Enter an explicit restore mode, load map/program resources, and allocate saved native registries before resolving handles.
   Keep simulation and client activation paused. Preserve Q2's resource/map initialization before game-state restore.
3. Resolve references, restore VM state, and finish game semantic validation. Commit the staged state at a controlled boundary.
4. Rebind metadata/resources, rebuild spatial links once, restore simulation time, and refresh client presentation/baselines.
   Allow `ClientBegin`/active gameplay only after this succeeds.

Existing WC3 map loading runs JASS `main()` to create a baseline registry. Preserve that dependency during initial migration,
but restore mode must prevent its profile mutations, cache commits, and presentation side effects from escaping. Eventually derive
mutable registry slots from the save itself. Removing the `main()` dependency requires explicit coverage of quest identity and
other currently baseline-dependent handles; do not simply delete the call.

Do not promise that preflight alone preserves the old live match on every failure. Resource-dependent restore errors may occur
after map teardown. The first migration must leave a clean stopped session with a specific error if this happens; keeping the
old match running until every stage succeeds requires separate world contexts or a tested rollback snapshot. Treat that as a
separate extension, not hidden complexity in the basic blob API.

## Delivery Plan

| Step | Concrete change | Completion evidence |
|---|---|---|
| 1. Engine block store | Add bounded buffers, record acquisition, version/status checks, shared commit implementation, and import entries. Keep existing game formats active until their consumers move. | Independent engine tests for acquire/reacquire, missing versus invalid, restart, failed writes preserving old bytes, and size/version rejection. |
| 2. Profile vertical slice | Point game/menu at one engine-owned progress struct. Replace `progress_load`/`progress_change` filesystem work with normal typed mutations and commits. | ROC/TFT unlock/lock/default behavior and path normalization survive process restart; menu observes updates without file reload; loading an older world never rolls profile back. |
| 3. Campaign cache | Register committed cache records with the engine; retain JASS working-cache semantics and game validation. Remove game-owned disk/memory backend duplication. | Existing typed values, Hero/item restore, cache alias rules, unsaved-edit isolation, and disk/memory/disabled modes pass. Failed commit changes neither durable nor published committed cache. |
| 4. World serializer | Describe existing live roots/pools through one engine walker. Adapt JASS byte callbacks and game identity codecs. Replace filename exports with schema/buffer/lifecycle exports. | Entity/client/group/quest/timer/trigger references, callback/move IDs, strings, rings, and sleeping coroutines round-trip across processes; no game `FILE *` save path remains. |
| 5. Restore boundary and cleanup | Wire engine slot metadata/preflight and restore mode; remove superseded record walkers and file formats from the active write path. | Bounded real ROC/TFT save/relaunch/load runs resume gameplay; restore has no profile writes, duplicate links, premature client activation, or partial-world continuation. |

Before step 4, inventory persistent state explicitly: level, clients, entities, AI state, RNG state, fog/exploration, queues,
and every native handle domain. Mark each field authoritative, derived, transient, or an explicitly documented gap. Merely
moving the existing writer will not automatically save currently unmapped state.

Compatibility policy: retain read-only importers for known profile/cache formats (including main's `ORGCACHE` v1 and this
branch's `W3GC` v3/`W3PR` v1), preserve originals until the new commit succeeds, and test migration fixtures. Full native-image
world saves remain explicitly build/version-bound; reject unsupported old images before teardown rather than guessing at
their layout. Keep a legacy reader only for versions whose complete layout can be validated.

Existing tests to preserve are `wc3_persistence.*`, `wc3_save.*`, JASS snapshot tests, and campaign menu tests. Add focused cases
for checksum-valid malformed data, out-of-range/duplicate IDs, alias/cycle restoration, callback roster mismatch, allocation
failure, and failures after map teardown. Verify the new API in all game builds with explicit unsupported-save behavior where
appropriate. Run bounded ROC/TFT gameplay validation and `make test` before committing implementation changes.

The first implementation should deliver steps 1–2. That proves the requested engine-owned typed-buffer model with a small,
visible feature before touching the larger entity/VM lifecycle.

See also: [current WC3 save/load](../games/warcraft-3/save-load.md),
[campaign progress](../games/warcraft-3/campaign-progress.md),
[campaign cache semantics](../games/warcraft-3/campaign-game-cache.md),
[runtime paths](runtime.md).
