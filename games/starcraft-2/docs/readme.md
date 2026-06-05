# StarCraft II Map Documentation

These notes collect public reverse-engineering and modding references for how StarCraft II maps are stored, opened, and partially interpreted.

They are not Blizzard documentation and they are not a complete implementation spec. Treat them as a map for future loader work: enough to know which files to inspect, which community tools already parse useful pieces, and which areas are still uncertain.

## Documents

- [Map Storage And Loading](map-storage-and-loading.md): container format, component folders, dependency/XML loading behavior, and cache/download context.
- [Embedded Map Files](embedded-map-files.md): known files inside `.SC2Map` archives and what each appears to represent.
- [Parser Notes](parser-notes.md): practical loading order and implementation guidance for an OpenWarcraft3-style parser.
- [References](references.md): public sources and tools used to assemble these notes.

## Short Version

`.SC2Map`, `.SC2Mod`, `.SC2Archive`, and many `.s2ma` files are MPQ archives. A map archive contains metadata, terrain/pathing binary layers, placed-object data, minimap/loading assets, localized strings, trigger/Galaxy code, and map-local game-data XML. The Galaxy Editor can also save a map as an `.SC2Components` folder, which is the same project content unpacked into a directory.

For implementation work, start with MPQ archive support, then parse `MapInfo`, localized strings, `Objects`, `t3Terrain.xml`, terrain/pathing `t3*` layers, and `Base.SC2Data/GameData` XML catalogs.

