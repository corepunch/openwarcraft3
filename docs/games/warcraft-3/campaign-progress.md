# Warcraft III Campaign Progress

## Ownership and Commit Boundaries

Campaign progress is game state, not configuration. `g_progress.c` commits campaign
script events and successful map visits to `campaign.w3p` through `gi.UserPath`.
The file lives directly in the engine's per-game writable user directory, alongside
campaign game-cache sidecars. The menu resolves that same directory through the
mandatory `menuImport_t.UserPath` callback and reads committed state when opening
or returning to the campaign menus. It never marks a map played merely because a
launch button was clicked.

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

`common/wc3_save.c` under `games/warcraft-3/` is shared by the game and menu modules.
Its `SAVEFIELD` walker is the extracted mapped-record path from `g_save.c`:

- scalars and bounded inline strings;
- nested structs, counted arrays and rings;
- tagged union schemas for game-cache entry values;
- a game callback for world-owned pointer/JASS identity conversions.

`campaign.w3p` uses a native-binary header (`W3PR`, version 1, payload struct size),
the table-described payload, and the same `W3OK`/FNV-1a footer as normal saves.
It stores nine campaign records, 128 mission availability values per campaign,
tutorial state, and up to 512 canonical map visit/completion records. Availability
is an enum: unset, explicitly locked, explicitly open. Map history uses independent
visited/completed bits; it does not invent completion from unlocking a later map.
Map paths are case-folded and normalized to forward slashes, matching VFS identity.
Only the two campaign archive path families create automatic visit/completion records.

Each update reloads the latest committed profile before applying one change, so
map transitions and menu reads cannot overwrite newer state with stale copies.
Repeated unchanged events do not rewrite the file. Invalid native indexes are
reported and do not mutate the profile.

`save_record` stages a complete checked file, backs up the previous file and
installs the replacement. On a reported installation failure it attempts to restore
the backup. This is not a crash-durability guarantee. `load_record` validates into
scratch memory; malformed/truncated files cannot partially update a live struct.
Profile updates refuse to overwrite a corrupt file. Menus keep their last valid
snapshot when a read fails (or initial defaults when none exists), with a diagnostic.

There is no conversion of the former played cvars, `campaign-progress.orcp` profiles, or version-1 `ORGCACHE` files.
Hero sidecars now use their own `W3GC` version-2 header with the shared machinery.

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
