# References

## Local Source

- `games/world-of-warcraft/readme.md`: target status and build/run commands.
- `games/world-of-warcraft/common/world_wow.c`: WDT/ADT path setup, terrain height cache, DBC map/safe-location helpers.
- `games/world-of-warcraft/renderer/wow/r_wowmap*.c`: ADT terrain, alpha maps, splats, doodads, WMOs, draw state.
- `games/world-of-warcraft/renderer/m2/r_m2.c`: M2 loader, skin sections, animation evaluation, character composite textures.
- `games/world-of-warcraft/game/g_model.c`: M2 animation metadata parsing for game-side movement.
- `games/world-of-warcraft/game/g_wow.c`: WoW game scaffolding, ambient creatures, packed appearance/equipment use.
- `tools/m2tool.c`: M2 inspection, DBC-backed player configuration, component texture path resolution.
- `tools/README.md`: `m2tool` and WoW install examples.
- `common/shared.h`: `Wow_PackAppearance`, `Wow_UnpackAppearance`, `Wow_PackEquipment`, and `Wow_UnpackEquipment`.

## Public References

- TrinityCore `ItemDisplayInfo.dbc`: https://trinitycore.info/files/DBC/335/itemdisplayinfo
- WoTLK Modding Wiki `ItemDisplayInfo`: https://wotlkdev.github.io/wiki/dbc/ItemDisplayInfo
- getMaNGOS TBC `ItemDisplayInfo`: https://www.getmangos.eu/wiki/referenceinfo/dbcfiles/mangosonedbc/ItemDisplayInfo-r7649/
- `wow_dbc` parser crate notes: https://github.com/gtker/wow_dbc

## Caution

WoW file schemas vary by client era. Validate every DBC field offset, M2 header/view layout, and ADT chunk assumption against the data version being inspected.

