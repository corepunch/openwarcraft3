# Terrain And World Rendering

## WDT And ADT

The WoW renderer starts from a WDT path and tracks a 64x64 tile grid. Each present tile resolves to an ADT file:

```text
World/Maps/<MapName>/<MapName>_<tile_x>_<tile_y>.adt
```

Important local constants:

| Constant | Value | Meaning |
| --- | --- | --- |
| `WOW_WDT_TILES` | `64` | WDT tile grid size per axis. |
| `WOW_ADT_SIZE` | `533.333313f` | World units per ADT tile. |
| `WOW_ADT_CHUNK_SIZE` | `WOW_ADT_SIZE / 16` | World units per ADT chunk. |
| `WOW_ADT_UNIT_SIZE` | `WOW_ADT_CHUNK_SIZE / 8` | Fine height-grid unit. |
| `WOW_MCVT_COUNT` | `9 * 9 + 8 * 8` | Height samples per ADT chunk. |

The current renderer loads a small ADT window around the active area and keeps an alpha atlas for terrain splat masks.

## ADT Chunk Tags

The code compares chunk tags in reversed byte order. Important ADT tags currently handled:

| Tag In Code | Normal Tag | Purpose |
| --- | --- | --- |
| `XETM` | `MTEX` | Texture filename block. |
| `XDMM` | `MMDX` | Doodad model filename block. |
| `DIMM` | `MMID` | Doodad model filename offsets. |
| `FDDM` | `MDDF` | Doodad placement definitions. |
| `OMWM` | `MWMO` | WMO filename block. |
| `DIWM` | `MWID` | WMO filename offsets. |
| `FDOM` | `MODF` | WMO placement definitions. |
| `KNCM` | `MCNK` | Terrain chunk. |
| `TVCM` | `MCVT` | Terrain heights. |
| `RNCM` | `MCNR` | Terrain normals. |
| `YLCM` | `MCLY` | Texture layers. |
| `LACM` | `MCAL` | Alpha maps. |

## Terrain Layers

Each ADT chunk can carry up to four texture layers. The renderer stores:

- up to four texture handles,
- a per-chunk alpha atlas coordinate,
- decoded alpha maps,
- chunk position,
- `WOW_MCVT_COUNT` heights,
- optional normals,
- bounds for culling.

The splat path has a small Z bias and height-delta guard to keep layer geometry close to terrain without exploding across sharp height changes.

## Height Queries

`games/world-of-warcraft/common/world_wow.c` keeps a one-ADT height cache for collision/spawn queries. It loads `MCVT` height samples from `MCNK` chunks and resolves point height by splitting a local cell around the center sample into triangles, then using barycentric interpolation.

This is intentionally narrow: it is enough to ground actors on loaded terrain while the renderer and game scaffolding evolve.

## Doodads And WMOs

ADT object references are renderer-owned today:

- `MDDF` entries produce doodad instances backed by M2 models.
- `MODF` entries produce WMO instances.
- Doodads are bucketed for draw-distance culling.
- Missing doodad/WMO models are counted and can be represented by debug marker geometry when debug flags are enabled.

Game entities are not spawned for every ADT doodad. `games/world-of-warcraft/game/g_wow.c` logs that static ADT doodads are renderer-owned and not synchronized as entities.

## Current Limits

- Terrain rendering is the core focus.
- WMO, doodad, lighting, particles, water, and animation fidelity are incomplete.
- The draw window and asset compatibility are tuned around local classic-era data.
- Production support for arbitrary WoW client versions is not present.

