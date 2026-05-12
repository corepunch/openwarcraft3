# Plan: UI Y-Flip with Generated Test Assets

Refactor UI parser/serialization/layout code for deterministic testing first, then migrate to Warcraft-style Y-up UI coordinates using a fully repo-owned generated asset pipeline. No Warcraft III assets will be copied into tests. Test fixtures will be authored from scratch, generated into a small `tests.mpq`, and exercised through parser, serialization, layout, and sprite/model integration tests.

## Phases

- [x] **Phase 0** — Establish asset ownership rules and harness targets. Require all new UI tests to use test-authored resources only. Introduce a dedicated generated archive target (`build/tests/tests.mpq`) and source asset-spec directory (`tests/resources-src/`) plus generated output directory (`build/tests/resources/`). _Blocks all later work._

- [x] **Phase 1** — Upgrade tooling: MPQ archive creation and packing. Extend `mpqtool` with `create` and `pack` commands so tests can pack generated files into `tests.mpq` without introducing a separate archive workflow. Add list/cat verification. _Blocks all resource-backed integration tests._
  - [x] Add `SFileCreateArchive`, `SFileAddFileFromMemory`, `SFileFinalizeArchive` to `common/mpq.c`
  - [x] Wire `create` and `pack` commands in `tools/mpqtool.c`
  - [x] Build clean, smoke test passed

- [x] **Phase 2** — Texture generation support. Add a small tool in `tools/` to emit BLP2 test textures from declarative inputs (solid color, checker, grid, alpha mask, border atlas). _Depends on Phase 1._
  - [x] Add `tools/blpgen.c`
  - [x] Generate: `solid_white.blp` (1x1), `checker_8x8.blp`, `alpha_ring_16x16.blp`, `panel_border_32x32.blp`
  - [x] BLP2 BGRA first; `paletted` preset added for decoder coverage

- [x] **Phase 3** — Model generation support. Create a deterministic MDX fixture generator in `tools/` or extend `mdxtool` with generation modes. _Depends on Phase 1; parallel with Phase 2._
  - [x] Add `tools/mdxgen.c`
  - [x] Generate: `quad_sprite.mdx` (1x1 quad, 1 seq), `panel_sprite.mdx` (2x1.5 quad), `anim_pulse.mdx` (2 seqs Stand+Birth)
  - [x] Include texture-path assignment, BONE, PIVT; validated with `mdxtool --info`

- [x] **Phase 4** — Define resource strategy and fixture set. Finalize file types and references per fixture. _Depends on Phases 2–3._
  - [x] `TestUI/Frames/basic_layout.fdf`
  - [x] `TestUI/Frames/backdrop_variants.fdf`
  - [x] `TestUI/Frames/simple_sprite.fdf`
  - [x] `TestUI/Frames/animated_sprite.fdf`
  - [x] `TestUI/Textures/solid_white.blp`
  - [x] `TestUI/Textures/checker_8x8.blp`
  - [x] `TestUI/Textures/alpha_ring_16x16.blp`
  - [x] `TestUI/Textures/panel_border_32x32.blp`
  - [x] `TestUI/Models/quad_sprite.mdx`
  - [x] `TestUI/Models/panel_sprite.mdx`
  - [x] `TestUI/Models/anim_pulse.mdx`

- [x] **Phase 5** — Build `tests.mpq` generation pipeline. Add `make test-assets` target that generates all fixtures and packs them into `build/tests/tests.mpq`. _Depends on Phases 1–4._
  - [x] Add `make test-assets` Makefile target
  - [x] Verify `mpqtool ls/cat` against `tests.mpq` in CI smoke check

- [x] **Phase 6** — Refactor server-side UI code for testability (no behavior change). Extract seams in `game/ui/ui_fdf.c` (parser/frame-graph) and `game/ui/ui_write.c` (serialization) so tests can capture deterministic byte streams and frame graphs without live networking. _Blocks parser/serialization/end-to-end suites._

- [x] **Phase 7** — FDF parser and frame-graph suites (~45–60 tests). Cover frame declarations, inheritance, SetPoint/Anchor semantics, mixed frame hierarchies, malformed inputs, duplicate-name behavior, and path/string handling for backdrop and sprite/model fields. _Depends on Phases 4 and 6._

