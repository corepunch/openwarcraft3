# References

Collected on 2026-06-05.

## Core Format References

- SC2Mapster Wiki, `File Formats`: https://sc2mapster.wiki.gg/wiki/File_Formats
	- Notes that `SC2Map` and `SC2Mod` are MPQ archives containing map/mod data.
- SC2Mapster Wiki, `File Formats/Maps`: https://sc2mapster.wiki.gg/wiki/File_Formats/Maps
	- Index of embedded map files including `MapInfo`, `Objects`, `Terrain`, `t3CellFlags`, `t3HeightMap`, `t3SyncPathingInfo`, `t3Terrain.xml`, `t3TextureMasks`, and others.
- SC2Mapster Wiki, `File Formats/Maps/MapInfo`: https://sc2mapster.wiki.gg/wiki/File_Formats/Maps/MapInfo
	- C-like `MapInfo` struct beginning with magic `MapI`, dimensions, bounds, theme/planet strings, loading image fields, and player-slot data.
- SC2Mapster Wiki, `File Formats/Maps/t3CellFlags`: https://sc2mapster.wiki.gg/wiki/File_Formats/Maps/t3CellFlags
	- Documents rough `LFCT` header, version `101`, dimensions, and one-byte cell types.
- Zezula, `MPQ Archives - MPQ file format`: https://www.zezula.net/en/mpq/mpqformat.html
	- Canonical public MPQ format reference. Notes MPQ user data is common in StarCraft II custom maps.
- Zezula, `Storm.dll`: https://www.zezula.net/en/mpq/stormdll.html
	- Notes Blizzard maps, including StarCraft II maps, are MPQ archives.
- StormLib: https://www.zezula.net/en/mpq/stormlib.html
	- Open-source MPQ library useful for opening/listing/extracting archives.

## Map Editing And Component Folders

- SC2Mapster Community GitHub, `Getting Started - Working with SC2`: https://sc2mapster.github.io/mkdocs/setup/
	- Describes `.SC2Map` and `.SC2Components` as archived versus unarchived project forms.
- SC2Mapster Community GitHub: https://sc2mapster.github.io/
	- Plain-text data format guides and open development maps.
- StarCraft II Editor Tutorials, `Data Spaces`: https://s2editor-guides.readthedocs.io/New_Tutorials/04_Data_Editor/data-spaces/
	- Describes map/mod data spaces, `Base.SC2Data/GameData`, catalog paths in `GameData.xml`, and locale-specific data folders.
- CurseForge, `Tutorial - How to get started`: https://www.curseforge.com/sc2/maps/tutorial-how-get-started
	- Old beta-era modding tutorial. Describes map files as MPQ contents and summarizes XML loading/override behavior.
- Hive Workshop, `My SC2 Map (heroes)`: https://www.hiveworkshop.com/threads/my-sc2-map-heroes.163156/
	- Old thread with practical references to `t3Terrain.xml`, `Objects`, `GameStrings.txt`, `MapInfo`, and MPQ editors.

## Existing Parsers And Analysis Tools

- `sc2reader` GitHub: https://github.com/ggtracker/sc2reader
	- Python library for SC2 replays, maps, and game summaries. Supports `load_map` and replay-associated map loading.
- `sc2reader` docs: https://sc2reader.readthedocs.io/
	- Says map parsing extracts minimap/icon images, localized metadata, dimensions, camera bounds, tileset, player slots, and team data.
- `sc2reader` support objects: https://sc2reader.readthedocs.io/en/latest/supportobjects.html
	- Documents `MapInfo` and `MapInfoPlayer` support structures.
- CurseForge, `SC2 Map Analyzer`: https://www.curseforge.com/sc2/assets/sc2-map-analyzer
	- GPL map-analysis tool. Discusses pathing approximations, `t3CellFlag`, `t3SyncPathingInfo`, cliff levels, resource/base analysis, and shortest paths.
- CurseForge, `SC2MapLib`: https://www.curseforge.com/sc2/assets/sc2maplib
	- XML loading/schema project for StarCraft II map data components.

## General File Summaries

- FileInfo, `.SC2MAP File Extension`: https://fileinfo.com/extension/sc2map
	- General description of SC2 maps containing name, author, description, size, terrain, units, triggers, pathing settings, and player starting points.

## Caution

Many sources are old, community-maintained, or beta-era. Validate every binary layout against current maps before depending on it in code.

