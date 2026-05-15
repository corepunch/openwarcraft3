# Plan: SLK Coverage Expansion with War3-Referenced Fixtures

Expand SLK parser and metadata test coverage with larger and more detailed SLK fixtures. Fixture design is guided by real Warcraft III tables inspected via `mpqtool`, while all committed test inputs remain repository-owned files in `tests/fixtures/`.

## Phases

- [ ] **Phase 0** — Ownership and provenance rules.
  - Require all committed SLK tests to consume repository fixtures only.
  - Allow `mpqtool` inspection of `data/Warcraft III/War3.mpq` strictly as reference input for fixture design.
  - Add review rule: no proprietary SLK files copied into repository.

- [ ] **Phase 1** — War3 reference sampling workflow (using mpqtool).
  - Capture realistic schema and value patterns from:
    - `Units\\UnitData.slk`
    - `Units\\UnitBalance.slk`
    - `Units\\UnitUI.slk`
    - `Units\\UnitWeapons.slk`
    - `Units\\UnitAbilities.slk`
    - one non-unit table (`Items` or `Doodads`)
  - Use commands like:
    - `build/bin/mpqtool -mpq data/Warcraft\ III/War3.mpq ls Units`
    - `build/bin/mpqtool -mpq data/Warcraft\ III/War3.mpq cat Units/UnitData.slk`
  - Record observed patterns (row/column scale, sparse cells, mixed numeric/text fields, C/F records) to drive fixture authoring.

- [ ] **Phase 2** — Add larger static fixture corpus.
  - Create bigger fixtures under `tests/fixtures/`, including:
    - wide table fixture (many columns)
    - tall table fixture (many rows)
    - sparse/missing-cell fixture
    - mixed-type fixture (ints/floats/bools/strings)
    - malformed fixture set (truncated or missing tokens)
    - custom unit fixtures that define game-playable units with realistic stats

- [ ] **Phase 3** — Parser conformance suite expansion.
  - Extend `tests/test_slk.c` and add parser-focused companion tests.
  - Add coverage for:
    - C and F row handling
    - case-insensitive field lookup
    - missing row/column behavior
    - large fixture parse determinism
    - malformed input robustness

- [ ] **Phase 4** — File-backed SLK loading tests.
  - Add harness helpers to load `.slk` files from `tests/fixtures/` and parse them.
  - Ensure tests cover runtime-adjacent paths (not only tiny inline strings).

- [ ] **Phase 5** — SLK-driven custom unit creation.
  - Add harness helper `setup_custom_unit_data_from_fixture()` to populate metadata tables from SLK fixture files.
  - Create test suites that:
    - Load custom unit definitions from fixtures
    - Allocate test game units using those definitions
    - Verify unit properties (collision, move type, stats) match SLK-loaded values
    - Run gameplay scenarios (movement, pathfinding, combat) using SLK-driven custom units

- [ ] **Phase 6** — Metadata integration tests.
  - [x] Break Phase 6 into commit-sized implementation checkpoints.
  - [x] Add harness support for user-created unit remap fixtures (`newUnitID` -> `originalUnitID`).
  - [ ] Add direct tests for `UnitIntegerField`, `UnitRealField`, and `UnitBooleanField` with known IDs.
  - [ ] Add direct tests for unknown-unit fallback behavior on all metadata accessors.
  - [ ] Add remap tests validating that user-created IDs resolve through original-unit metadata rows.
  - [ ] Add macro-wrapper coverage that exercises integer, real, and boolean metadata paths.
  - [ ] Keep metadata integration tests in `tests/test_slk.c` under the default `make test` suite.

- [ ] **Phase 7** — Parser hardening in `common/sheet.c`.
  - Implement fixes uncovered by new tests:
    - malformed token handling
    - bounds safety
    - deterministic failure behavior
  - Keep behavior simple and deterministic, matching existing style.

- [ ] **Phase 8** — Stress and regression lock-in.
  - Add high-volume fixtures and lookups.
  - Keep regression fixtures for each previously fixed parser bug.

- [ ] **Phase 9** — Enforcement and CI flow.
  - Ensure new SLK suites run in `make test` by default.
  - Document fixture update policy and required test updates for parser changes.

## Test Target Estimates

| Suite | Tests | Notes |
|---|---|---|
| Parser baseline and conformance | 30-45 | C/F records, malformed handling, deterministic parse |
| File-backed fixture parsing | 15-25 | large fixture ingestion from `tests/fixtures/` |
| Metadata integration | 20-30 | field conversion and unit macro accessors |
| Stress/regression | 10-20 | large tables and bug-lock fixtures |
| **Total** | **75-120** | significant increase over current SLK coverage |

## Key Files

- `plans/slk-coverage-tests.md`
- `tests/test_slk.c`
- `tests/test_harness.c`
- `tests/test_harness.h`
- `tests/fixtures/*.slk`
- `common/sheet.c`
- `game/g_metadata.c`
- `game/g_unitdata.h`
- `Makefile`
