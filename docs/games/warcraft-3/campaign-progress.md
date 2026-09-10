# Warcraft III Campaign Progress

## Ownership and Commit Boundaries

`progress_def` describes one engine-owned `CAMPAIGNPROGRESS` root. Game and menu acquire it through their mandatory
`StateAcquire` import and observe the same allocation across maps and menu transitions. The game directly mutates this root
and explicitly commits campaign events. Menu refresh only reacquires a view; it performs no disk reload or file writes.
`Com_StateAcquire` resolves `campaign.w3p` under the current game's writable directory. See the
[engine persistence contract](../../architecture/persistence-plan.md#implemented-migration-2026-09-10).

| Event | Persistent change |
|---|---|
| `SetTutorialCleared` | Tutorial-cleared state |
| `SetCampaignAvailable` | Explicit campaign availability |
| `SetMissionAvailable` | Explicit mission availability |
| `SetOpCinematicAvailable`, `SetEdCinematicAvailable` | Opening/ending availability |
| Successful `G_LoadMap` for a campaign map | Canonical map visit |
| Connected player's single-player victory | Canonical map completion, also visited |
| Normal Save/Load Game | Full world snapshot; does not roll profile progress back |
| `SaveGameCache` | Commit the named script cache; does not commit the profile |

The profile is independent of `wc3_gamecache_mode`. It is scoped to the per-game
user directory; there is no separate named-player profile selection yet.

This follows the state/lifetime distinction in the local Quake II reference:
`data/Quake-2-master/game/g_save.c` saves `game` and clients in `WriteGame`, while
`WriteLevel` saves map-local state. Cross-level triggers modify `game.serverflags`
in `g_target.c`. Warcraft-specific profile records preserve that separation while
also being readable by the client menu when no server map is running.

## Shared Serialization

The engine's `shared/source/state.c` owns the bounded buffers, checksum envelope and replacement transaction. New profile
records contain a `W3PR` v2 header and the whole pointer-free `CAMPAIGNPROGRESS` block, followed by `STOK` and its checksum.
The old v1 mapped schema and `W3OK` envelope are read-only import contracts in `wc3_progress.c`; originals remain intact until
commit succeeds. Explicit record revisions must change when native layout semantics change, even if size stays the same.

It stores nine campaign records, 128 mission availability values per campaign,
tutorial state, and up to 512 canonical map visit/completion records. Availability
is an enum: unset, explicitly locked, explicitly open. Map history uses independent
visited/completed bits; it does not invent completion from unlocking a later map.
Map paths are case-folded and normalized to forward slashes, matching VFS identity.
Only the two campaign archive path families create automatic visit/completion records.

Updates validate native indexes and mutate the live root. A failed commit preserves disk state and leaves live edits pending
for retry; repeated events can retry that commit. Corrupt disk data is rejected before acquisition returns an allocation.
Menus diagnose acquisition failure instead of replacing it with an empty writable profile. Deleting a file while its root is
live does not reset progress: tests modelling a fresh profile must explicitly reset the engine store.

World restore suppresses profile events from map bootstrap, and does not roll the profile back to an older snapshot.

There is no conversion of the former played cvars, `campaign-progress.orcp` profiles, or version-1 `ORGCACHE` files.
Hero sidecars use a `W3GC` version-4 header with the engine machinery. Their
pointer-free value unions use native binary blocks, with tag and string validation
before committing or publishing a loaded cache. Earlier sidecars are rejected.

## Native Numbering and Menu Defaults

Verified from archive `Scripts/Blizzard.j`, `UI/CampaignStrings.txt`, and
`UI/CampaignStrings_exp.txt`:

| Campaign | Global cinematic index | Mission/campaign native offset |
|---|---:|---:|
| ROC Tutorial | 0 | 0 |
| ROC Human | 1 | 1 |
| ROC Undead | 2 | 2 |
| ROC Orc | 3 | 3 |
| ROC Night Elf | 4 | 4 |
| TFT Night Elf | 5 | 0 |
| TFT Human | 6 | 1 |
| TFT Undead | 7 | 2 |
| TFT Orc | 8 | 3 |

`SetMissionAvailableBJ` decomposes its packed mission index into expansion-local
campaign offset and zero-based mission ordinal. `SetCampaignAvailableBJ` converts
global campaign indexes to those offsets. Cinematic natives receive global indexes
directly. `campaign_offset()` and the shared `campaign_ids` table capture that
native ABI. The loaded map's campaign path selects the campaign/mission offset
domain, even when a direct CLI launch uses the other expansion's configuration.
Custom maps have no campaign path domain and use the active `fs_expansion` instead.
Do not use a compressed campaign-list row index as a native campaign ID. TFT's
`CampaignList` contains empty entries and uses a different menu ordering.

The menu parses `DefaultOpen` from the authoritative campaign file. Explicit
profile availability overrides defaults. An unmodified campaign exposes its first
mission and previously visited missions; an explicit mission lock takes precedence.
Named campaign shortcuts follow the same campaign availability rules as list clicks.
Open cinematics default to available with an available campaign; explicit native
locks hide them. Ending cinematics require an explicit native unlock. Intro rows
remain archive-authored. Tutorial-cleared state is stored independently.

## Investigation and Verification

The removed frontend path set `wc3_campaign_played_<campaign>_<mission>` through
`Cvar_Set`, which created an unarchived cvar. A bounded Human01 run with temporary
logs in `Cvar_Set` and `Cvar_WriteConfig` confirmed `flags=0` and omission from the
written config. Those logs were removed. `git blame` traced the old frontend bridge
to `f99f8b785`; it was not a completed profile persistence implementation.

Tests use fixture MPQs and isolated temporary profile paths:

```sh
make openwarcraft3-tests test-menu
XDG_DATA_HOME=/tmp/wc3-tests build/bin/openwarcraft3-tests -data build/tests +dedicated 1 +test 'wc3_persistence.*' +com_frame_limit 2
XDG_DATA_HOME=/tmp/wc3-tests build/bin/openwarcraft3-tests -data build/tests +dedicated 1 +test 'wc3_save.*' +com_frame_limit 2
```

The persistence suite covers explicit cache commits, disk reload and hero/inventory
restoration, native availability writes, overlapping ROC/TFT offsets, merged map
history, invalid indexes, failed writes retaining the old cache, and corruption/
truncation rejection. The menu suite reads actual committed profiles, checks
script locks and fresh-profile defaults, and verifies that launch requests do not
write cvars or mark maps visited. Normal save tests cover the shared walker's
world-owned fixups and JASS snapshots.

See also [Save/Load](save-load.md), [Campaign Game Cache](campaign-game-cache.md),
and [UI Flow](architecture/ui-flow.md).
