# GitHub Copilot Instructions

## Project Context

This codebase is inspired by **Quake 2**. The developer working on this project is deeply familiar with the architecture and source code of Quake 2 and wants to build a **real-time strategy (RTS) game** following the same style, conventions, and design philosophy found in Quake 2.

## Coding Style

- Follow the C coding style used in the Quake 2 source code (id Software style).
- Use the same patterns for module organization, data structures, and naming conventions as in Quake 2.
- Prefer simple, flat, and data-oriented design over complex object-oriented abstractions.
- Keep the code readable, compact, and close to the metal — minimize unnecessary indirection.
- Use `snake_case` for functions and variables, `ALL_CAPS` for constants and macros, matching Quake 2 conventions.

## Architecture

- Structure the project similarly to Quake 2: separate modules for rendering, game logic, input, sound, networking, etc.
- Game state should be managed in a straightforward, imperative style consistent with Quake 2's `g_*.c` / `cl_*.c` / `r_*.c` file layout.
- The engine and game code may be separated (similar to Quake 2's `game.dll` / `ref_gl` split) to allow modular replacement of subsystems.
- Runtime modules communicate through function tables (`R_GetAPI`, `UI_GetAPI`, game imports/exports). Prefer this boundary over direct cross-module dependencies.
- Use cvars for runtime choices: `r_module`, `ui_module`, `g_module`, `ui_start_route`, `net_enabled`, and `com_frame_limit`.

## Network State Contracts

- Do not casually add fields to `entityState_t`. It is a network snapshot/delta contract, so every new field increases protocol surface, bandwidth, baseline/delta behavior, save/load assumptions, and renderer/client coupling. Adding a field must be extremely well justified and should only happen after considering narrower alternatives such as existing state fields, configstrings, typed UI payloads, game-side state, or explicit commands.

## Engine/Game Boundary (Strict)

- Engine modules (`renderer/`, `client/`, `common/`, `server/` core paths) must remain game-agnostic.
- Never hardcode game-specific asset names, animation names, frame names, map/script conventions, or franchise-specific literals in engine code.
- Examples of forbidden engine literals: specific sequence names like `"MainMenu Stand"`, specific UI roots, or title-specific asset assumptions.
- If behavior needs title/game knowledge, put that policy in game-side code (`game/`, game.dll boundary) and pass generic parameters into engine APIs.
- Prefer generic fallbacks in engine code (caller-provided names, first available sequence, data-driven metadata) rather than title-specific heuristics.

## Domain

- This is a **real-time strategy game** (RTS), so game logic should account for unit management, pathfinding, resource gathering, building construction, and large numbers of entities — adapted from the Quake 2 entity/server model where applicable.

## MPQ Inspection Workflow

- When investigating Warcraft III assets, prefer using the local CLI utility `build/bin/mpqtool` instead of guessing file paths.
- Use `ls` mode to browse archive structure incrementally:
	- `build/bin/mpqtool -mpq <path-to-mpq> ls`
	- `build/bin/mpqtool -mpq <path-to-mpq> ls <subdir>`
- Use `cat` mode to dump file contents to stdout so output can be piped or redirected:
	- `build/bin/mpqtool -mpq <path-to-mpq> cat <archive-file>`
	- Example with redirect: `build/bin/mpqtool -mpq <path-to-mpq> cat Scripts/war3map.j > /tmp/war3map.j`
- Normalize slashes as needed when querying paths; both `\` and `/` are accepted by the tool input.
- For agent workflows, default to this tool whenever you need to discover MPQ contents, inspect text assets, or extract raw file bytes for analysis.

## MDX Inspection Workflow (mdxtool)

- Use `build/bin/mdxtool` to validate MDX assets and detect data problems before debugging render code.
- CLI synopsis:
	- `build/bin/mdxtool -mpq <path-to-mpq> -model <archive-model-path> [--anim <sequence>] [--use-model-camera] [--front-ortho] [--info] [--dump-all] [--once]`
- Viewer mode (opens window):
	- `build/bin/mdxtool -mpq <path-to-mpq> -model <archive-model-path>`
	- Example: `build/bin/mdxtool -mpq data/Warcraft\ III/War3.mpq -model UI\Glues\MainMenu\WarCraftIIILogo\WarCraftIIILogo.mdx`
- Front-ortho viewer mode for flat UI layers:
	- `build/bin/mdxtool -mpq <path-to-mpq> -model <archive-model-path> --front-ortho`
	- Example: `build/bin/mdxtool -mpq data/Warcraft\ III/War3.mpq -model UI\Glues\SpriteLayers\TopRightPanel.mdx --front-ortho`
- Info mode (no window, stdout only):
	- `build/bin/mdxtool -mpq <path-to-mpq> -model <archive-model-path> --info`

Common flags:
- `--anim <sequence>`: render or inspect a specific sequence by name, e.g. `--anim "MainMenu Stand"`.
- `--use-model-camera`: prefer the first embedded MDX camera when present.
- `--front-ortho`: use a front-facing orthographic preview camera for flat UI models without useful embedded cameras.
- `--info`: print model metadata and chunk counts without opening a window.
- `--dump-all`: print loaded model details including nodes, bones, geosets, materials, and cameras.
- `--once`: render one frame and exit; useful for scripted diagnostics and agent workflows.

When to use `--info`:
- Confirm the model exists and loads from MPQ path.
- Check whether a model has cameras (`CAMS`) for portrait/model-camera paths.
- Check sequence counts (`SEQS`) for Birth/Stand/Death animation expectations.
- Check textures (`TEXS`) and pivots (`PIVT`) for basic model completeness.
- Check optional systems that often explain missing visuals:
	- lights (`LITE`)
	- particle emitters (`PRE2`)
	- attachments (`ATCH`)
	- helpers (`HELP`)
	- bones (`BONE`)
	- collision shapes (`CLID`)
	- geosets (`GEOS`) and geoset anims (`GEOA`)

Expected output style:
- `mdxtool --info: model=<path> size=<bytes>`
- one line per relevant chunk with counts, e.g. `SEQS: count=...`, `CAMS: count=...`, `LITE: count=...`.

Agent guidance:
- Prefer `--info` first for existence, chunk counts, and camera availability.
- Use `--dump-all --once` when chunk summaries are not enough and you need loaded geoset, material, bone, or animation-state details.
- Use `--front-ortho` for glue sprites, panel layers, logos, and other flat UI-facing models.
- Use `--use-model-camera` only when the model actually contains a useful embedded camera.

Use this output in bug reports/diagnostics so rendering issues can be triaged from data facts (camera/lights/particles/sequence availability) without requiring screenshots.

## UI Text Renderer Workflow

- Use `make run-ui-text` to inspect client-side UI rendering without opening a window or taking screenshots.
- Default command:
	- `make run-ui-text UI_ROUTE=/main`
- Equivalent explicit command:
	- `build/bin/openwarcraft3 -data=data/Warcraft\ III -net_enabled=0 -r_module=stdout -ui_start_route=/main -com_frame_limit=1`
- `r_module=stdout` selects the text renderer.
- `net_enabled=0` avoids binding the UDP port for menu-only diagnostics.
- `com_frame_limit=1` exits after one frame and skips writing `share/config.cfg`.

Expected output includes:
- `load_texture`, `load_model`, `load_font`
- `draw_portrait`, `draw_sprite`
- `draw_image` with texture name, screen rect, UV rect, blend mode, color, rotation
- `draw_text` with font, rect, measured size, color, translated text
- `draw_sys_text` for console overlay output

Agent guidance:
- Prefer the stdout renderer first for UI layout, FDF translation, button state, backdrop tiling, UV, color-code, and route-composition bugs.
- Use `mdxtool --info` first when a UI model itself may be missing or malformed.
- `fdftool` is no longer the primary UI inspection path; Phase 8 moved UI rendering into the client-side UI library.

## UI Screen Authoring Conventions

- In client/UI code, never define or hardcode UI elements, layout coordinates, textures, frame names, or control structures that can be read from FDF. Parse and reuse the actual FDF frames/templates, then bind dynamic data into those frames.
- The only exception is `game.dll` / `game/` code, where there is no FDF parser. Server-authored gameplay HUD payloads may generate simple proxy frames there when needed.
- In `ui/screens/*.c`, prefer `UI_FRAME(...)` and `UI_CHILD_FRAME(...)` for readability and FDF-name coupling.
- Use `UI_FindChildFrame(...)` when it is clearly shorter or cleaner than introducing temporary macro-bound locals.
- Avoid excessive pointer null-check noise in screen controllers. Prefer one scene-level readiness gate (early return) over repeated per-widget checks.
- If a required root frame is missing, fail fast for that screen and skip further scene setup/update work.
- Keep frame names data-driven by FDF and avoid hardcoded lookup strings when macro-based lookup can use the frame identifier directly.
