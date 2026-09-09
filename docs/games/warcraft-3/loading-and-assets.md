# Campaign loading and asset resolution

For measured menu/loading resource residency and reclamation priorities, see [WC3 memory](memory.md).

## Loading-screen ownership

`CL_BeginLoadingMap` publishes the resolved destination and freezes a non-interactive loading plaque.
The client remains `ca_connected` while registering assets; the first usable server frame activates it once
`CL_PrepRefresh` has completed. `SCR_DrawCursor` hides both the native and authored cursors during loading.

The game binds native `UI/FrameDef/Glue/Loading.fdf` in `UI_LoadHudLoading`. During the initial handshake,
`SV_Configstrings_f` calls `ClientLoading` before sending the media table. `UI_WriteLoadingLayout` reads
`level.mapinfo` and resolves WTS text through `UI_LevelStringSafe`. A custom W3I loading model takes precedence;
otherwise the campaign background number selects a model and sequence from `UI/WorldEditData.txt`'s
`LoadingScreens` section. Maps without either use the decorated `LoadingMeleeBackground` skin entry.

Both background and progress bar retain the FDF's `FT_SPRITE` type. The background uses `#!sequence` for native
screen-space geometry. The zero-size bar uses `#0` plus `UI_STAT_LOADING_PROGRESS`, a client-local normalized
binding, so `SCR_LayoutDrawSprite` supplies `#0@ratio`. No player-state field or network layout size changes.
`FT_LOADING_BAR` is the image-bar contract used by WoW; an image and a model may have the same numeric index.

### September 7 initial-layout regression

Commit `aa5f89f73` moved the loading screen from the menu to the initial server layout but lost campaign-row
resolution and changed native sprites into portraits. A bounded Human02 trace confirmed background number `3`
was registered as nonexistent `LoadingMeleeBackground.mdx`, while the bar's portrait viewport was zero-sized.
Once images registered, the same model index (`1`) selected an unrelated image. Restoring native sprite types,
the ROC/TFT row parser, and an explicit progress binding fixes all three without changing FDF geometry.

The server-layout lifecycle still publishes the screen only after synchronous `SV_Map` finishes, and its resources
become available during client registration. It cannot display destination artwork during the earlier server-load
phase; providing that requires a separate earlier publication lifecycle. Do not fabricate progress for that phase.

### June 28 regression

Commit `b6349c392` removed `cl.loading_map`/`GetLoadingMap` but left `CL_BeginLoadingMap` ignoring its
`mapName` argument. UI lookup could only see a startup `+map` cvar, explaining why direct map launches
worked while campaign buttons produced a black background plus LOADING. Runtime diagnostics for Human01
showed a valid requested destination and an empty UI map cvar. In addition, the UI looked up its own
cached path, preventing invalidation on subsequent maps. Both paths now use the published destination.
TFT has an additional independent schema difference: ROC `LoadingScreens` rows are
`label,sequence,model`, while TFT rows are `expansion-category,label,sequence,model`, including ROC campaigns
under TFT archives. Fixed ROC indices interpreted HumanX01's sequence `6` as a filename. `UI_ParseLoadingRow`
consumes the optional numeric category and validates the sequence/model fields; malformed rows log their key.

In that menu-owned implementation, the loading-state draw check had to precede standalone-screen dispatch because
`menu_ingame` is queued asynchronously. The initial-layout path now dispatches loading from `SCR_DrawScreenField`.

### Historical menu FDF registry isolation

`libmenu` and `libgame` intentionally have separate `stb_fdf` frame registries.  The parser implementation functions are hidden per shared library, and the `frames[]` backing store must be hidden as well.  On ELF/Linux, exporting `frames` allows normal symbol interposition to alias the two registries even though each library defines its own copy.  `G_LoadMap -> UI_ResetHud -> UI_ClearTemplates` then clears/reuses the menu's live `Loading`/`LoadingBar` slots during `SV_Map`; cached non-NULL loading-frame pointers survive but no longer describe the loading sprites, so later progress updates reach the old `M_DrawLoadingScreen` without reaching the MDX renderer.

A characteristic failure is that the initial loading sprite renders before `SV_Map`, while later progress values change without reaching the MDX sprite renderer. Verify registry isolation before changing progress math or swap/present code.

## Level-transition asset ownership

A campaign `ChangeLevel` is a **map replacement**, not a mutation of the current world. The client clears its
map-owned renderer handles before the next registration sequence, while `G_LoadMap` clears the server-side model
metadata cache because `CS_MODELS` indices restart for each `SV_Map`. The WC3 map loader releases the previous
map's W3I/WTS/object-modification allocations, placements, terrain arrays, and pathing storage before reading the
destination archive. These boundaries prevent a model index, trigger string, placement, or pathing allocation from
retaining the previous level by accident.

