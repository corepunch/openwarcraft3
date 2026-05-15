# Migrate Tests to Real `openwarcraft3` Host

## Goal

Move integration and game-behavior tests into the real `openwarcraft3` executable while keeping fast pure unit tests compiled separately in a standalone test binary.

The runtime command surface for this migration is:

- `-mpq=<path>` (existing)
- `-map=<path>` (existing)
- `-test=<name>` (new, testing builds only)

Example:

`openwarcraft3_test -mpq=build/tests/tests.mpq -map=main_menu -test=unit_commands`

## Core decisions

- Use `-test=<name>` for now (not `+test` yet).
- Guard all integration-test runtime hooks with `TEST`.
- Use `TEST` as the compile-time gate for all integration-test code.
- Use `build/tests/tests.mpq` for test runs.
- Integration tests must run without `war3.mpq`; `build/tests/tests.mpq` is the only required archive for CI and local test runs.
- Add a null renderer build for integration tests.
- Keep standalone unit tests compiled separately for pure logic and very fast execution.
- Migrate incrementally; do not delete all `tests/` in one step.

## Test split

### Standalone unit tests (keep separate)

These do not need full game bootstrapping and should remain in a separate test binary:

- parser, SLK/table parsing
- math helpers and small utility modules
- net/message helper units that do not require full server/client runtime
- tool helper tests

Concretely this likely includes:

- `tests/test_slk.c`
- `tests/test_net.c`
- `tests/test_tool_common.c`
- `tests/test_mpq_compat.c`
- selected low-level portions of `tests/test_jass.c` if they remain engine-independent

### Integration tests in real game (migrate)

These should run through normal game startup and command handling:

- `tests/test_unit.c`
- `tests/test_combat.c`
- `tests/test_movement.c`
- `tests/test_collision.c`
- `tests/test_api.c`
- `tests/test_game.c`
- `tests/test_ui_e2e.c`
- `tests/test_ui_oracle.c`

UI FDF/layout serialization tests can remain standalone if they stay deterministic and renderer-free.

## Phase 1: runtime plumbing

### 1) Add test CLI option in `common/main.c`

Parse `-test=<name>` alongside existing args for game-backed integration tests.

Behavior in testing builds:

1. Initialize engine normally (`Com_Init()`).
2. Apply requested startup mode (`-map` and/or menu/connect path).
3. If `-test` is set, dispatch test runner.
4. Exit with pass/fail code.

In non-testing builds:

- Either reject `-test` with clear message and nonzero exit, or omit parser branch entirely.

### 2) Add controlled exit path

Current frame loop is infinite. Add a controlled process-exit path for tests, e.g.:

- `Com_Exit(int code)` for clean subsystem shutdown and final process exit code.

### 3) Register test commands in game/runtime

Add testing-only registration and dispatch, e.g.:

- `Test_Run(const char *name)`
- an explicit registry of named test functions, similar to:

```c
typedef struct {
  const char *name;
  void (*func)(void);
  qboolean passed;
} test_func_t;

static test_func_t tests[] = {
  {"pmove_gravity", test_pmove_gravity},
  {"trace_box", test_trace_box},
  {NULL, NULL}
};
```

This should execute via normal runtime state after map/bootstrap setup.

## Phase 2: build system updates

### 4) Add `TEST` build variant

Add make targets/flags to build test-enabled app binary:

- `openwarcraft3_test` (or similarly named binary)
- compile with `-DOW3_TESTING`
- include test integration sources only in this build

### 5) Add null renderer target

Create a null renderer implementation and link it in integration-test build:

- no SDL window creation
- no OpenGL context
- no real draw calls
- safe no-op/stub returns for renderer API

This keeps integration tests deterministic and CI-friendly while preserving engine startup flow.

### 6) Keep and use `tests.mpq`

Use `make test-assets` to generate all required fixtures and package into:

- `build/tests/tests.mpq`

Integration tests should depend on this archive and avoid external Warcraft MPQ dependencies.

Explicit requirement:

- `-mpq=build/tests/tests.mpq` must be sufficient to run integration tests.
- Test execution must not require `data/Warcraft III/War3.mpq` to exist.
- If a test introduces new assets, those assets must be generated/packed into `tests.mpq` via `make test-assets`.

## Phase 3: first migration slice

### 7) Migrate one representative suite first

Start with unit commands/combat behavior suite (whichever currently provides best signal):

- move into game-side testing module (testing build only)
- execute through `-test=<suite>` path
- validate end-to-end pass/fail and exit code behavior

Do not delete old harness immediately.

## Phase 4: incremental migration and cleanup

### 8) Migrate remaining integration suites one-by-one

For each migrated suite:

- ensure test no longer depends on fake globals/gi stubs from `test_harness.c`
- ensure deterministic setup via real runtime + test fixtures in `tests.mpq`
- keep runtime duration bounded and auto-exit cleanly

### 9) Remove obsolete harness bootstrapping

After all candidate integration suites are migrated and stable:

- remove fake engine bootstrapping from standalone test path
- keep only minimal standalone harness needed for pure unit tests

## Make targets (proposed)

- `make test-unit`
  - builds/runs standalone pure unit tests
- `make test-integration`
  - builds `openwarcraft3_test` with `OW3_TESTING`
  - uses null renderer
  - ensures `test-assets` produced
  - runs tests via `-mpq=build/tests/tests.mpq -test=all` (no `war3.mpq` fallback)
- `make test`
  - may become aggregate: `test-unit` + `test-integration`

## Runtime behavior requirements

- `-test=<name>` returns `0` on pass, nonzero on fail.
- unknown test name returns nonzero and prints available tests.
- `-test=all` runs all registered integration suites and aggregates exit code.
- test mode should be deterministic:
  - fixed timestep or deterministic frame-driving where needed
  - predictable asset inputs from `tests.mpq`
  - no dependency on interactive UI/windowing

## Risks and mitigations

- Speed regression from full-engine tests:
  - keep pure unit tests standalone
  - run integration suites selectively and in parallel where safe
- Flakiness from renderer/network/time:
  - null renderer
  - deterministic runtime switches
  - avoid external services/assets
- Migration churn:
  - incremental suite-by-suite approach
  - maintain old path until replacement is proven

## Acceptance criteria

1. Can run an integration test directly through the game binary:
   - `openwarcraft3_test -mpq=build/tests/tests.mpq -test=<name>`
2. Integration tests run successfully when `data/Warcraft III/War3.mpq` is absent.
3. Integration test result maps to process exit code.
4. No SDL/OpenGL dependency for integration tests (null renderer path).
5. Standalone unit tests still run fast and independently.
6. Legacy harness-heavy bootstrapping removed only after migration completion.
