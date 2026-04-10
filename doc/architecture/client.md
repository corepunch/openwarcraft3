# Client Architecture

The client (`src/client/`) is the presentation layer of OpenWarcraft3. It owns the SDL2 window, the OpenGL context, the input system, and the connection to the server. It never runs game logic — all simulation takes place on the server.

## Startup

The client is initialised by `CL_Init` in `cl_main.c`, which:

1. Opens a connection to the server via `NET_OpenLoopback` (`src/common/net.c`).
2. Registers client-side console commands.
3. Sends a `clc_connect` message to the server.

The server responds with `svc_serverdata` (map name, game parameters) followed by a stream of entity baselines and the initial UI layout.

## Main Loop

`CL_Frame(DWORD msec)` is called once per rendered frame by the platform layer (`src/common/main.c`). It performs the following steps in order:

```c
void CL_Frame(DWORD msec) {
    cl.time += msec;
    CL_ReadPackets();   // 1. apply incoming server messages
    CL_Input();         // 2. sample keyboard / mouse
    CL_SendCommand();   // 3. forward commands to server
    CL_PrepRefresh();   // 4. build scene for the renderer
    SCR_UpdateScreen(); // 5. draw the frame
}
```

### 1. CL_ReadPackets

Drains the loopback receive buffer and dispatches each message by its `svc_*` opcode:

| Opcode | Handler | Effect |
|--------|---------|--------|
| `svc_serverdata` | `CL_ParseServerData` | Store map path, spawn player camera |
| `svc_spawnbaseline` | `CL_ParseBaseline` | Initialise entity `s` field from delta |
| `svc_packetentities` | `CL_ReadPacketEntities` | Apply per-frame entity delta |
| `svc_layout` | `CL_ParseLayout` | Store serialised UI blob |
| `svc_disconnect` | — | Reset client state |

### 2. CL_Input

`CL_Input` (in `cl_input.c`) reads the current SDL2 keyboard and mouse state and fills a `usercmd_t` struct:

```c
typedef struct {
    DWORD  msec;
    DWORD  buttons;   // bitmask of pressed buttons
    SHORT  forwardmove;
    SHORT  sidemove;
    float  angles[3]; // camera Euler angles
} usercmd_t;
```

Camera rotation is applied to the stored `cl.viewangles` every frame. Mouse button events are translated to `BUTTON_*` bitmask bits. Keyboard events produce `CL_KeyEvent` calls which feed the console and keyboard bindings.

### 3. CL_SendCommand

Serialises the current `usercmd_t` as a `clc_move` message and writes it to the loopback send buffer for the server to read on its next `SV_ReadPackets` call.

For higher-level actions (right-click, ability use, unit selection), the input code sends dedicated `clc_*` messages that the server's command dispatcher (`src/game/g_commands.c`) handles.

### 4. CL_PrepRefresh

Builds the render scene:
- Sets the camera view origin and angles from `cl.viewangles`.
- Calls `CL_AddEntities` which iterates `cl.entities` and calls `V_AddEntity` for each visible entity. Entities with the `EF_NODRAW` flag are skipped.
- Client-side temporary effects (`src/client/cl_tent.c`) add short-lived visual-only entities such as blood splats and hit sparks.

### 5. SCR_UpdateScreen

Calls into the renderer library:
1. `R_BeginFrame` — clear colour/depth, update matrices.
2. `R_RenderFrame` — draw all entities, terrain, water, particles.
3. `R_DrawUI` — rasterise the cached UI layout blob.
4. `R_EndFrame` — SDL2 `GL_SwapWindow`.

## Entity Interpolation

The client keeps two snapshots per entity: `prev` and `current`. `CL_PrepRefresh` blends between them using a fraction derived from `cl.time` and the server frame interval, producing smooth motion even when the client render rate exceeds the server tick rate.

## Console and HUD

`cl_console.c` maintains an in-game console that can be toggled with the tilde key. `cl_scrn.c` overlays FPS, ping, and debug counters. Neither system communicates with the server.

## Key Files

| File | Purpose |
|------|---------|
| `src/client/cl_main.c` | `CL_Frame`, `CL_Init`, `CL_ReadPackets` |
| `src/client/cl_input.c` | Input sampling, `usercmd_t` construction |
| `src/client/cl_parse.c` | Server message handlers |
| `src/client/cl_view.c` | Camera, `CL_PrepRefresh`, `V_AddEntity` |
| `src/client/cl_tent.c` | Temporary client-side effects |
| `src/client/cl_scrn.c` | HUD / screen overlay |
| `src/client/cl_console.c` | In-game console |
| `src/client/keys.c` | Key event dispatch and binding table |
| `src/common/net.c` | Loopback transport shared by client and server |
