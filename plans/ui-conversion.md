# UI Module Conversion Plan (`ui.dll` Style)

## Goal

Move UI ownership into a separate `ui` shared module (Quake 3 style), keep engine code game-agnostic, and switch menu/HUD selection to route-based scene requests (for example `/`, `/main-menu`, `/single-player`, `/ingame/console`).

## Non-goals (for first pass)

- No browser/webserver runtime.
- No HTML/DOM/text rendering pipeline.
- No VM sandbox yet (native shared lib first).
- No broad gameplay refactors outside UI call sites.

## Core decisions

- Build `libui` as a native shared library now, with a stable C ABI.
- Keep existing network packet/UI frame serialization format for now.
- Treat UI as a scene router + builder, not a renderer.
- Use route strings as stable scene identifiers.
- Keep game logic actions in `game` (for example map load, quit), UI navigation actions in `ui`.

## Route and action conventions

### Routes

- `/`
- `/main-menu`
- `/single-player`
- `/single-player/maps`
- `/multiplayer`
- `/options`
- `/credits`
- `/ingame/console`
- `/ingame/commandbar`
- `/ingame/inventory`

### Actions

- `ui:navigate:/main-menu`
- `ui:navigate:/single-player`
- `game:load-map:Maps/Campaign/Human02.w3m`
- `game:quit`

## Phase 1: ABI and module skeleton

### 1) Add new UI public interface

Create a shared header (for example `ui/ui_api.h`) with:

- `struct ui_import` (callbacks provided by host)
- `struct ui_export` (functions provided by UI module)
- `GetUIAPI(struct ui_import *)`

Initial export functions:

- `Init()`
- `Shutdown()`
- `RenderRoute(LPEDICT ent, LPCSTR route)`
- `DispatchAction(LPEDICT ent, LPCSTR action)`

Acceptance:

- `ui` module builds with no references to `client` internals.
- Existing binaries link and run with stub behavior.

### 2) Add `libui` to build system

Update `Makefile`:

- Add `UI_LIB := build/lib/libui$(LIB_EXT)`
- Add `ui` target
- Build `libui` from `ui/` sources
- Link `openwarcraft3` and relevant tools/tests against `-lui`

Acceptance:

- `make build` produces `libui`.
- `openwarcraft3` starts and exits cleanly.

## Phase 2: host wiring (game + client)

### 3) Add host-side loader/wiring

Wire `GetUIAPI` similarly to existing module patterns:

- Initialize UI API during startup after core systems are ready.
- Store `ui_export` table globally where needed.
- Shutdown in reverse order.

Acceptance:

- UI API table is valid at runtime.
- No null-pointer calls in startup path.

### 4) Define host callback surface (`ui_import`)

Expose only generic callbacks needed by UI builder:

- frame write helpers (`WriteUIFrame`, message begin/end)
- resource lookups (`ModelIndex`, `ImageIndex`, `FontIndex`)
- FDF/config access (`ReadFileIntoString`, `ReadConfig`, `FindSheetCell`)
- memory helpers (`MemAlloc`, `MemFree`)
- action bridge callback to game command path

Acceptance:

- UI module no longer directly reaches game globals except via import callbacks.

## Phase 3: move menu scenes

### 5) Move main menu scene builders into `ui`

Move or wrap from current game UI files into `ui` module:

- main menu root and submenus
- background/top layer glue composition
- per-route scene selection

Map old calls to routes:

- `UI_ShowMainMenu` -> `RenderRoute(ent, "/main-menu")`
- `UI_ShowSinglePlayerMenu` -> `RenderRoute(ent, "/single-player")`
- `UI_ShowMapSelectMenu` -> `RenderRoute(ent, "/single-player/maps")`

Acceptance:

- Menu visuals and buttons remain functionally equivalent.
- No `Com_InMenuMode()` checks added in renderer/client hot paths.

### 6) Move menu button handling to action dispatch

Replace direct menu command coupling with `DispatchAction` route/actions:

- UI receives click action ids.
- `ui:navigate:*` handled in UI module.
- `game:*` forwarded to game callback.

Acceptance:

- Main menu navigation works through action ids only.

## Phase 4: move in-game UI scenes

### 7) Move HUD scene builders into `ui`

Move scene builders for:

- `/ingame/console`
- `/ingame/commandbar`
- `/ingame/inventory`

Keep gameplay data ownership in `game`; pass required read-only view/model data into UI build calls.

Acceptance:

- In-game panels render with same behavior as before.
- Game no longer owns frame tree construction details.

### 8) Introduce scene context objects

Define plain structs for UI data inputs (no hidden globals):

- player resource snapshot
- selected entity/unit list summary
- command button descriptors

Acceptance:

- UI rendering inputs are explicit and testable.

## Phase 5: remove legacy coupling

### 9) Delete or deprecate old direct scene entry points

After route path is stable, remove legacy direct functions where possible:

- old `UI_Show*` wrappers become thin compatibility layer or are removed.

Acceptance:

- Only one canonical path remains for scene rendering.

### 10) Reduce `Com_InMenuMode()` responsibility

Keep startup mode selection if needed, but remove menu-policy branching from gameplay/render loops.

Acceptance:

- Menu/in-game scene choice is route-driven and command-driven, not global-flag-driven in many subsystems.

## Phase 6: tests and diagnostics

### 11) Add route/action integration tests

Add tests that verify:

- known route renders without error
- unknown route returns graceful fallback
- action dispatch behavior (`ui:navigate`, `game:*` forward)

Acceptance:

- CI covers route table and action dispatch regressions.

### 12) Add diagnostics for scene/action tracing

Add optional debug logs:

- rendered route name
- action id received
- forwarded game action

Acceptance:

- debug output is enough to trace menu flow without stepping through debugger.

## Task checklist (execution order)

1. Create `ui/ui_api.h` ABI.
2. Add `ui/` module skeleton (`ui_main.c`).
3. Add `libui` target and link wiring in `Makefile`.
4. Wire `GetUIAPI` host-side startup/shutdown.
5. Implement `RenderRoute` with `/main-menu` and `/single-player`.
6. Route old menu entry points to `RenderRoute`.
7. Implement `DispatchAction` with `ui:navigate:*`.
8. Bridge `game:*` actions back to game command handlers.
9. Move `/ingame/console` and `/ingame/commandbar` builders.
10. Move `/ingame/inventory` builder.
11. Add integration tests for route/action behavior.
12. Remove legacy duplicate UI paths.

## Risks and mitigations

- ABI churn between modules:
  - Freeze `ui_api.h` early, version fields in import/export structs.
- Hidden dependencies on game globals:
  - Introduce explicit scene context structs and callback APIs.
- Regression in menu flow:
  - Keep thin compatibility wrappers during migration.
- Test fragility:
  - Add route/action smoke tests before moving all scenes.

## Acceptance criteria

1. `libui` is built and loaded as a separate module.
2. Main menu and single-player menu are rendered via route API.
3. In-game console/commandbar/inventory are rendered via route API.
4. Button clicks use action ids and action dispatcher.
5. Game actions remain game-owned and reachable from UI actions.
6. No new widespread `Com_InMenuMode()` branching appears in engine hot paths.
7. Existing startup modes (`menu`, `map`, `connect`) still work.