The WC3 renderer likewise replaces its map state at `_W3M_RegisterMap`: old terrain segment/layer VAOs, ground
layer lists, cliff model choices, fog render targets, minimap handle, terrain shadow, and source W3E arrays are
released before the destination world is built. Ground and cliff lookup caches are map-scoped and reset at the
same boundary. Do not append a new campaign map to the previous terrain lists.

### Map-imported model and texture overrides

The MPQ reader supports nested archive paths: a lookup such as
`Maps\Campaign\Human03.w3m\Textures\Foo.blp` opens `Human03.w3m` and then resolves the inner path. WC3 installs
the currently registered map as the renderer's generic map-asset scope, so ordinary model and texture references
resolve with this order:

1. `<current .w3m/.w3x>\<logical Warcraft asset path>`;
2. the ordinary base-data path.

The resolved scoped path is also the renderer cache key. This is required for consecutive maps that import
different bytes under the same logical name; a `Textures\Foo.blp` cached for Human02 must not satisfy Human03's
own `Textures\Foo.blp`. Missing scoped models/textures fall back to Blizzard data without turning the scoped miss
into a missing-resource placeholder. Texture fallbacks alias the scoped key to the resolved base texture so later
lookups do not repeat the nested-archive miss. Clearing client state clears the generic renderer asset scope so a
map import cannot continue overriding menu assets after disconnect.

Standalone renderer regression coverage exercises both a present scoped model/cache hit and a missing scoped
model that falls back to the base path, including scope clearing at a registration boundary.

This is intentionally narrower than a full Warsmash-style data-source stack. The current transition still does
**not** rebuild all WC3 SLK/TXT data per map, merge all `war3map.w3a/.w3t/.w3b/.w3d/.w3q` object modifications,
implement JASS `Preload`/`Preloader`, or expose staged byte/task loading progress. `war3map.w3u` now applies the
registered `UnitProfile`/`UnitUI` subset (including Required Animation Names and custom model paths), but the remaining
Balance/Data/Weapons/Abilities tables and true per-map `war3mapMisc.txt`/skin overlays remain separate data-layer work;
do not infer those capabilities from the renderer's map-import lookup.
## Loading progress contract

Loading progress is client-owned and intentionally coarse. `CL_BeginLoadingMap` resets `cl.loading_progress` to
zero. `CL_PrepRefresh` advances it monotonically at existing registration boundaries and
`SCR_UpdateLoadingPlaque` explicitly repaints the otherwise frozen Quake-style loading plaque after each advance.
The initial server payload carries the `Loading.fdf` tree as `svc_layout`; its `FT_SPRITE` bar binds
`UI_STAT_LOADING_PROGRESS` to the `LoadingProgressBar` MDX sequence using `#0@ratio`. No player-state/network field is involved.

Current phase values are:

| Progress | Completed boundary |
| ---: | --- |
| 0.05 | local listen-server `SV_Map()` returned successfully (local games only) |
| 0.10 | `CS_WORLD` is available and client refresh preparation has started |
| 0.40 | collision/world map and renderer map registration are ready |
| 0.60 | model configstrings are registered |
| 0.75 | image configstrings are registered |
| 0.87 | sound configstrings are registered |
| 0.94 | font configstrings are registered |
| 0.98 | client `begin` has been queued |
| 1.00 | sound registration is closed and `cl.refresh_prepped` is true |

These are **phase milestones, not byte percentages or time estimates**. In particular, local `SV_Map()` /
`ge->LoadMap()` remains synchronous and publishes only its completed boundary, not progress from inside its
server/game loading work, so the bar can remain at its initial position during a long map load. Do not manufacture sub-percentages for that work;
add progress only when an owned lifecycle can report a real boundary. The plaque stays visible at 100% until the
first usable server frame promotes the client to `ca_active` and `SCR_EndLoadingPlaque` runs.

## Asset names are data

| Source | Meaning / resolution |
| --- | --- |
| `Units/DestructableData.slk`, `texFile` | `_` (or empty/absent) means no replacement image. Preserve an authored extension; do not append `.blp` in the spawn code. Human01 `LT05` uses `ReplaceableTextures\Cliff\Cliff0.tga`; crates/gates use `_`. |
| Texture references ending in `.tga` | The exact file wins. If absent, `R_ReadTextureFile` tries the same stem with `.blp`, then normal renderer diagnostics/placeholders apply. This handles converted source names without overriding real custom TGA files. |
| `ReplaceableTextures\CameraMasks\White_mask.tga` | Human01's cinematic script requests this; the local ROC archive contains `White_mask.blp` (1422 bytes), not TGA. The cliff BLP is 90592 bytes. |
| Native `StandardTemplates.fdf` | Some unused templates reference art absent from ROC archives (`HeavyBorder*`, `LightBorder*`, `ButtonBackGround`, `ButtonCorners`, `GlueScreen-PlayerButton-BorderRight`, `GlueScreen-ROC-EditionButton-*`). Parsing records texture keys/indices; `UI_GetTexture` loads only on consumption. Do not replace missing art with invented paths or suppress renderer warnings for consumed resources. |

