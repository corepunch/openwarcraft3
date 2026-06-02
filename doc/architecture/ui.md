# UI System Architecture

As of Phase 8 (May 2026), all UI logic in OpenWarcraft3 runs **client-side** in the selected UI library. Warcraft III UI sources live in `games/warcraft-3/ui/`; the shared client-facing UI API is declared in `client/ui.h`. The server is now game-agnostic and provides unit data through a query protocol when clients request it. This follows the Quake 3 Arena pattern where UI is a separate client-side library.

## Migration from Server-Side UI (Phase 1-7)

Previously, all UI logic ran on the server in `game/ui/` and `game/hud/`. The server would generate complete frame trees and send them via `svc_layout` messages. This violated the game-agnostic principle and bloated the game library by ~107KB.

Phase 8 removed all server-side UI code. The game library now only provides **data** (command buttons, inventory, build queue) via callbacks, and the client handles all rendering and layout.

## Quick Navigation

**Looking for a specific topic?**

- **Complete end-to-end flow** (client input → command/data update → rendering): See [UI_FLOW.md](./UI_FLOW.md)
- **Runtime cvars and stdout renderer**: See [Runtime Modules and Cvars](./runtime.md)
- **How to add a new UI element**: See [Adding a New UI Element](#adding-a-new-ui-element) below
- **FDF file syntax**: See [FDF File Format](../file-formats/fdf.md)

## Concepts

| Term | Meaning |
|------|---------|
| `frameDef_t` | A template parsed from an FDF file and stored in the registry |
| `FRAMEDEF` | The alias used by the C API for a frame definition being constructed |
| `uiScreen_t` | A screen controller with init, refresh, draw, and input callbacks |
| `ui_start_command` | Cvar selecting the first UI command, e.g. `menu_main` |

## Initialisation

`UI_Init` (`games/warcraft-3/ui/ui_main.c`) is called once from `CL_Init` when the client starts:

1. Loads UI library via `UI_GetAPI(uiImport_t)` function table.
2. Client provides import functions: memory allocation, file I/O, MPQ access.
3. UI library loads Warcraft III `.fdf` files from MPQ via `UI_ParseFDF` (`games/warcraft-3/ui/ui_fdf.c`).
4. UI executes `ui_start_command`, defaulting to `menu_main`.
5. Screen controller manages frame lifecycle, drawing, and input routing.

## Frame Definition Files (FDF)

FDF files declare frame templates hierarchically. `UI_ParseFDF_Buffer` (`games/warcraft-3/ui/ui_fdf.c`) accepts a writable C string, tokenises it, and registers `frameDef_t` entries in a global lookup table. Frames can inherit from a base template with `InheritsParts`.

See [FDF File Format](../file-formats/fdf.md) for the full syntax reference.

## Unit Data Query Protocol (Phase 8)

When the player selects units, the client requests unit UI data from the server:

### Client → Server: `clc_request_unit_ui`

```c
// client/cl_input.c - Selection complete
CL_RequestUnitUI(num_selected, entity_nums);

// Message format:
MSG_WriteByte(&cls.netchan.message, clc_request_unit_ui);
MSG_WriteByte(&cls.netchan.message, num_selected);
for (i = 0; i < num_selected; i++)
    MSG_WriteShort(&cls.netchan.message, entity_nums[i]);
```

### Server → Client: `svc_unit_ui`

Server queries game DLL for unit data and responds:

```c
// server/sv_unit_ui.c - Handle request
gameCommandButton_t buttons[12];
BYTE num_buttons = ge->GetCommandButtons(ent, buttons, 12);

// Message format (for each selected entity):
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

### Client Storage

```c
// games/warcraft-3/ui/screens/console_ui.c - Store unit data
static uiUnitData_t cached_units[MAX_CACHED_UNITS];
static DWORD cached_unit_count = 0;

void ConsoleUI_UpdateUnitUI(DWORD num_units, uiUnitData_t *units) {
    cached_unit_count = num_units;
    memcpy(cached_units, units, sizeof(uiUnitData_t) * num_units);
    // Rendering uses cached_units[] to draw command card
}
```

## Frame Tree Layout

The frame tree is a depth-first hierarchy managed client-side by the UI library. Each frame stores:

- **Anchor point** — which of the nine anchor points (`TOPLEFT` … `BOTTOMRIGHT`) of this frame is attached to which point of which parent frame, and at what X/Y offset.
- **Size** — explicit width and height if set.
- **Texture** — texture name resolved via UI library's asset loader.
- **Text** — optional UTF-8 string for labels.
- **Stat** — optional `PLAYERSTATE_*` tag that makes the text field track a live player stat (gold, lumber, supply, etc.) and update automatically every frame.
- **Type-specific data** — backdrop edge insets, button up/down/hover states, label font index.
- **Children** — singly-linked list of child frames.

The UI library maintains the frame tree and recalculates layout when frames are added, removed, or resized.

## Client-Side Rendering (Phase 8+)

The Warcraft III UI library (`games/warcraft-3/ui/`) handles all frame rendering:

1. Screen controller (e.g., `console_ui.c`) updates frame states based on game data.
2. `UI_DrawFrame` walks the frame tree and dispatches each frame to type-specific renderers.
3. Frame renderers call back into the client's renderer import functions to draw quads, text, and models.
4. Resource stats (gold, lumber, food) update automatically from `cl.playerstate`.

The client parses FDF files locally and maintains the complete frame hierarchy. No serialized UI blobs are transmitted over the network.

## Menu Commands

UI screens are selected by explicit menu commands. This keeps navigation in the same Quake-style command stream as buttons, console input, and startup configuration.

Examples:

| Command | Purpose |
|---------|---------|
| `menu_main` | Main menu |
| `menu_game` | Single-player menu |
| `menu_lan_refresh` | Refresh LAN map list |
| `menu_startserver` | LAN create-game screen |

The startup command is configurable:

```bash
build/bin/openwarcraft3 -data=data/Warcraft\ III -ui_start_command=menu_main
```

For isolated UI diagnostics:

```bash
make run-ui-text
```

That command uses `r_module=stdout` and `com_frame_limit=1` to print one frame of draw calls and exit. Menu-only diagnostics do not open UDP sockets.

## Dynamic Updates

UI updates happen client-side in response to:

- **Selection changes** — `IN_SelectUp()` triggers `CL_RequestUnitUI()`, server responds with unit data, `ConsoleUI_UpdateUnitUI()` stores it.
- **Command card** — client renders buttons from `cached_units[].buttons[]` received via `svc_unit_ui`.
- **Resource display** — frames poll `cl.playerstate` directly, no network traffic.
- **Chat messages** — (future) append frames to chat panel.

All rendering happens client-side; server only provides game data (unit stats, abilities, inventory).

## Stdout Renderer Diagnostics

The stdout renderer is the preferred first-pass diagnostic for UI rendering. It implements the same renderer API as the OpenGL renderer but writes draw calls to stdout:

- `load_texture`, `load_model`, `load_font`
- `draw_portrait`, `draw_sprite`
- `draw_image` with texture name, screen rect, UV rect, color, blend mode, and rotation
- `draw_text` with font, rect, measured size, color, and translated text
- `draw_sys_text` for console overlay text

Use it to check screen composition, frame positions, backdrop tiling, missing assets, hover/pressed state changes, translated strings, and Warcraft color codes without taking screenshots.

## UI Test Asset Policy

UI test fixtures and assets are repository-owned. Do not copy Warcraft III assets into tests.

- Author source fixtures in `tests/resources-src/`.
- Generate deterministic BLP/MDX assets into `build/tests/resources/`.
- Pack and validate `build/tests/tests.mpq` via `make test-assets`.

The normal test pipeline enforces this by running `test-assets` before `make test`.

For UI-impacting changes, use `make test-ui` as the required gate. It runs:

- FDF parser/frame-graph suites
- UI layout conformance suites
- End-to-end client UI rendering suites
- Tool-backed oracle suites (`mdxtool --info`)

Note: `fdftool` was removed in Phase 8 as it depended on deleted server-side UI code. Use `make run-ui-text` for UI draw-call inspection and `mdxtool --info` for model data inspection.

## Adding a New UI Element

1. In the appropriate screen controller (e.g., `games/warcraft-3/ui/screens/console_ui.c`), define the frame using the FDF API or programmatically:

```c
// Option A: Reference FDF-defined frame
LPFRAMEDEF btn = UI_FindFrame("MyCommandButton");

// Option B: Create frame programmatically
FRAMEDEF btn;
UI_InitFrame(&btn, FT_COMMANDBUTTON);
UI_SetPoint(&btn, FRAMEPOINT_BOTTOMLEFT, parent, FRAMEPOINT_BOTTOMLEFT, 0.40, 0.10);
UI_SetSize(&btn, 0.04, 0.04);
UI_SetTexture(&btn, "CommandButtonNormal", true);
```

2. Update frame state in response to game events (e.g., unit selection, stat changes):

```c
void ConsoleUI_UpdateUnitUI(DWORD num_units, uiUnitData_t *units) {
    // Store unit data
    cached_unit_count = num_units;
    memcpy(cached_units, units, sizeof(uiUnitData_t) * num_units);
    
    // Frame tree updates automatically on next render
}
```

3. The UI library's render loop will automatically draw updated frames.

## Key Files

### Client-Side UI (Phase 8+)

| File | Purpose |
|------|---------|
| `server/sv_unit_ui.c` | Handle `clc_request_unit_ui`, query game DLL |
| `games/warcraft-3/game/g_unit_ui.c` | `G_GetCommandButtons`, `G_GetInventory`, `G_GetBuildQueue` |
| `games/warcraft-3/game/g_ui_stubs.c` | No-op stubs for legacy server-side UI functions |
| `server/game.h` | `game_export` callbacks for unit data queries |
| `client/ui.h` | Shared UI module API declaration |
| `games/warcraft-3/ui/ui_main.c` | `UI_GetAPI`, library entry point, screen routing |
| `games/warcraft-3/ui/ui_fdf.c` | FDF parser and programmatic frame API |
| `games/warcraft-3/ui/ui_render.c` | Frame rendering dispatch |
| `games/warcraft-3/ui/screens/console_ui.c` | In-game HUD (resource bar, command card, inventory) |
| `games/warcraft-3/ui/screens/main_menu.c` | Main menu, single player menu, etc. |
| `client/cl_unit_ui.c` | `CL_ParseUnitUI` — receive unit data from server |
| `client/cl_input.c` | Selection tracking, query trigger |
| `renderer/r_font.c` | Bitmap font rasteriser |
| `common/common.h` | `clc_request_unit_ui`, `svc_unit_ui` opcodes |

### Server-Side Data Providers

| File | Purpose |
|------|---------|
| `games/warcraft-3/game/g_unit_ui.c` | Converts selected entities into command card, inventory, and build queue data |
| `server/sv_unit_ui.c` | Marshals unit UI data into `svc_unit_ui` messages |
| `client/cl_unit_ui.c` | Receives `svc_unit_ui` and forwards decoded data to the UI library |