- [x] **Phase 8** — UI serialization and delta suites (~30–40 tests). Validate `uiFrame_t` masks, payload sizes, text handling, type-specific buffer payloads, and roundtrip decode. Include generated texture/model path references. _Depends on Phase 6; parallel with Phase 7._

- [x] **Phase 9** — Refactor client layout solver for deterministic conformance testing. Extract anchor math, parent queries, measurement hooks, and stat providers from `client/cl_scrn.c`. _Depends on Phase 6; parallel with Phases 7–8._

- [ ] **Phase 10** — Client layout suites (~30–40 tests). Cover single-anchor, dual-anchor stretch, center alignment, parent-relative recursion, offset signs, implied sizes, and Warcraft-style Y-up expectations. _Depends on Phase 9._

- [ ] **Phase 11** — End-to-end server→client UI tests against `tests.mpq` (~20–30 tests). Build frame tree from generated FDF, serialize on server path, parse on client path, solve bounds, assert deterministic outcomes. _Depends on Phases 5, 8, and 10._

- [ ] **Phase 12** — Tool-backed oracle tests (~5–10 tests, 15–25 assertions). Run `fdftool` and `mdxtool --info` against generated assets and compare normalized output summaries. _Depends on Phase 11._

- [ ] **Phase 13** — UI coordinate migration. Flip canonical projection/input/layout to Warcraft-style Y-up (0 at bottom, 0.6 at top), remove compensating sign hacks, use new suites to catch regressions. _Depends on Phases 7–12._
  - [ ] Flip ortho in `renderer/r_draw.c`
  - [ ] Fix mouse mapping in `client/cl_scrn.c`
  - [ ] Remove sign hacks in `SCR_GetAnchor`
  - [ ] All new suites green

- [ ] **Phase 14** — Cleanup and enforcement. Make generated-asset pipeline part of normal test flow, document resource ownership, require UI suites for changes touching parser/layout/serialization/sprites. _Depends on Phase 13._

## Test Targets

| Suite | Tests | Notes |
|---|---|---|
| Parser / FDF | 45–60 | FDF fixtures only |
| Serialization / delta | 30–40 | generated texture/model refs |
| Client layout | 30–40 | anchor math, Y-flip coverage |
| End-to-end archive-backed | 20–30 | `tests.mpq` |
| Tool-oracle | 5–10 (15–25 assertions) | fdftool / mdxtool |
| Asset generator / MPQ packing | 15–25 | BLP/MDX/MPQ correctness |
| **Total** | **145–205** | |

## Resource Decisions

- **FDF**: authored from scratch; never copied from Warcraft III data.
- **BLP**: generated deterministically in-repo; no proprietary textures. BLP2 BGRA first, then one paletted fixture.
- **MDX**: generated deterministic binary fixtures. Parametric generator preferred over hand-edited blobs.
- **MDL**: only if needed for `.mdl` path/fallback tests; not required for rendering.
- **MPQ**: `build/tests/tests.mpq` generated during build from source specs (not committed as binary).
- **External downloads**: avoid; only CC0/public-domain with explicit provenance if a case cannot be generated simply.

## Key Files

- `common/mpq.h` / `common/mpq.c` — MPQ write API (✅ done)
- `tools/mpqtool.c` — archive creation/packing (✅ done)
- `tools/blpgen.c` — BLP2 texture generator (✅ done)
- `tools/mdxtool.c` / `tools/mdxgen.c` — MDX fixture generator (✅ done)
- `tools/fdftool.c` — oracle/inspection path
- `game/ui/ui_fdf.c` — parser and frame-graph seams
- `game/ui/ui_write.c` — UI frame serialization seams
- `common/msg.c` — delta write/read verification
- `client/cl_parse.c` — layout blob ingest
- `client/cl_scrn.c` — layout solver and coordinate-sensitive behavior
- `renderer/r_draw.c` — UI ortho projection
- `tests/test_harness.c` / `tests/test_harness.h` — extend for UI/archive fixture plumbing
- `tests/resources-src/` — source-controlled FDF fixtures and asset specs
- `build/tests/tests.mpq` — generated artifact (not committed)
