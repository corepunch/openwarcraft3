/*
 * test_harness.h — Declarations for the test harness.
 *
 * The harness owns the global game state (gi, globals, level, game,
 * g_edicts), supplies mock implementations of all gi function-pointers,
 * and provides helpers used by the individual test suites.
 */
#ifndef test_harness_h
#define test_harness_h

/* Pull in the full game type hierarchy (same includes the game uses). */
#include "../src/game/g_local.h"
#include "../src/game/g_unitdata.h"

/* -----------------------------------------------------------------------
 * Forward declarations for functions defined in game .c files that have
 * no public header of their own.
 * --------------------------------------------------------------------- */

/* m_unit.c */
void unit_stand(LPEDICT self);
void unit_birth(LPEDICT self);
void unit_die(LPEDICT self, LPEDICT attacker);
BOOL unit_issueorder(LPEDICT self, LPCSTR order, LPCVECTOR2 point);
BOOL unit_issueimmediateorder(LPEDICT self, LPCSTR order);
BOOL unit_issuetargetorder(LPEDICT self, LPCSTR order, LPEDICT target);
BOOL unit_additem(LPEDICT edict, DWORD class_id);
BOOL unit_additemtoslot(LPEDICT edict, DWORD class_id, DWORD slot);

/* g_ai.c */
FLOAT unit_movedistance(LPEDICT self);

/* g_monster.c */
LPEDICT Waypoint_add(LPCVECTOR2 spot);
FLOAT   M_DistanceToGoal(LPEDICT ent);

/* skills/s_move.c */
void order_move(LPEDICT self, LPEDICT target);

/* g_phys.c */
void G_PushEntity(LPEDICT ent, FLOAT distance, LPCVECTOR2 direction);

/* -----------------------------------------------------------------------
 * Harness lifecycle
 * --------------------------------------------------------------------- */

/* Initialise all global state and mock gi before each test suite. */
void setup_game(void);

/* Reset globals after a test suite. */
void teardown_game(void);

/*
 * Reset the entity pool between individual tests that share a suite
 * (i.e., that run between a single setup_game() / teardown_game() pair).
 * Only the entity slots and the count are reset; gi, game, and level
 * globals remain unchanged.
 */
void reset_entities(void);

/* -----------------------------------------------------------------------
 * Unit-data helpers
 * --------------------------------------------------------------------- */

/*
 * Populate UnitsMetaData tables with hard-coded stats for two test units:
 *   hpea (Peasant)  — speed 270, HP 250, build time 45, collision 16
 *   hfoo (Footman)  — speed 270, HP 420, build time 60, collision 16
 * Called automatically by setup_game().
 */
void setup_test_unit_data(void);

/* -----------------------------------------------------------------------
 * Edict helpers
 * --------------------------------------------------------------------- */

/*
 * Allocate the next free slot in g_edicts, zero-initialise it, set
 * class_id and position, then return a pointer to it.  The unit is NOT
 * fully spawned (no model, no stand/birth callbacks) so the caller can
 * set up whatever it needs.
 */
LPEDICT alloc_test_unit(DWORD class_id, FLOAT x, FLOAT y);

/*
 * UNIT_ID(s) — build a unit-class DWORD from a 4-character string
 * literal at compile time, matching the memcpy-based FOURCC convention
 * used throughout the game code.
 *
 *   UNIT_ID("hpea")  →  same value as: DWORD id; memcpy(&id,"hpea",4);
 */
#define UNIT_ID(s)  MAKEFOURCC((s)[0], (s)[1], (s)[2], (s)[3])

/* -----------------------------------------------------------------------
 * SLK helpers
 * --------------------------------------------------------------------- */

/*
 * Pure-C implementation of the SLK cell-lookup from sheet.c, reproduced
 * here so the test binary does not need to link against StormLib.
 */
LPCSTR FS_FindSheetCell(sheetRow_t *sheet, LPCSTR row, LPCSTR column);

/*
 * Parse a minimal SLK spreadsheet stored as a NUL-terminated string.
 * Returns a heap-allocated chain of sheetRow_t / sheetField_t nodes.
 * The caller must free the result with free_slk_rows().
 */
sheetRow_t *parse_slk_string(const char *slk_text);

/* Release all rows returned by parse_slk_string(). */
void free_slk_rows(sheetRow_t *rows);

#endif /* test_harness_h */
