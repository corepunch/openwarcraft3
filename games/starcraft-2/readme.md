# StarCraft II

This is an alternate game target for StarCraft II data experiments. It exists mainly to keep the engine honest across more than one Blizzard RTS asset family and to exercise the M3 renderer path behind the same selected-game module boundary.

The code here owns a small game module and the StarCraft II M3 renderer hooks.

## Status

Prototype asset-rendering target.

`opensc2` builds, links, and provides a clean place for M3 work. It is not a playable StarCraft II implementation, and it does not yet have a StarCraft II UI or gameplay layer. The current value is technical: model format coverage, renderer integration, and a second RTS-shaped game module.

## Working

- Separate `opensc2` executable and game/renderer libraries.
- Selected-game module integration for StarCraft II via the shared engine build.
- M3 model loader entry point for StarCraft II model data.
- M3 material, reference table, sequence, keyframe interpolation, and shader scaffolding.
- Basic skeletal/skinned model render path through the compound renderer.
- Minimal game module that can initialize, load map collision through the shared map interface, and provide required game exports.
- Build integration through `make opensc2`.

## Partial

- M3 support is actively shaped around the renderer path and is not full StarCraft II asset parity.
- The game module is intentionally minimal and mostly acts as a host for map/model experiments.
- UI currently falls back to the default build shape; there is no StarCraft II-specific UI library.
- Map/world behavior is placeholder-level compared with the Warcraft III target.

## Not There Yet

- Playable StarCraft II gameplay.
- StarCraft II data table, trigger, ability, race, or campaign systems.
- Full SC2 map format support.
- Complete M3 material, animation, particle, attachment, and lighting fidelity.
- StarCraft II menus, HUD, editor-like behavior, or multiplayer flow.

## Build And Run

Build:

```bash
make opensc2
```

There is no polished run target yet. Direct runs should be treated as development experiments against locally supplied data and explicit map/model paths.

## Notes

This target expects locally supplied StarCraft II data for real asset experiments. Original assets, names, and game data belong to Blizzard Entertainment. The directory is here so the engine can grow beyond one asset format without pretending the SC2 game is already built.

Map-format research notes live in [docs/readme.md](docs/readme.md).
