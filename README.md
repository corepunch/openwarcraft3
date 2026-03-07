<img width="647" height="88" alt="OpenWarcraft3 logo" src="https://github.com/user-attachments/assets/67313005-70d7-4e68-97a3-3e0ff1e788ed" />

**OpenWarcraft3** is an open-source implementation of Warcraft III that uses SDL2 and runs on Linux and macOS.

It was developed using War3.mpq from Warcraft III v1.0 as reference, with ongoing support for version 1.29b.

<a href="https://youtu.be/vg7Jm046vcI">▶ Watch the demo on YouTube</a> · see screenshots below

## Download

Pre-built binaries for Linux and macOS are available on the [Releases page](https://github.com/corepunch/openwarcraft3/releases/latest).

You can also download the latest build artifact from the [CI workflow runs](https://github.com/corepunch/openwarcraft3/actions/workflows/c-cpp.yml) (click the most recent successful run and download `openwarcraft3-linux-x64`).

<p align="center">
  <img src="https://github.com/user-attachments/assets/c76a93af-1801-402e-83bc-b3e0a4462312" width="31%" style="margin-right:2%;" />
  <img src="https://github.com/corepunch/openwarcraft3/assets/83646194/643c7aa7-2b91-469c-857e-0f6910c939af" width="31%" style="margin-right:2%;" />
  <img src="https://github.com/corepunch/openwarcraft3/assets/83646194/a79e447d-e42c-4468-b4ca-3d212efe346a" width="31%" />
</p>

## Getting Started

### 1. Clone

```bash
git clone --recurse-submodules git@github.com:corepunch/openwarcraft3.git
cd openwarcraft3
```

### 2. Install Dependencies

The build requires **StormLib**, **SDL2**, and **libjpeg**.

**macOS** (via [Homebrew](https://brew.sh/)):

```bash
brew install sdl2 libjpeg stormlib
```

**Linux** (Ubuntu/Debian):

```bash
sudo apt-get install libsdl2-dev libjpeg-dev libstorm-dev
```

### 3. Build

```bash
make build
```

Compiles all libraries (`cmath3`, `renderer`, `game`) and the `openwarcraft3` executable into `build/`.

### 4. Run

```bash
make run
```

Runs `openwarcraft3` from `build/bin/` using the MPQ path configured in the Makefile.

### (Optional) Download Warcraft III 1.29b assets

```bash
make download
```

Downloads a ~1.2 GB installer from `archive.org` into the `data/` folder. Skip this step if you already have a `War3.mpq` and update the `MPQ` variable in the Makefile to point to it.

---

## Advanced: Building with Premake5

Premake5 can generate native project files for Visual Studio, Xcode, or GMake. Bundled binaries are in `tools/bin/`.

| Platform | Command |
|---|---|
| Windows (VS 2022) | `tools\bin\windows\premake5.exe vs2022` |
| macOS (Xcode) | `tools/bin/darwin/premake5 xcode4` |
| macOS (GMake) | `tools/bin/darwin/premake5 --cc=clang gmake` |
| Linux (GMake) | `premake5 --cc=clang gmake` *(install [premake5](https://premake.github.io/download) separately)* |

Project files are generated in the `build/` folder.

---

# Architecture

## Client-Server Architecture

OpenWarcraft3 uses a strict client-server separation where all game logic runs exclusively on the server and clients are responsible only for rendering and input.

The **server** hosts the game library (`src/game/`), which is a shared library loaded at runtime. It maintains the authoritative game state: all entities, their positions, health, current animations, and AI state. The server processes player commands, runs the game simulation each frame, and sends the resulting state to clients.

The **client** (`src/client/`) captures user input via SDL2, forwards commands to the server, receives the updated game state, and renders it using the renderer library (`src/renderer/`). The client never runs game logic directly — it is purely a display and input layer.

Communication between the client and server happens through the network layer (`src/common/net.c`). Currently the game runs in a single process and uses a **double-buffer loopback** for communication: two circular buffers act as one-way channels (client→server and server→client). The architecture is designed so that replacing the loopback with a real TCP/IP socket requires no changes outside `net.c`.

```
SDL2 Input  →  Client (cl_main.c)  →  net.c loopback  →  Server (sv_main.c)
                    ↑                                            |
                    └──────────── net.c loopback ←──────────────┘
                                  (game state)
```

## Frame Syncing

The server advances the game in fixed-size time steps. Every frame the server:

1. Reads any pending client commands (`SV_ReadPackets` in `sv_main.c`)
2. Advances the game clock by `FRAMETIME` (100 ms) and calls `ge->RunFrame()` in `g_main.c`
3. Builds and sends a snapshot of the current game state to each client (`SV_SendClientMessages`)

```c
// src/server/sv_main.c
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
// src/client/cl_main.c
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

Each server frame, `SV_BuildClientFrame` (`src/server/sv_ents.c`) collects the entities visible to a client into a snapshot. `SV_WriteFrameToClient` then compares the new snapshot against the previous one and writes only the fields that changed using `MSG_WriteDeltaEntity`. This delta compression keeps message sizes small even when many entities exist.

The client receives these delta messages in `CL_ReadPacketEntities` (`src/client/cl_parse.c`) and applies them to its local entity table, keeping `prev` and `current` state for interpolation.

## Projectile System

Ranged attacks spawn a projectile entity (`fire_rocket` in `src/game/skills/s_attack.c`). The projectile stores a reference to its target entity, a launch speed, a damage value, and a model index for rendering. Its `movetype` is set to `MOVETYPE_FLYMISSILE`.

Each game frame, `G_RunEntity` (`src/game/g_phys.c`) dispatches on `movetype`. For `MOVETYPE_FLYMISSILE`, `SV_Physics_Toss` moves the projectile straight toward the target's current position at a fixed speed. When the remaining distance is less than the distance traveled this frame, the projectile calls `T_Damage` on the target and frees itself:

```c
// src/game/g_phys.c
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

`T_Damage` (`src/game/skills/s_attack.c`) reduces the target's health. If the target survives and is capable of attacking back, it automatically issues a counter-attack order. If the target's health reaches zero, its `die` callback is invoked.

Because projectiles are regular server entities with a model, they are automatically included in the entity snapshot and rendered on the client without any special handling.

## Unit Movement

When a player right-clicks on the ground, the client sends a command to the server. The server's command handler (`src/game/g_commands.c`) resolves the selected units and calls `move_selectlocation` (`src/game/skills/s_move.c`), which:

1. Allocates a **waypoint** entity at the target map position, snapping its Z coordinate to the terrain height (`Waypoint_add` in `src/game/g_monster.c`)
2. Calls `order_move` on each selected unit, setting the unit's `goalentity` to the waypoint and switching its current move to the walk state

Each server frame, units in the walk state execute `ai_walk` (`src/game/skills/s_move.c`):

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

After all entities have moved, `G_SolveCollisions` (`src/game/g_phys.c`) iterates over every pair of overlapping entities and separates them. When two moving units collide, the separation is split proportionally based on each unit's remaining distance to its goal — the unit that is closer to its destination yields more, preventing deadlocks at the destination.

Animation is driven by `M_MoveFrame` (`src/game/g_monster.c`), which advances `edict->s.frame` each game tick according to the current animation interval. When an animation cycle completes, the `endfunc` of the current `umove_t` is called to transition to the next state (e.g. attack → cooldown → attack again).

## UI System

All UI logic runs on the **server** inside the game library (`src/game/ui/`). The client receives serialized UI state and renders it; it has no knowledge of UI structure or layout rules.

### FDF Parsing and Frame Templates

At startup, `UI_Init` (`src/game/ui/ui_init.c`) loads Warcraft III's `.fdf` (Frame Definition File) assets via `UI_ParseFDF`. These files describe the hierarchy of UI frames — their type (backdrop, button, label, etc.), textures, fonts, anchor points, and sizes. The parsed data is stored as `frameDef_t` templates in a global registry.

### Writing UI to Clients

When a client connects, `G_ClientBegin` (`src/game/g_main.c`) calls `UI_WriteLayout` to serialize the complete UI tree and send it to the client as an `svc_layout` message. `UI_WriteLayout` (`src/game/ui/ui_write.c`) traverses the frame tree depth-first; for each frame it calls `UI_WriteFrame`, which:

1. Copies the frame's base properties (position anchors, size, texture, color) into a `uiFrame_t` struct
2. Writes type-specific data (backdrop edges, button states, label font settings, etc.) into a small inline buffer
3. Calls `gi.WriteUIFrame` to emit the frame as a delta-encoded message

The client receives the `svc_layout` message in `CL_ParseLayout` (`src/client/cl_parse.c`) and stores the raw serialized layout blob. The renderer reads this blob each frame to draw the UI without the client needing to understand the frame hierarchy.

### Wire Message Format (`uiFrame_t`)

Every UI element is represented as a `uiFrame_t` struct (defined in `src/common/shared.h`). The server populates one of these per frame and calls `gi.WriteUIFrame` to send it. The struct is compact and self-contained — it describes where the element sits, what it looks like, what text or stat it displays, and what command it fires when clicked.

The key fields (see `uiFrameFields[]` in `src/common/msg.c`):

| Field | Description |
|---|---|
| `type` (in `flagsvalue`) | Frame type: `FT_TEXT`, `FT_BACKDROP`, `FT_COMMANDBUTTON`, `FT_PORTRAIT`, … |
| `parent` | Frame number of the parent frame (use `UI_PARENT` to anchor to the layer root) |
| `points.x[FPP_MIN/MID/MAX]` | Horizontal anchor points (left / center / right), each carrying a `relativeTo` reference frame number and a pixel `offset` |
| `points.y[FPP_MIN/MID/MAX]` | Vertical anchor points (top / middle / bottom) |
| `size.width`, `size.height` | Explicit size in normalized screen units (0–0.8 × 0–0.6) |
| `tex.index` | Texture/image index (`pic`) — resolved from the MPQ at load time |
| `tex.coord[4]` | UV sub-rectangle within the texture (min/max x and y as bytes) |
| `stat` | Player stat index to display as a live number; `0` means use `text` instead |
| `color` | RGBA tint applied to the element |
| `text` | Static string to display (label, button caption, etc.) |
| `tooltip` | Tooltip string shown on hover |
| `onclick` | Server command string sent back when the element is clicked (e.g. `"button Amov"`) |

A small type-specific buffer is appended after the base fields for things like backdrop edge textures, button state textures, or label alignment — see `UI_WriteFrame` in `src/game/ui/ui_write.c`.

Messages are **delta-encoded**: only fields that changed since the last transmission are included in the packet, so a simple text label might fit in just a handful of bytes.

A minimal server-side example that produces a "Hello, World" text element:

```c
FRAMEDEF f;
UI_InitFrame(&f, FT_TEXT);           // type = FT_TEXT
f.Text = "Hello, World";             // text field
UI_SetPoint(&f, FRAMEPOINT_CENTER,
             NULL, FRAMEPOINT_CENTER,
             0.0f, 0.0f);            // x/y centered on screen
UI_WriteFrame(&f);                   // serialise → uiFrame_t → client
```

### Dynamic UI Updates

During gameplay the server can push incremental UI updates for things like the command card (ability buttons shown for the selected unit). These updates follow the same `svc_layout` / delta-encoding path, replacing only the affected layer on the client.

### Building UI with String Definitions

The server supports two complementary ways to define and send UI frames at runtime.

**1. Inline FDF strings**

A frame hierarchy can be written as a C string literal that follows FDF syntax and registered with `UI_ParseFDF_Buffer`. This is useful for creating self-contained components without a corresponding `.fdf` file on disk:

```c
// src/game/ui/ui_init.c
static LPCSTR tooltip =
    "Frame \"FRAME\" \"ToolTip\" {\n"
    "    Frame \"TOOLTIPTEXT\" \"ToolTipText\" {\n"
    "        SetAllPoints,\n"
    "        BackdropBackground  \"ToolTipBackground\",\n"
    "        BackdropCornerFlags \"UL|UR|BL|BR|T|L|B|R\",\n"
    "        BackdropCornerSize  0.008,\n"
    "        FrameFont \"MasterFont\", 0.010, \"\",\n"
    "        FontJustificationH JUSTIFYLEFT,\n"
    "        FontColor 1.0 1.0 1.0 1.0,\n"
    "    }\n"
    "}\n";

void Init_ToolTip(LPFRAMEDEF parent) {
    LPSTR buffer = strdup(tooltip);
    UI_ParseFDF_Buffer("Tooltip", buffer);  // registers the template
    free(buffer);

    // retrieve the registered frame by name and position it
    UI_FRAME(ToolTip);
    UI_SetParent(ToolTip, parent);
    UI_SetSize(ToolTip, 0.22, 0.10);
    UI_SetPoint(ToolTip, FRAMEPOINT_BOTTOMRIGHT, NULL, FRAMEPOINT_BOTTOMRIGHT, 0.0, 0.16);
}
```

`UI_ParseFDF_Buffer` parses the string in place (it is mutated during parsing, so pass a writable copy) and stores the resulting `frameDef_t` templates in the global registry. After the call you can retrieve a frame by name with `UI_FindFrame` or the `UI_FRAME` convenience macro, then position and parent it as usual.

**2. Programmatic C API**

Individual frames can be created directly in C without any FDF text. Initialise a `FRAMEDEF` struct with `UI_InitFrame`, set its properties via the helper functions, then emit it to the client with `UI_WriteFrame`:

```c
// src/game/hud/ui_log.c
static void ui_print_text(LPGAMECLIENT client, LPCSTR message) {
    FRAMEDEF text;
    UI_InitFrame(&text, FT_TEXT);
    text.Font.Index = gi.FontIndex("MasterFont", 10);
    text.Text = message;
    UI_SetPoint(&text, FRAMEPOINT_TOPLEFT,
                NULL, FRAMEPOINT_TOPLEFT, 0.05, -0.30); // anchor to screen TOPLEFT
    UI_WriteFrame(&text);                               // emit to the outgoing message
}
```

Key helpers available on any `FRAMEDEF`:

| Function | Effect |
|---|---|
| `UI_InitFrame(frame, type)` | Zero-initialise and set the frame type (`FT_TEXT`, `FT_BACKDROP`, `FT_COMMANDBUTTON`, …) |
| `UI_SetPoint(frame, point, relativeTo, targetPoint, x, y)` | Anchor one of the frame's nine points to another frame |
| `UI_SetSize(frame, width, height)` | Set explicit width/height |
| `UI_SetText(frame, fmt, ...)` | Printf-style text for label/text frames |
| `UI_SetTexture(frame, name, decorate)` | Assign a texture by skin-entry name |
| `UI_SetParent(frame, parent)` | Attach the frame to a parent in the hierarchy |
| `UI_WriteFrame(frame)` | Serialize one frame into the outgoing `svc_layout` message |
| `UI_WriteFrameWithChildren(frame, parent)` | Serialize a frame and its entire sub-tree |

**3. Sending a UI layer to a client**

Both approaches write frames into an outgoing buffer. The `UI_WRITE_LAYER` macro wraps the entire send sequence — opening the layer message, calling a builder function, and unicasting the result to a single client:

```c
// open layer, call builder, close and send
UI_WRITE_LAYER(edict, ui_print_text, LAYER_MESSAGE, message);

// equivalent explicit form
void UI_ShowText(LPEDICT ent, LPCVECTOR2 pos, LPCSTR text, FLOAT duration) {
    UI_WriteStart(LAYER_MESSAGE);          // begin svc_layout for this layer
    ui_print_text(ent->client, text);      // emit frames into the buffer
    gi.WriteLong(0);                       // end-of-list sentinel
    gi.unicast(ent);                       // send to the client
}
```

Available layers (defined in `g_local.h`):

| Constant | Purpose |
|---|---|
| `LAYER_PORTRAIT` | Unit portrait |
| `LAYER_CINEMATIC` | Cinematic panel |
| `LAYER_CONSOLE` | Main HUD console |
| `LAYER_COMMANDBAR` | Ability/command buttons |
| `LAYER_INFOPANEL` | Selected-unit info panel |
| `LAYER_INVENTORY` | Inventory slot buttons |
| `LAYER_MESSAGE` | On-screen text messages |
| `LAYER_QUESTDIALOG` | Quest dialog overlay |

---

## Build System

The project builds three shared libraries and one executable:

1. **libcmath3** — mathematics (vectors, matrices, quaternions, geometric primitives); no external dependencies
2. **librenderer** — OpenGL rendering engine; depends on `libcmath3`, SDL2, StormLib, libjpeg
3. **libgame** — server-side game logic; depends on `libcmath3`
4. **openwarcraft3** — main executable linking all three libraries plus SDL2 and StormLib

The build is driven by a `Makefile` for Linux/macOS and a `premake5.lua` that can generate Visual Studio 2022, Xcode, or GMake project files.

## External Dependencies

- **StormLib**: reads Warcraft III MPQ archives
- **SDL2**: windowing, input, and OpenGL context
- **libjpeg**: JPEG texture decoding
- **OpenGL**: 3D rendering (system-provided)

## Current Status
- Functional game implementation with basic Warcraft III features
- Support for original v1.0 assets with ongoing work for v1.29b compatibility
- Active development focused on expanding game feature completeness
- Cross-platform support for Linux and macOS environments
