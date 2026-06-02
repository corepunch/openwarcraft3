# Runtime Modules and Cvars

OpenWarcraft3 uses Quake-style runtime configuration: small cvars select subsystems, choose startup modes, and make diagnostics reproducible from the command line.

Project-private compile-time macros, generated binding helpers, environment toggles, and namespaced constants use the `BZ_` prefix. Keep new project-prefixed names on that prefix instead of adding another project namespace.

## Config Files

Config files live under `share/`, which is the engine-owned default resource directory.

Load order:

1. Built-in cvar defaults from `Cvar_Init`
2. `share/default.cfg`
3. `share/config.cfg`
4. `share/autoexec.cfg`
5. Command-line cvars and commands

`share/default.cfg` is committed and stores app defaults. `share/config.cfg` is generated and stores archived user cvars. `share/autoexec.cfg` is optional local configuration.

Command-line forms:

```bash
build/bin/openwarcraft3 -data data/Warcraft\ III +r_module stdout
build/bin/openwarcraft3 -data data/Warcraft\ III +menu_main
```

## Core Cvars

| cvar | Default | Description |
|------|---------|-------------|
| `config` | `share/config.cfg` | Generated config path |
| `data` | empty | Saved Warcraft III data directory |
| `map` | empty | Internal MPQ map path for listen-server mode |
| `connect` | empty | Remote server address |
| `r_module` | `renderer` | Renderer backend name |
| `ui_module` | `ui` | UI module name |
| `g_module` | `game` | Game module name |
| `com_frame_limit` | `0` | Exit after N frames; `0` means run forever |

## Module Boundary

The runtime libraries are built into `build/lib/`:

- `libshared` — math and shared primitives
- `libjass` — Warcraft III JASS VM from `games/warcraft-3/jass/`
- `libsheet` — Warcraft III SLK/profile parser from `games/warcraft-3/sheet/`
- `librenderer` — generic renderer sources from `renderer/` plus the selected game's renderer hooks
- `libui` — selected-game UI library; for Warcraft III this is `games/warcraft-3/ui/`
- `libgame` — selected-game server-side game logic; for Warcraft III this is `games/warcraft-3/game/`

Game-owned sources live under `games/<game>/`:

| Game | Game logic | Renderer hooks | UI | Other game-owned sources |
|------|------------|----------------|----|--------------------------|
| Warcraft III | `games/warcraft-3/game/` | `games/warcraft-3/renderer/` | `games/warcraft-3/ui/` | `jass/`, `sheet/`, `tests/` |
| World of Warcraft | `games/world-of-warcraft/game/` | `games/world-of-warcraft/renderer/` | `games/world-of-warcraft/ui/` | none today |
| StarCraft II | `games/starcraft-2/game/` | `games/starcraft-2/renderer/` | uses the default UI library today | none today |

The project follows the Quake 2/Quake 3 module style: modules communicate through import/export function tables, not by reaching directly into each other's internals. The renderer exposes `R_GetAPI`; the UI exposes `UI_GetAPI`; the game module has a server/game API boundary.

The renderer has one extra internal boundary: engine code in `renderer/` calls the `R_Game*` functions declared in `renderer/r_game.h`. Those hooks are implemented by the selected `games/<game>/renderer/` tree. This keeps common renderer code from switching on game-specific model formats such as MDX, M2, or M3.

The cvars `r_module`, `ui_module`, and `g_module` are the runtime names for those modules. At present, `r_module` selects between:

- `renderer` — the normal OpenGL renderer
- `stdout` or `text` — the diagnostic text renderer

`ui_module` and `g_module` currently document the configured module names and keep the config shape ready for fully dynamic library selection.

## Stdout Renderer

The stdout renderer implements the renderer API without opening an SDL/OpenGL window. It records renderer calls as text, which makes UI layout and draw order inspectable in scripts and CI.

Recommended command:

```bash
make run-ui-text
```

Equivalent explicit command:

```bash
build/bin/openwarcraft3 \
  -data data/Warcraft\ III \
  +r_module stdout \
  +menu_main \
  +com_frame_limit 1
```

Important flags:

- `+r_module stdout` selects the text renderer.
- Menu-only checks do not enter the LAN browser, connect, or host a lobby, so UDP sockets are never opened.
- `+menu_main` chooses the UI starting command.
- `+com_frame_limit 1` exits after one frame.

One-frame runs with `com_frame_limit > 0` do not write `share/config.cfg`, so diagnostics do not change the next normal launch.

Example output:

```text
renderer_init backend="stdout" window={w:1024,h:768}
begin_frame index=1
draw_portrait model="UI\\Glues\\MainMenu\\MainMenu3d\\MainMenu3d.mdl" anim="Stand" viewport={x:0.000000,y:0.000000,w:1.000000,h:1.000000}
draw_sprite model="UI\\Glues\\MainMenu\\WarCraftIIILogo\\WarCraftIIILogo.mdl" anim="Stand" x=0.130000 y=0.080000
draw_image texture=39 name="UI\\Widgets\\Glues\\GlueScreen-Button1-Border.blp" screen={x:0.529000,y:0.110625,w:0.256000,h:0.064000} uv={x:0.000000,y:0.000000,w:1.000000,h:1.000000}
draw_text font_size=13 font="Fonts\\FRIZQT__.TTF" rect={x:0.594000,y:0.127125,w:0.179000,h:0.031000} text="|CffffffffS|Ringle Player"
end_frame index=1
renderer_shutdown backend="stdout"
```

## Use Cases

Use the stdout renderer when investigating UI issues that are hard to see from screenshots:

- missing frames or wrong command startup
- button/backdrop placement and sizes
- texture and model load paths
- UVs, tiling, rotation, colors, and blend modes
- translated strings and Warcraft color codes
- layout regressions in automated checks

Screenshots are still useful for final visual review, but stdout rendering gives a fast first answer to "what did the UI ask the renderer to draw?"
