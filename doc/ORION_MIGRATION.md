# Orion-UI Migration Notes

This document describes the port of OpenWarcraft3 from SDL2 to
[orion-ui](https://github.com/corepunch/orion-ui) and records any gaps or
additions that were needed during the process.

---

## What Changed

### Platform / Windowing (`src/renderer/r_main.c`, `src/common/main.c`)

| Before (SDL2) | After (orion-ui) |
|---|---|
| `SDL_Init` / `SDL_CreateWindow` / `SDL_GL_CreateContext` in `R_Init` | `ui_init_graphics(flags, title, w, h)` called from `main()` before `Com_Init()` |
| `SDL_GL_SwapWindow` + `SDL_Delay` in `R_EndFrame` | `repost_messages()` called from `UI_ProcessEvents()` in the main loop |
| `SDL_GetTicks()` for frame timing | `axGetMilliseconds()` from `platform/platform.h` |
| Single fullscreen window | Three orion-ui windows (game, log, map selector) |

### Renderer FBO (`src/renderer/r_main.c`, `src/renderer/r_local.h`)

The renderer no longer renders directly to the default framebuffer.  Instead,
every game frame is rendered into a dedicated off-screen FBO (`RT_GAME`) whose
colour attachment is a `GL_RGBA` / `GL_UNSIGNED_BYTE` texture.  The game
window's `evPaint` handler blits this texture to the window via `draw_rect()`.

New enum entry: `RT_GAME` in the `RT_*` enum (r_local.h).  
New field: `DWORD game_depth_rbo` in `render_globals` for the depth
renderbuffer attached to the game FBO.  
New function: `DWORD R_GetGameTexture(void)` — returns the texture ID.

`R_SetupGL(false)` now binds `tr.rt[RT_GAME]->buffer` instead of `0` so that
all non-shadow game rendering lands in the game FBO.

### Input (`src/client/cl_input.c`)

SDL event polling (`SDL_PollEvent`) has been removed.  Input is now driven by
orion-ui window messages delivered to the game window proc in `src/ui/ui_main.c`.

New bridge functions in `cl_input.c`:

```c
void CL_MouseButtonDown(int button, float x, float y, unsigned int time);
void CL_MouseButtonUp  (int button, float x, float y, unsigned int time);
void CL_MouseMove      (float x, float y, float dx, float dy);
void CL_KeyDown        (unsigned char key, unsigned int time);
void CL_KeyUp          (unsigned char key, unsigned int time);
```

`CL_Input()` is now a no-op that only resets the per-frame mouse-event flag.

### Console / Log Window (`src/client/cl_console.c`)

A hook mechanism was added:

```c
void CON_SetLogHook(void (*hook)(const char *msg));
```

`ui_main.c` registers a hook that appends every `CON_printf` message to the
log window's multiedit control.

### New UI Layer (`src/ui/ui_main.c`)

Three windows are created at startup:

1. **Game Window** (1024 × 768) — blits the game FBO via `draw_rect()` and
   routes all keyboard/mouse events to the game via the bridge functions above.
2. **Log Window** (320 × 400) — a read-only `win_multiedit` that mirrors
   `CON_printf` output via the log hook.
3. **Map Selector** (320 × 120) — lists `*.w3m` files found in the MPQ via
   `FS_ListMaps()` and calls `SV_Map()` when the player clicks "Load".

Public API (`src/ui/ui_main.c`):

```c
void UI_Init(void);          // create windows, register log hook
void UI_ProcessEvents(void); // drain event queue + repost_messages
int  UI_IsRunning(void);     // wraps ui_is_running()
void UI_Shutdown(void);      // calls ui_shutdown_graphics()
```

### Common (`src/common/common.c`, `src/common/common.h`)

`FS_ListMaps(char (*out)[260], int max_maps)` was added.  It enumerates all
`*.w3m` files in the primary MPQ archive using
`SFileFindFirstFile` / `SFileFindNextFile` and stores the internal MPQ paths
in *out*.  Returns the number of entries written.

---

## Header Conflict: `rect_t`

The game's `src/cmath3/types/rect.h` defines `rect_t` as
`struct rect { float x, y, w, h; }`, while orion-ui's `user/user.h` defines
`rect_t` as `typedef struct rect_s { int x, y, w, h; } rect_t`.

Both definitions cannot be in scope at the same time.  The workaround used
here is that `src/ui/ui_main.c` **does not include any game headers**.
Instead it forward-declares all game functions it calls using only primitive
C types.  All game-side bridge functions live in `cl_input.c` (which includes
game headers but not orion-ui headers).

### Recommendation for orion-ui

Rename `rect_t` to `ui_rect_t` in orion-ui (or use a namespace prefix like
`orion_rect_t`) so that the type can coexist with application-defined
`rect_t` types without requiring workarounds.

---

## Missing / Incomplete Features in orion-ui

The following functionality was needed during this port but is not yet present
in orion-ui.  These items should be moved upstream:

| Feature | Status | Notes |
|---|---|---|
| `lbGetCurSel` message for `win_listbox` | **Assumed present** — verify that `lbGetCurSel` returns the selected index as `(void*)(intptr_t)index`; document this in `commctl/listbox.h`. |
| `meAppendText` message for `win_multiedit` | **Assumed present** — verify that posting `meAppendText` with `lparam = char*` appends a line; document in `commctl/multiedit.h`. |
| `meSetReadOnly` message | **Assumed present** — document in commctl headers. |
| Window title setter `set_window_title()` | **Assumed present** — if not, can be replaced by `send_message(win, evSetText, 0, title)`. |
| `set_window_pos()` for child resize on `evSize` | **Assumed present** — needed to resize child controls when the parent window is resized. |

---

## Build Notes

The Makefile now:

1. Builds the platform shared library via its own Makefile:
   ```
   $(MAKE) -C vendor/orion-ui/platform OUTDIR=build/lib
   ```
2. Unity-builds orion-ui (user + kernel + commctl) into a static archive
   `build/lib/liborion.a`.
3. Links `openwarcraft3` against `liborion.a -lplatform` instead of `-lSDL2`.

Required system packages (Linux/X11):
```
libx11-dev libegl-dev libgl-dev libcglm-dev
```
(Wayland: `libwayland-dev libwayland-egl-backend-dev libxkbcommon-dev libegl-dev libgl-dev libcglm-dev`)

SDL2 is no longer a runtime dependency.
