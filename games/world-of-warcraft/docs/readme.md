# World of Warcraft Documentation

These notes collect what the World of Warcraft target currently knows how to load and render. The target is an exploratory renderer/world-loading path, not a full MMO implementation.

## Documents

- [Data Loading](data-loading.md): MPQ data layout, WDT/ADT map entry, DBC helpers, and tool commands.
- [File Formats](file-formats.md): collected reverse-engineered format notes and source map for MPQ/CASC, WDT/ADT/WDL, WMO, M2/SKIN/ANIM, BLP, DBC/DB2, WDB, and related files.
- [Terrain And World Rendering](terrain-and-world-rendering.md): WDT tiles, ADT chunks, splats, alpha maps, doodads, WMOs, and height queries.
- [M2 And Character Display](m2-and-character-display.md): M2 loading, skin files, DBC-backed outfit data, geosets, and component texture rules.
- [References](references.md): public schema references and local source/tool entry points.

## Short Version

`openwow` mounts locally supplied WoW client data, opens a WDT path such as `World/Maps/Azeroth/Azeroth.wdt`, loads nearby ADT tiles, renders terrain chunks and splat layers, and uses M2 models for creatures/player-style actors. Character appearance is data-driven by packed appearance/equipment values, DBC records, M2 skin section IDs, and composed body textures.

The important rule for future work: keep WoW-specific policy under `games/world-of-warcraft/`. Engine modules should stay format-agnostic and receive generic renderer/game data through the selected-game boundary.
