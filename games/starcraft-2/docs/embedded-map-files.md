# Embedded Map Files

This file summarizes known embedded files listed by SC2Mapster's `File Formats/Maps` index and nearby community notes. Many details are incomplete or based on beta-era reverse engineering, so preserve unknown fields and parse conservatively.

## Common Files

| File | Format | Notes |
| --- | --- | --- |
| `MapInfo` | binary | High-level map metadata: dimensions, playable/camera bounds, theme/planet strings, loading-screen path/format, and player slot records. Magic is documented as `MapI`. |
| `Objects` | XML-like/text in community examples | Placed units, doodads, resources, regions, points, and other object-layer data. Old modding threads mention removing doodads from this file. |
| `Terrain` | binary | Terrain container or terrain-related metadata. Exact public detail is sparse. |
| `PaintedPathingLayer` | binary | Painted pathing data from the editor. Exact layout still needs verification. |
| `t3Terrain.xml` | XML | Terrain texture/tile mapping data. Community examples used it to change map terrain textures. |
| `t3CellFlags` | binary | Cell flags. Documented header uses magic `LFCT`, version `101`, dimensions, then one byte per cell. |
| `t3HeightMap` | binary | Height map data. |
| `t3SyncHeightMap` | binary | Synchronized/runtime height map data. |
| `t3SyncCliffLevel` | binary | Synchronized cliff-level data. |
| `t3SyncPathingInfo` | binary | Synchronized pathing data. SC2 Map Analyzer discusses using this for more accurate pathing than `t3CellFlags`. |
| `t3HardTile` | binary | Hard tile data. |
| `t3TextureMasks` | binary | Terrain texture blend/mask data. |
| `t3VertCol` | binary | Vertex color data. |
| `t3Water` | binary | Water/lava placement or water-layer data. |
| `t3FluffDoodad` | binary | Fluff doodad placement. Community notes call this harder to edit than `Objects`. |

Other map archives may contain:

- `DocumentHeader`
- `Minimap.tga`
- loading-screen textures under `Assets/Textures/`
- trigger/Galaxy script files
- `Base.SC2Data/GameData/*.xml`
- locale data such as `enUS.SC2Data/LocalizedData/GameStrings.txt`
- imported custom models, textures, sounds, and UI assets

## `MapInfo`

SC2Mapster documents `MapInfo` as a binary struct beginning with:

```c
struct MapInfo {
	DWORD magic;      /* "MapI" */
	u32 fileVersion;
	u32 width;
	u32 height;
	u32 unknown2;
	u32 unknown3;
	char theme[];
	char planet[];
	u32 boundaryLeft;
	u32 boundaryBottom;
	u32 boundaryRight;
	u32 boundaryTop;
	...
};
```

Later fields include a loading image path, loading image stretch mode, width/height-like values, a player count, and per-player structures. Community examples identify slot owner/control values such as disabled, human, AI, and observer, but the full record layout should be verified against real maps before hardcoding.

OpenWarcraft3 parser guidance:

- Check the `MapI` magic before reading.
- Parse strings explicitly instead of relying on packed C structs.
- Treat unknown fields as named `unknown*` values and preserve them in dumps.
- Do not use runtime structs as serialized structs.

## `t3CellFlags`

SC2Mapster documents the rough layout as:

```c
struct {
	DWORD magic;      /* "LFCT" */
	UINT version;     /* current: 101 */
	DWORD zero[4];
	UINT sizeX;
	UINT sizeY;
} header;

BYTE types[header.sizeX * header.sizeY];
```

Observed byte values include:

- `0x00`: black in visualizations.
- `0x01`: red near ramps.
- `0x02`: yellow.
- `0x03`: green/cliff holes.

The page says this file does not appear to drive normal pathing directly, but `0x03` removes world terrain so cliff models can fit without terrain poking through. For pathing, prefer `t3SyncPathingInfo` when it is understood; use `t3CellFlags` only as a coarse visualization/fallback.

## XML Catalogs

Map-local XML can live in `Base.SC2Data/GameData/` and related data folders. Important catalog families include unit, ability, terrain, water, texture, behavior, effect, actor, model, button, and requirement data. Specific catalog filenames vary by project.

Implementation guidance:

- Load XML with a real XML parser or a small purpose-built catalog parser once the required subset is known.
- Preserve catalog paths from `GameData.xml`.
- Keep localized text separate from object/catalog IDs.
- Expect dependency resolution to matter.

## Localization

Display strings are commonly stored in locale-specific folders such as:

```text
enUS.SC2Data/LocalizedData/GameStrings.txt
```

The map title, author, description, website, and other UI strings may be resolved through these files. Parser tests should avoid assuming a single locale exists.

## Terrain And Pathing Work Items

The following files still need direct fixture-based reverse engineering before engine use:

- `t3HeightMap`
- `t3SyncHeightMap`
- `t3SyncCliffLevel`
- `t3SyncPathingInfo`
- `t3TextureMasks`
- `t3Water`
- `t3FluffDoodad`
- `PaintedPathingLayer`

SC2 Map Analyzer is the strongest public proof that these files are usable for map analysis, but its documentation is mostly tool behavior and screenshots rather than a complete binary spec.

