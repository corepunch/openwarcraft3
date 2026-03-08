/*
 * tests/stubs/g_local.h — Minimal stub that replaces game/g_local.h
 * when compiling game/parser.c in isolation for unit tests.
 *
 * parser.c only uses the types and macros provided by shared.h and
 * parser.h; this stub satisfies that dependency without pulling in
 * the full game/server include tree.
 */
#include <stdlib.h>
#include <ctype.h>

#include "../../src/common/shared.h"
#include "../../src/game/parser.h"