`UI_GetTexture` caches both real and placeholder renderer handles. Decorated names are re-evaluated on theme
changes; an unchanged theme does not reload. Parsing or inheriting a template alone must not fetch its art.

## Cliff-transition classification

Terrain vertices are ordered NE, NW, SE, SW by `GetTileVertices`; model configuration letters use
SW, NW, NE, SE. A transition requires exactly two **adjacent ramp corners one cliff level apart**.
Two same-height flagged corners belong to an adjoining ordinary cliff, not a sloped transition.

Human01 cell `(64,31)` yielded model-order levels `[1,0,1,1]` and ramp flags `[1,0,0,1]`.
The former `GetTileRamps(tile) > 1` test built nonexistent `CliffTransHABH0.mdx` and omitted its geometry.
Correct classification selects the ordinary `CliffsBABB0.mdx`. The native transition directory contains
L/H and H/X edge pairs, not an HH or LX edge. Do not turn file-not-found into a guessed terrain fallback.
The [Warsmash terrain loader](https://github.com/Retera/WarsmashModEngine/blob/main/core/src/com/etheller/warsmash/viewer5/handlers/w3x/environment/Terrain.java)
is a useful secondary reference for separating transition edges from regular cliff cells; local archive
contents and logged vertex metadata establish this case.

## Diagnostics and verification

```sh
build/bin/mpqtool -mpq 'data/Warcraft III/War3.mpq' cat Units/DestructableData.slk
build/bin/mpqtool -mpq 'data/Warcraft III/War3.mpq' ls ReplaceableTextures/CameraMasks
build/bin/mpqtool -mpq 'data/Warcraft III/War3.mpq' ls Doodads/Terrain/CliffTrans
build/bin/mpqtool -mpq 'data/Warcraft III/War3.mpq' cat UI/FrameDef/Glue/StandardTemplates.fdf
build/bin/openwarcraft3 -data 'data/Warcraft III' +map 'Maps/Campaign/Human01.w3m' +com_frame_limit 100
build/bin/openwarcraft3 -data 'data/Warcraft III' -tft +map 'Maps/FrozenThrone/Campaign/HumanX01.w3x' +com_frame_limit 100
make test-renderer-model test-ui
make test
```

For the menu regression, launch without `+map`, then select Single Player → Campaign → Human → the desired mission. The console
registers `menu_single_player_campaign`, but `menu_single_player_campaign_human` is a UI-handler command,
not a console command. Campaign selection now enters `MissionSelectFrame`; the selected mission's
`menu_single_player_mission_select N` handler is what issues the map command. A diagnostic replay can temporarily
invoke those handlers after showing the campaign screen; remove the hook afterward. Queue `screenshot 1` before
the mission handler to capture the frozen loading plaque.
Use `+com_frame_limit 100` for bounded runs; engine screenshots appear under `screenshots/`.

Regression tests cover texture extension lookup and exact-file precedence, SLK replacement sentinels,
ROC/TFT loading-row schemas, rotated/equal-height/diagonal cliff edges, unused FDF art, lazy texture cache hits/misses and theme changes,
and native loading-sprite serialization, client progress binding, and separate model/image namespaces.
Campaign artwork needs ROC/TFT runtime verification against real archives. Use explicit `-roc` and `-tft`
when a saved `fs_expansion` setting might override the default.

See also [UI authoring](../../ui-authoring.md), [scene workflow](../../rendering-scene-workflow.md),
and [filesystem loading](../../fs-loading-architecture.md).

The `wc3_loading.initial_layout_resolves_campaign_custom_and_melee_art` test uses `tests.mpq`'s native
`Loading.fdf`, skin keys and `WorldEditData.txt` rows. It calls the production loader and writer, then inspects
emitted sprites, model configstrings, title/subtitle/body, sequence and progress binding. Cases cover ROC and
TFT row schemas, custom-model precedence, and default skin decoration. Restoring the pre-fix
`hud_loading.c` makes this test fail. The client drawing tests additionally check progress 0/0.5/1 and collisions
between model and image indices.
