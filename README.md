<p align="center">
  <img width="480" height="265" alt="OpenWarcraft3 logo" src="doc/images/logo.png" />
</p>

**OpenWarcraft3** is an open-source implementation of Warcraft III that uses SDL2 and runs on Windows, Linux, and macOS.

It was developed using War3.mpq from Warcraft III v1.0 as reference, with ongoing support for version 1.29b.

<a href="https://corepunch.github.io/openwarcraft3/">📖 Documentation</a> · <a href="https://youtu.be/vg7Jm046vcI">▶ Watch the demo on YouTube</a> · see screenshots below

## Download

Pre-built binaries for Linux and macOS are available on the [Releases page](https://github.com/corepunch/openwarcraft3/releases/latest).

You can also download the latest build artifact from the [CI workflow runs](https://github.com/corepunch/openwarcraft3/actions/workflows/c-cpp.yml) (click the most recent successful run and download `openwarcraft3-linux-x64`).

<p align="center">
  <img src="doc/images/screenshot1.jpg" width="31%" style="margin-right:2%;" />
  <img src="doc/images/screenshot2.jpg" width="31%" style="margin-right:2%;" />
  <img src="doc/images/screenshot3.jpg" width="31%" />
</p>

## Getting Started

### 1. Clone

```bash
git clone git@github.com:corepunch/openwarcraft3.git
cd openwarcraft3
```

### 2. Install Dependencies

The build requires **SDL2**. MPQ reading is handled by the in-tree `common/mpq.c` implementation, and JPEG texture decoding uses the in-tree `renderer/stb/stb_image.h`.

**macOS** (via [Homebrew](https://brew.sh/)):

```bash
brew install sdl2
```

**Linux** (Ubuntu/Debian):

```bash
sudo apt-get install libsdl2-dev
```

**Windows**:

Install SDL2 development libraries and build with a C compiler such as MSYS2/MinGW or Visual Studio. The codebase uses platform-specific directory iteration for Windows, Linux, and macOS when mounting data folders.

### 3. Build

```bash
make build
```

Compiles all runtime libraries (`shared`, `renderer`, `game`, `ui`) and the `openwarcraft3` executable into `build/`.

Viewer tools are also built into `build/bin/`:
- `mdxtool` — model viewer
- `maptool` — map viewer
- `mpqtool` — archive inspection (`ls`, `cat`, `pack`)
- `blp2jpg`, `blpgen`, `mdxgen` — asset conversion/generation helpers

### 4. Run

```bash
make run
```

Runs `openwarcraft3` from `build/bin/` using the data folder configured in the Makefile.

The executable expects a Warcraft III data folder rather than a single archive:

```bash
build/bin/openwarcraft3 -data "data/Warcraft III"
```

The data folder is scanned for top-level `.mpq` archives and an optional loose `Maps/` directory. This lets newer installs expose multiplayer maps from the filesystem while older assets can still be loaded from MPQs.

To start a game from the normal client menu, click **Single Player**, then **Campaign**, then choose a campaign race. The current Single Player flow launches the first available campaign map for the selected race:

- Tutorial: `Maps\Campaign\Prologue01.w3m`
- Human: `Maps\Campaign\Human02.w3m`
- Orc: `Maps\Campaign\Orc01.w3m`
- Undead: `Maps\Campaign\Undead01.w3m`
- Night Elf: `Maps\Campaign\NightElf01.w3m`

These are MPQ-internal paths, so campaign maps may not appear in the loose `data/Warcraft III/Maps/` directory even when they are present in `War3.mpq`. You can inspect them with `build/bin/mpqtool -mpq "data/Warcraft III/War3.mpq" ls Maps/Campaign`.

Useful run targets:

- `make run` — start the client menu using `WC3DATA`
- `make run-map` — start a listen-server game using `MAP`
- `make run-ui-text` — render one UI frame through the stdout renderer

The stdout UI renderer is meant for layout and draw-call debugging without opening a window or taking screenshots:

```bash
make run-ui-text
```

That expands to:

```bash
build/bin/openwarcraft3 -data data/Warcraft\ III +net_enabled 0 +r_module stdout +com_frame_limit 1 +menu_main
```

It prints calls such as `draw_portrait`, `draw_sprite`, `draw_image`, `draw_text`, and `draw_sys_text`, then exits after one frame.

### In-game console

OpenWarcraft3 includes a Quake-style console for diagnostics and runtime commands. Press backtick/tilde to open or close it, press Enter to run a command, use Up/Down for command history, and press Tab to complete command and cvar names.

The console accepts the same commands and cvars used by the command line. For example, `map "Maps\Campaign\Orc01.w3m"` starts a listen-server game directly, while `menu_main`, `menu_credits`, and `menu_options` switch client-side menu screens.

### Configuration and cvars

OpenWarcraft3 uses Quake-style cvars and config files. Defaults live in `share/default.cfg`; generated user settings are written to `share/config.cfg`; optional local overrides can be placed in `share/autoexec.cfg`.

Config load order:

1. Built-in cvar defaults
2. `share/default.cfg`
3. `share/config.cfg`
4. `share/autoexec.cfg`
5. Command-line launch args such as `-data data/Warcraft\ III`, command-line cvars such as `+r_module stdout` or `+set net_enabled 0`, and queued commands such as `+map Maps\\Campaign\\Orc01.w3m` or `+menu_main`

Common runtime cvars:

| cvar | Default | Purpose |
|------|---------|---------|
| `data` | `""` | Saved Warcraft III data folder |
| `map` | `""` | Internal map path for listen-server mode |
| `connect` | `""` | Remote server address |
| `r_module` | `"renderer"` | Renderer backend: `renderer` or `stdout` |
| `ui_module` | `"ui"` | UI module name for the Quake-style module boundary |
| `g_module` | `"game"` | Game module name for the server game boundary |
| `net_enabled` | `"1"` | Disable with `0` for isolated UI/render diagnostics |
| `com_frame_limit` | `"0"` | Exit after N frames; useful with `r_module=stdout` |

### (Optional) Download Warcraft III 1.29b assets

```bash
make download
```

Downloads a ~1.2 GB installer from `archive.org` into the `data/` folder. Skip this step if you already have a Warcraft III installation and update the `WC3DATA` variable in the Makefile to point to that data folder.

---

# Architecture

## Client-Server Architecture

OpenWarcraft3 uses a strict client-server separation where all game logic runs exclusively on the server and clients are responsible only for rendering and input.

The **server** hosts the game library (`game/`), which is built as a runtime module with a Quake-style function table boundary. It maintains the authoritative game state: all entities, their positions, health, current animations, and AI state. The server processes player commands, runs the game simulation each frame, and sends the resulting state to clients.

The **client** (`client/`) captures user input via SDL2, forwards commands to the server, receives the updated game state, and renders it using the renderer API (`renderer/`). The client never runs game logic directly — it is purely a display and input layer.

Communication between the client and server happens through the network layer (`common/net.c`), which follows the Quake 2 runtime-dispatch model.  The routing decision is made at runtime on `netadr_t.type`:

- **Loopback** (`NA_LOOPBACK`) — when both server and client run in the same process with `+map`, two 256 KiB ring buffers carry traffic in each direction with zero latency and no socket overhead.
- **UDP** (`NA_IP`) — when the executable is started with `-connect <host>`, it binds a non-blocking UDP socket and communicates with a remote listen server over the network.

```
# Listen server (loopback, single process)
SDL2 Input  →  Client (cl_main.c)  →  net.c ring buffer  →  Server (sv_main.c)
                    ↑                                               |
                    └──────────── net.c ring buffer ←──────────────┘

# Remote client (UDP)
SDL2 Input  →  Client (cl_main.c)  →  UDP socket  →  Server (sv_main.c)
                    ↑                                      |
                    └─────────── UDP socket ←──────────────┘
```

See [Network Architecture](doc/architecture/network.md) for the full design, wire format, and CLI reference. See [Runtime Modules and Cvars](doc/architecture/runtime.md) for config files, module cvars, and stdout renderer diagnostics.

## Frame Syncing

The server advances the game in fixed-size time steps. Every frame the server:

1. Reads any pending client commands (`SV_ReadPackets` in `sv_main.c`)
2. Advances the game clock by `FRAMETIME` (100 ms) and calls `ge->RunFrame()` in `g_main.c`
3. Builds and sends a snapshot of the current game state to each client (`SV_SendClientMessages`)

```c
// server/sv_main.c
void SV_Frame(DWORD msec) {
    svs.realtime += msec;
    if (svs.realtime < sv.time)
        return;  // not yet time for a new frame
    SV_ReadPackets();
    SV_RunGameFrame();        // increments sv.framenum, sv.time, calls ge->RunFrame()
    SV_SendClientMessages();  // sends entity state to each connected client
}
```

The client runs at its own rate and simply applies each server snapshot as it arrives. On every client frame (`CL_Frame` in `cl_main.c`) the client reads incoming server messages, processes input, sends commands, and renders the current known state:

```c
// client/cl_main.c
void CL_Frame(DWORD msec) {
    cl.time += msec;
    CL_ReadPackets();   // apply server snapshots
    CL_Input();         // sample user input
    CL_SendCommand();   // forward commands to server
    CL_PrepRefresh();   // build render scene
    SCR_UpdateScreen(); // draw the frame
}
```

### Entity Synchronization and Delta Compression

Each server frame, `SV_BuildClientFrame` (`server/sv_ents.c`) collects the entities visible to a client into a snapshot. `SV_WriteFrameToClient` then compares the new snapshot against the previous one and writes only the fields that changed using `MSG_WriteDeltaEntity`. This delta compression keeps message sizes small even when many entities exist.

The client receives these delta messages in `CL_ReadPacketEntities` (`client/cl_parse.c`) and applies them to its local entity table, keeping `prev` and `current` state for interpolation.

## Projectile System

Ranged attacks spawn a projectile entity (`fire_rocket` in `game/skills/s_attack.c`). The projectile stores a reference to its target entity, a launch speed, a damage value, and a model index for rendering. Its `movetype` is set to `MOVETYPE_FLYMISSILE`.

Each game frame, `G_RunEntity` (`game/g_phys.c`) dispatches on `movetype`. For `MOVETYPE_FLYMISSILE`, `SV_Physics_Toss` moves the projectile straight toward the target's current position at a fixed speed. When the remaining distance is less than the distance traveled this frame, the projectile calls `T_Damage` on the target and frees itself:

```c
// game/g_phys.c
void SV_Physics_Toss(LPEDICT ent) {
    FLOAT distance = ent->velocity * FRAMETIME;
    VECTOR3 dir = Vector3_sub(&ent->goalentity->s.origin, &ent->s.origin);
    if (Vector3_len(&dir) < distance) {
        T_Damage(ent->goalentity, ent->owner, ent->damage);
        G_FreeEdict(ent);
    } else {
        Vector3_normalize(&dir);
        G_PushEntity3(ent, distance, &dir);
    }
}
```

`T_Damage` (`game/skills/s_attack.c`) reduces the target's health. If the target survives and is capable of attacking back, it automatically issues a counter-attack order. If the target's health reaches zero, its `die` callback is invoked.

Because projectiles are regular server entities with a model, they are automatically included in the entity snapshot and rendered on the client without any special handling.

## Unit Movement

When a player right-clicks on the ground, the client sends a command to the server. The server's command handler (`game/g_commands.c`) resolves the selected units and calls `move_selectlocation` (`game/skills/s_move.c`), which:

1. Allocates a **waypoint** entity at the target map position, snapping its Z coordinate to the terrain height (`Waypoint_add` in `game/g_monster.c`)
2. Calls `order_move` on each selected unit, setting the unit's `goalentity` to the waypoint and switching its current move to the walk state

Each server frame, units in the walk state execute `ai_walk` (`game/skills/s_move.c`):

```c
static void ai_walk(LPEDICT ent) {
    if (M_DistanceToGoal(ent) <= unit_movedistance(ent)) {
        ent->stand(ent);        // arrived — switch to idle
    } else {
        unit_changeangle(ent);  // rotate toward goal
        unit_moveindirection(ent); // advance one frame's worth
    }
}
```

After all entities have moved, `G_SolveCollisions` (`game/g_phys.c`) iterates over every pair of overlapping entities and separates them. When two moving units collide, the separation is split proportionally based on each unit's remaining distance to its goal — the unit that is closer to its destination yields more, preventing deadlocks at the destination.

Animation is driven by `M_MoveFrame` (`game/g_monster.c`), which advances `edict->s.frame` each game tick according to the current animation interval. When an animation cycle completes, the `endfunc` of the current `umove_t` is called to transition to the next state (e.g. attack → cooldown → attack again).

## UI System (Phase 8: Client-Side Architecture)

All UI logic runs **client-side** in the UI library (`ui/`). The server provides only game data (unit abilities, inventory, build queues) through a query protocol. This follows the Quake 3 Arena pattern where UI is a separate client-side library.

### Migration from Server-Side UI

Previously (Phase 1-7), UI logic ran on the server in `game/ui/` and `game/hud/`. This violated the game-agnostic principle and bloated the game library by ~107KB. Phase 8 (May 2026) moved all UI to the client, reducing `libgame.dylib` from 406K to 299K.

### FDF Parsing and Frame Management

The UI library (`ui/ui_main.c`) loads Warcraft III's `.fdf` (Frame Definition File) assets via `UI_ParseFDF` (`ui/ui_fdf.c`). These files describe the hierarchy of UI frames — their type (backdrop, button, label, etc.), textures, fonts, anchor points, and sizes. The UI library maintains the complete frame tree and handles all layout calculation and rendering client-side.

### Unit Data Query Protocol

When the player selects units, the client requests unit data from the server:

**Client → Server:** `clc_request_unit_ui`
```c
// client/cl_input.c — Selection complete
MSG_WriteByte(&cls.netchan.message, clc_request_unit_ui);
MSG_WriteByte(&cls.netchan.message, num_selected);
for (i = 0; i < num_selected; i++)
    MSG_WriteShort(&cls.netchan.message, entity_nums[i]);
```

**Server → Client:** `svc_unit_ui`
```c
// server/sv_unit_ui.c — Query game DLL and respond
gameCommandButton_t buttons[12];
BYTE num_buttons = ge->GetCommandButtons(ent, buttons, 12);

MSG_WriteByte(response, num_buttons);
for (j = 0; j < num_buttons; j++) {
    MSG_WriteString(response, buttons[j].art);
    MSG_WriteString(response, buttons[j].tooltip);
    MSG_WriteString(response, buttons[j].ubertip);
    MSG_WriteString(response, buttons[j].command);
    MSG_WriteByte(response, buttons[j].hotkey);
}
// ... repeat for inventory and build queue
```

**Client Storage:**
```c
// ui/screens/console_ui.c — Cache and render
static uiUnitData_t cached_units[MAX_CACHED_UNITS];
void ConsoleUI_UpdateUnitUI(DWORD num_units, uiUnitData_t *units) {
    memcpy(cached_units, units, sizeof(uiUnitData_t) * num_units);
    // Rendering uses cached_units[] to draw command card
}
```

### Client-Side Rendering

The UI library dispatches rendering to screen controllers (e.g., `ui/screens/console_ui.c` for in-game HUD, `ui/screens/main_menu.c` for menus). Each screen manages its own frame tree and updates frames based on game state. The renderer calls back into client import functions to draw quads, text, and models.

No serialized UI blobs are transmitted over the network. The server is game-agnostic and provides only data.

### Text Renderer Diagnostics

For UI work, use the stdout renderer before reaching for screenshots:

```bash
make run-ui-text
```

This runs the configured UI command for one frame, skips network socket binding, prints draw calls to stdout, and exits without writing `share/config.cfg`. It is useful for checking:

- which textures, models, fonts, and screens were loaded
- button/backdrop rects, UVs, colors, and blend modes
- text content after FDF string translation and Warcraft color-code expansion
- scene placement across commands such as `menu_main`, `menu_startserver`, or `menu_game`

---

## Build System

The project builds four runtime libraries and one executable:

1. **libshared** (`shared/`) — mathematics (vectors, matrices, quaternions, geometric primitives); no external dependencies
2. **librenderer** (`renderer/`) — renderer API implementations, including OpenGL and stdout diagnostics; depends on `libshared`, SDL2
3. **libgame** (`game/`) — server-side game logic; depends on `libshared`
4. **libui** (`ui/`) — client-side FDF parser, command-driven screen controller, and UI renderer
5. **openwarcraft3** — main executable linking the runtime libraries plus SDL2

The module boundary follows the Quake 2/Quake 3 style: subsystems expose function tables (`R_GetAPI`, `UI_GetAPI`, game exports/imports) rather than sharing global implementation details. The cvars `r_module`, `ui_module`, and `g_module` name the active modules. Today `r_module=stdout` selects the text renderer backend; the cvar layout is also the path toward fully dynamic library selection.

The build is driven by a `Makefile` for Linux/macOS. Run `make test` to execute the unit test suite.

### UI Test Enforcement

UI tests are fully repo-owned and deterministic:

1. Source fixtures live under `tests/resources-src/`.
2. Generated test assets are built into `build/tests/resources/`.
3. The packed archive is `build/tests/tests.mpq` (generated, never committed).

`make test` now always runs `make test-assets` first, so the generated UI archive is part of the normal test flow.

For UI-impacting changes (`ui/*`, `client/cl_scrn.c`, `renderer/r_draw.c`, sprite/model UI paths), run `make test-ui` before merging. This gate executes parser, layout, end-to-end, and tool-oracle UI suites.

Note: `fdftool` was removed in Phase 8 as it depended on deleted server-side UI code (`game/ui/`).

## External Dependencies

- **MPQ layer** (`common/mpq.c`): in-tree Warcraft III MPQ reader; no StormLib dependency at build or run time. `mpqtool` CLI exposes `ls` and `cat` for archive inspection.
- **SDL2**: windowing, input, and OpenGL context
- **stb_image**: in-tree JPEG texture decoding
- **OpenGL**: 3D rendering (system-provided)

## Current Status
- Functional game implementation with basic Warcraft III features
- Client-side main menu, Single Player menu, campaign selection, and campaign-map launching
- Quake-style runtime console with command history, cvar lookup, and completion
- Client-side Warcraft III FDF UI rendering with stdout renderer diagnostics
- Support for original v1.0 assets with ongoing work for v1.29b compatibility
- Active development focused on expanding game feature completeness
- Cross-platform support for Linux and macOS environments

## License

This project is licensed under the [MIT License](LICENSE). Contributions are welcome!
