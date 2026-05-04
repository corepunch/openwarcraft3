/*
 * test_jass.c — Tests for the JASS interpreter (vm_main.c / jass_parser.c).
 *
 * Tests are self-contained: each one calls jass_newstate(), runs a snippet
 * of JASS via jass_dobuffer() or jass_callbyname(), then inspects the stack
 * or the result of a call.  No MPQ, renderer, or StormLib is needed.
 *
 * The minimal jass_funcs[] table at the bottom of this file exposes just
 * enough native functions to exercise native-call paths without linking
 * the full api_module.c dependency tree.
 */

#include "test_framework.h"
#include "test_harness.h"

/* Pull in the JASS public API.  vm_main.c is compiled into the test binary. */
#include "../src/game/jass/vm_public.h"
#include "../src/game/jass/jass_parser.h"

/* -------------------------------------------------------------------------
 * Minimal jass_funcs table (replaces api_module.c for the test binary).
 * Each entry wires a JASS native name to a C function.
 * We expose only what the tests actually call.
 * --------------------------------------------------------------------- */

static DWORD native_Add(LPJASS j) {
    return jass_pushinteger(j, jass_checkinteger(j, 1) + jass_checkinteger(j, 2));
}

static DWORD native_Mul(LPJASS j) {
    return jass_pushinteger(j, jass_checkinteger(j, 1) * jass_checkinteger(j, 2));
}

static DWORD native_Negate(LPJASS j) {
    return jass_pushinteger(j, -jass_checkinteger(j, 1));
}

/* jass_runevents is declared in vm_public.h but not yet implemented in the
 * game source — provide a no-op stub here as a temporary workaround for
 * the test binary. */
void jass_runevents(LPJASS j) { (void)j; }

JASSMODULE jass_funcs[] = {
    { "NativeAdd",    native_Add    },
    { "NativeMul",    native_Mul    },
    { "NativeNegate", native_Negate },
    { NULL }
};

/* -------------------------------------------------------------------------
 * G_GetPlayerByNumber stub — vm_main.c calls this for GetLocalPlayer
 * branches inside IF tokens.  We don't use GetLocalPlayer in our test
 * snippets, but the linker still needs the symbol.
 * --------------------------------------------------------------------- */
/* Already provided by test_harness.c stubs + g_main.c via TEST_GAME_SRCS */

/* =========================================================================
 * Helper: create a fresh JASS state with TextRemoveComments / TextRemoveBom
 * pointing to no-ops (already wired by the test harness).
 * ========================================================================= */

static LPJASS make_jass(void) {
    return jass_newstate();
}

/*
 * Run a JASS snippet and return the state.  The state must be freed with
 * jass_close() after assertions.
 */
static LPJASS run(LPCSTR src) {
    LPJASS j = make_jass();
    /* jass_dobuffer modifies the buffer in place, so we need a writable copy */
    size_t len = strlen(src);
    char *buf = gi.MemAlloc((long)len + 1);
    memcpy(buf, src, len + 1);
    jass_dobuffer(j, buf);
    gi.MemFree(buf);
    return j;
}

/* =========================================================================
 * Integer arithmetic
 * ========================================================================= */

static void test_integer_literal(void) {
    const char *src =
        "globals\n"
        "  integer g = 7\n"
        "endglobals\n"
        "function GetG takes nothing returns integer\n"
        "  return g\n"
        "endfunction\n";
    LPJASS j = run(src);
    jass_callbyname(j, "GetG", false);
    ASSERT_EQ_INT(jass_checkinteger(j, -1), 7);
    jass_close(j);
}

static void test_integer_addition(void) {
    const char *src =
        "function Add takes nothing returns integer\n"
        "  local integer a = 3\n"
        "  local integer b = 4\n"
        "  return a + b\n"
        "endfunction\n";
    LPJASS j = run(src);
    jass_callbyname(j, "Add", false);
    ASSERT_EQ_INT(jass_checkinteger(j, -1), 7);
    jass_close(j);
}

static void test_integer_subtraction(void) {
    const char *src =
        "function Sub takes nothing returns integer\n"
        "  local integer a = 10\n"
        "  local integer b = 3\n"
        "  return a - b\n"
        "endfunction\n";
    LPJASS j = run(src);
    jass_callbyname(j, "Sub", false);
    ASSERT_EQ_INT(jass_checkinteger(j, -1), 7);
    jass_close(j);
}

static void test_integer_multiplication(void) {
    const char *src =
        "function Mul takes nothing returns integer\n"
        "  local integer a = 6\n"
        "  local integer b = 7\n"
        "  return a * b\n"
        "endfunction\n";
    LPJASS j = run(src);
    jass_callbyname(j, "Mul", false);
    ASSERT_EQ_INT(jass_checkinteger(j, -1), 42);
    jass_close(j);
}

static void test_integer_division(void) {
    const char *src =
        "function Div takes nothing returns integer\n"
        "  local integer a = 20\n"
        "  local integer b = 4\n"
        "  return a / b\n"
        "endfunction\n";
    LPJASS j = run(src);
    jass_callbyname(j, "Div", false);
    ASSERT_EQ_INT(jass_checkinteger(j, -1), 5);
    jass_close(j);
}

static void test_integer_unary_minus(void) {
    const char *src =
        "function Neg takes nothing returns integer\n"
        "  local integer a = 9\n"
        "  return -a\n"
        "endfunction\n";
    LPJASS j = run(src);
    jass_callbyname(j, "Neg", false);
    ASSERT_EQ_INT(jass_checkinteger(j, -1), -9);
    jass_close(j);
}

/* =========================================================================
 * Real (float) arithmetic
 * ========================================================================= */

static void test_real_addition(void) {
    const char *src =
        "function RAdd takes nothing returns real\n"
        "  local real a = 1.5\n"
        "  local real b = 2.5\n"
        "  return a + b\n"
        "endfunction\n";
    LPJASS j = run(src);
    jass_callbyname(j, "RAdd", false);
    ASSERT_EQ_FLOAT(jass_checknumber(j, -1), 4.0f, 0.001f);
    jass_close(j);
}

static void test_real_multiplication(void) {
    const char *src =
        "function RMul takes nothing returns real\n"
        "  local real a = 2.0\n"
        "  local real b = 3.5\n"
        "  return a * b\n"
        "endfunction\n";
    LPJASS j = run(src);
    jass_callbyname(j, "RMul", false);
    ASSERT_EQ_FLOAT(jass_checknumber(j, -1), 7.0f, 0.001f);
    jass_close(j);
}

/* =========================================================================
 * Boolean logic
 * ========================================================================= */

static void test_boolean_true_literal(void) {
    const char *src =
        "function GetTrue takes nothing returns boolean\n"
        "  return true\n"
        "endfunction\n";
    LPJASS j = run(src);
    jass_callbyname(j, "GetTrue", false);
    ASSERT(jass_toboolean(j, -1));
    jass_close(j);
}

static void test_boolean_false_literal(void) {
    const char *src =
        "function GetFalse takes nothing returns boolean\n"
        "  return false\n"
        "endfunction\n";
    LPJASS j = run(src);
    jass_callbyname(j, "GetFalse", false);
    ASSERT(!jass_toboolean(j, -1));
    jass_close(j);
}

static void test_boolean_and(void) {
    const char *src =
        "function AndTest takes nothing returns boolean\n"
        "  return true and false\n"
        "endfunction\n";
    LPJASS j = run(src);
    jass_callbyname(j, "AndTest", false);
    ASSERT(!jass_toboolean(j, -1));
    jass_close(j);
}

static void test_boolean_or(void) {
    const char *src =
        "function OrTest takes nothing returns boolean\n"
        "  return false or true\n"
        "endfunction\n";
    LPJASS j = run(src);
    jass_callbyname(j, "OrTest", false);
    ASSERT(jass_toboolean(j, -1));
    jass_close(j);
}

static void test_boolean_not(void) {
    const char *src =
        "function NotTest takes nothing returns boolean\n"
        "  return not true\n"
        "endfunction\n";
    LPJASS j = run(src);
    jass_callbyname(j, "NotTest", false);
    ASSERT(!jass_toboolean(j, -1));
    jass_close(j);
}

/* =========================================================================
 * Comparison operators
 * ========================================================================= */

static void test_compare_equal_true(void) {
    const char *src =
        "function EqTrue takes nothing returns boolean\n"
        "  return 5 == 5\n"
        "endfunction\n";
    LPJASS j = run(src);
    jass_callbyname(j, "EqTrue", false);
    ASSERT(jass_toboolean(j, -1));
    jass_close(j);
}

static void test_compare_equal_false(void) {
    const char *src =
        "function EqFalse takes nothing returns boolean\n"
        "  return 5 == 6\n"
        "endfunction\n";
    LPJASS j = run(src);
    jass_callbyname(j, "EqFalse", false);
    ASSERT(!jass_toboolean(j, -1));
    jass_close(j);
}

static void test_compare_not_equal(void) {
    const char *src =
        "function NeTest takes nothing returns boolean\n"
        "  return 3 != 4\n"
        "endfunction\n";
    LPJASS j = run(src);
    jass_callbyname(j, "NeTest", false);
    ASSERT(jass_toboolean(j, -1));
    jass_close(j);
}

static void test_compare_less_than(void) {
    const char *src =
        "function LtTest takes nothing returns boolean\n"
        "  return 3 < 4\n"
        "endfunction\n";
    LPJASS j = run(src);
    jass_callbyname(j, "LtTest", false);
    ASSERT(jass_toboolean(j, -1));
    jass_close(j);
}

static void test_compare_greater_than(void) {
    const char *src =
        "function GtTest takes nothing returns boolean\n"
        "  return 5 > 2\n"
        "endfunction\n";
    LPJASS j = run(src);
    jass_callbyname(j, "GtTest", false);
    ASSERT(jass_toboolean(j, -1));
    jass_close(j);
}

static void test_compare_less_equal(void) {
    const char *src =
        "function LeTest takes nothing returns boolean\n"
        "  local integer a = 4\n"
        "  return a <= 4\n"
        "endfunction\n";
    LPJASS j = run(src);
    jass_callbyname(j, "LeTest", false);
    ASSERT(jass_toboolean(j, -1));
    jass_close(j);
}

static void test_compare_greater_equal(void) {
    const char *src =
        "function GeTest takes nothing returns boolean\n"
        "  local integer a = 5\n"
        "  return a >= 5\n"
        "endfunction\n";
    LPJASS j = run(src);
    jass_callbyname(j, "GeTest", false);
    ASSERT(jass_toboolean(j, -1));
    jass_close(j);
}

/* =========================================================================
 * Control flow: if / else
 * ========================================================================= */

static void test_if_taken_branch(void) {
    const char *src =
        "globals\n"
        "  integer g_result = 0\n"
        "endglobals\n"
        "function IfTest takes nothing returns nothing\n"
        "  if true then\n"
        "    set g_result = 1\n"
        "  endif\n"
        "endfunction\n"
        "function GetResult takes nothing returns integer\n"
        "  return g_result\n"
        "endfunction\n";
    LPJASS j = run(src);
    jass_callbyname(j, "IfTest", false);
    jass_callbyname(j, "GetResult", false);
    ASSERT_EQ_INT(jass_checkinteger(j, -1), 1);
    jass_close(j);
}

static void test_if_not_taken_branch(void) {
    const char *src =
        "globals\n"
        "  integer g_r2 = 99\n"
        "endglobals\n"
        "function IfNotTaken takes nothing returns nothing\n"
        "  if false then\n"
        "    set g_r2 = 0\n"
        "  endif\n"
        "endfunction\n"
        "function GetR2 takes nothing returns integer\n"
        "  return g_r2\n"
        "endfunction\n";
    LPJASS j = run(src);
    jass_callbyname(j, "IfNotTaken", false);
    jass_callbyname(j, "GetR2", false);
    ASSERT_EQ_INT(jass_checkinteger(j, -1), 99);
    jass_close(j);
}

static void test_if_else(void) {
    const char *src =
        "globals\n"
        "  integer g_else = 0\n"
        "endglobals\n"
        "function ElseTest takes nothing returns nothing\n"
        "  if false then\n"
        "    set g_else = 1\n"
        "  else\n"
        "    set g_else = 2\n"
        "  endif\n"
        "endfunction\n"
        "function GetElse takes nothing returns integer\n"
        "  return g_else\n"
        "endfunction\n";
    LPJASS j = run(src);
    jass_callbyname(j, "ElseTest", false);
    jass_callbyname(j, "GetElse", false);
    ASSERT_EQ_INT(jass_checkinteger(j, -1), 2);
    jass_close(j);
}

/* =========================================================================
 * Control flow: loop / exitwhen
 * ========================================================================= */

static void test_loop_exitwhen(void) {
    const char *src =
        "function LoopTest takes nothing returns integer\n"
        "  local integer i = 0\n"
        "  loop\n"
        "    set i = i + 1\n"
        "    exitwhen i >= 5\n"
        "  endloop\n"
        "  return i\n"
        "endfunction\n";
    LPJASS j = run(src);
    jass_callbyname(j, "LoopTest", false);
    ASSERT_EQ_INT(jass_checkinteger(j, -1), 5);
    jass_close(j);
}

static void test_loop_sum(void) {
    const char *src =
        "function SumTo5 takes nothing returns integer\n"
        "  local integer i = 1\n"
        "  local integer s = 0\n"
        "  loop\n"
        "    set s = s + i\n"
        "    set i = i + 1\n"
        "    exitwhen i > 5\n"
        "  endloop\n"
        "  return s\n"
        "endfunction\n";
    LPJASS j = run(src);
    jass_callbyname(j, "SumTo5", false);
    ASSERT_EQ_INT(jass_checkinteger(j, -1), 15);
    jass_close(j);
}

/* =========================================================================
 * Local variables
 * ========================================================================= */

static void test_local_variable_set(void) {
    const char *src =
        "function LocalTest takes nothing returns integer\n"
        "  local integer x = 0\n"
        "  set x = 42\n"
        "  return x\n"
        "endfunction\n";
    LPJASS j = run(src);
    jass_callbyname(j, "LocalTest", false);
    ASSERT_EQ_INT(jass_checkinteger(j, -1), 42);
    jass_close(j);
}

/* =========================================================================
 * Global variables
 * ========================================================================= */

static void test_global_variable(void) {
    const char *src =
        "globals\n"
        "  integer g_val = 100\n"
        "endglobals\n"
        "function GetGlobal takes nothing returns integer\n"
        "  return g_val\n"
        "endfunction\n";
    LPJASS j = run(src);
    jass_callbyname(j, "GetGlobal", false);
    ASSERT_EQ_INT(jass_checkinteger(j, -1), 100);
    jass_close(j);
}

static void test_global_variable_mutation(void) {
    const char *src =
        "globals\n"
        "  integer g_mut = 1\n"
        "endglobals\n"
        "function MutGlobal takes nothing returns nothing\n"
        "  set g_mut = g_mut * 3\n"
        "endfunction\n"
        "function GetMut takes nothing returns integer\n"
        "  return g_mut\n"
        "endfunction\n";
    LPJASS j = run(src);
    jass_callbyname(j, "MutGlobal", false);
    jass_callbyname(j, "GetMut", false);
    ASSERT_EQ_INT(jass_checkinteger(j, -1), 3);
    jass_close(j);
}

/* =========================================================================
 * Function arguments
 * ========================================================================= */

static void test_function_with_args(void) {
    const char *src =
        "function Square takes integer n returns integer\n"
        "  return n * n\n"
        "endfunction\n"
        "function CallSquare takes nothing returns integer\n"
        "  return Square(7)\n"
        "endfunction\n";
    LPJASS j = run(src);
    jass_callbyname(j, "CallSquare", false);
    ASSERT_EQ_INT(jass_checkinteger(j, -1), 49);
    jass_close(j);
}

static void test_function_two_args(void) {
    const char *src =
        "function Max2 takes integer a, integer b returns integer\n"
        "  if a > b then\n"
        "    return a\n"
        "  endif\n"
        "  return b\n"
        "endfunction\n"
        "function CallMax takes nothing returns integer\n"
        "  return Max2(3, 8)\n"
        "endfunction\n";
    LPJASS j = run(src);
    jass_callbyname(j, "CallMax", false);
    ASSERT_EQ_INT(jass_checkinteger(j, -1), 8);
    jass_close(j);
}

/* =========================================================================
 * Recursive function call
 * ========================================================================= */

static void test_recursive_factorial(void) {
    const char *src =
        "function Fact takes integer n returns integer\n"
        "  if n <= 1 then\n"
        "    return 1\n"
        "  endif\n"
        "  return n * Fact(n - 1)\n"
        "endfunction\n"
        "function RunFact takes nothing returns integer\n"
        "  return Fact(5)\n"
        "endfunction\n";
    LPJASS j = run(src);
    jass_callbyname(j, "RunFact", false);
    ASSERT_EQ_INT(jass_checkinteger(j, -1), 120);
    jass_close(j);
}

/* =========================================================================
 * Array variables
 * ========================================================================= */

static void test_array_read_write(void) {
    const char *src =
        "globals\n"
        "  integer array arr\n"
        "endglobals\n"
        "function SetArr takes nothing returns nothing\n"
        "  set arr[0] = 10\n"
        "  set arr[1] = 20\n"
        "  set arr[2] = 30\n"
        "endfunction\n"
        "function GetArr takes nothing returns integer\n"
        "  return arr[1]\n"
        "endfunction\n";
    LPJASS j = run(src);
    jass_callbyname(j, "SetArr", false);
    jass_callbyname(j, "GetArr", false);
    ASSERT_EQ_INT(jass_checkinteger(j, -1), 20);
    jass_close(j);
}

/* =========================================================================
 * Native function call
 * ========================================================================= */

static void test_native_call(void) {
    const char *src =
        "native NativeAdd takes integer a, integer b returns integer\n"
        "function TestNative takes nothing returns integer\n"
        "  return NativeAdd(6, 7)\n"
        "endfunction\n";
    LPJASS j = run(src);
    jass_callbyname(j, "TestNative", false);
    ASSERT_EQ_INT(jass_checkinteger(j, -1), 13);
    jass_close(j);
}

static void test_native_call_chain(void) {
    const char *src =
        "native NativeAdd takes integer a, integer b returns integer\n"
        "native NativeMul takes integer a, integer b returns integer\n"
        "function TestChain takes nothing returns integer\n"
        "  return NativeMul(NativeAdd(1, 2), NativeAdd(3, 4))\n"
        "endfunction\n";
    LPJASS j = run(src);
    jass_callbyname(j, "TestChain", false);
    /* (1+2) * (3+4) = 3 * 7 = 21 */
    ASSERT_EQ_INT(jass_checkinteger(j, -1), 21);
    jass_close(j);
}

/* =========================================================================
 * Type system: typedef / handle types
 * ========================================================================= */

static void test_typedef(void) {
    /* Verify that a user-defined handle type can be declared and used
     * as a null handle without crashing.  The handle is opaque — we only
     * check that parsing succeeds and a null is returned. */
    const char *src =
        "type myhandle extends handle\n"
        "function GetHandle takes nothing returns myhandle\n"
        "  return null\n"
        "endfunction\n";
    LPJASS j = run(src);
    jass_callbyname(j, "GetHandle", false);
    /* The top of the stack should be a null handle — value pointer is NULL. */
    ASSERT(jass_gettype(j, -1) == jasstype_handle);
    jass_close(j);
}

/* =========================================================================
 * Stack: push / check / pop primitives
 * ========================================================================= */

static void test_push_pop_integer(void) {
    LPJASS j = make_jass();
    jass_pushinteger(j, 123);
    ASSERT_EQ_INT(jass_checkinteger(j, -1), 123);
    jass_pop(j, 1);
    jass_close(j);
}

static void test_push_pop_real(void) {
    LPJASS j = make_jass();
    jass_pushnumber(j, 3.14f);
    ASSERT_EQ_FLOAT(jass_checknumber(j, -1), 3.14f, 0.001f);
    jass_pop(j, 1);
    jass_close(j);
}

static void test_push_pop_boolean(void) {
    LPJASS j = make_jass();
    jass_pushboolean(j, 1);
    ASSERT(jass_checkboolean(j, -1));
    jass_pop(j, 1);
    jass_close(j);
}

static void test_push_pop_string(void) {
    LPJASS j = make_jass();
    jass_pushstring(j, "hello");
    ASSERT_STR_EQ(jass_checkstring(j, -1), "hello");
    jass_pop(j, 1);
    jass_close(j);
}

static void test_push_null_handle(void) {
    LPJASS j = make_jass();
    jass_pushnull(j);
    ASSERT(jass_gettype(j, -1) == jasstype_handle);
    jass_pop(j, 1);
    jass_close(j);
}

/* =========================================================================
 * toboolean coercion
 * ========================================================================= */

static void test_toboolean_integer_nonzero(void) {
    LPJASS j = make_jass();
    jass_pushinteger(j, 5);
    ASSERT(jass_toboolean(j, -1));
    jass_pop(j, 1);
    jass_close(j);
}

static void test_toboolean_integer_zero(void) {
    LPJASS j = make_jass();
    jass_pushinteger(j, 0);
    ASSERT(!jass_toboolean(j, -1));
    jass_pop(j, 1);
    jass_close(j);
}

static void test_toboolean_real_nonzero(void) {
    LPJASS j = make_jass();
    jass_pushnumber(j, 1.0f);
    ASSERT(jass_toboolean(j, -1));
    jass_pop(j, 1);
    jass_close(j);
}

static void test_toboolean_string_nonempty(void) {
    LPJASS j = make_jass();
    jass_pushstring(j, "x");
    ASSERT(jass_toboolean(j, -1));
    jass_pop(j, 1);
    jass_close(j);
}

/* =========================================================================
 * Test suite entry point
 * ========================================================================= */

BEGIN_SUITE(jass)
    /* Stack primitives */
    RUN_TEST(test_push_pop_integer);
    RUN_TEST(test_push_pop_real);
    RUN_TEST(test_push_pop_boolean);
    RUN_TEST(test_push_pop_string);
    RUN_TEST(test_push_null_handle);
    /* toboolean coercions */
    RUN_TEST(test_toboolean_integer_nonzero);
    RUN_TEST(test_toboolean_integer_zero);
    RUN_TEST(test_toboolean_real_nonzero);
    RUN_TEST(test_toboolean_string_nonempty);
    /* Integer arithmetic */
    RUN_TEST(test_integer_literal);
    RUN_TEST(test_integer_addition);
    RUN_TEST(test_integer_subtraction);
    RUN_TEST(test_integer_multiplication);
    RUN_TEST(test_integer_division);
    RUN_TEST(test_integer_unary_minus);
    /* Real arithmetic */
    RUN_TEST(test_real_addition);
    RUN_TEST(test_real_multiplication);
    /* Boolean logic */
    RUN_TEST(test_boolean_true_literal);
    RUN_TEST(test_boolean_false_literal);
    RUN_TEST(test_boolean_and);
    RUN_TEST(test_boolean_or);
    RUN_TEST(test_boolean_not);
    /* Comparisons */
    RUN_TEST(test_compare_equal_true);
    RUN_TEST(test_compare_equal_false);
    RUN_TEST(test_compare_not_equal);
    RUN_TEST(test_compare_less_than);
    RUN_TEST(test_compare_greater_than);
    RUN_TEST(test_compare_less_equal);
    RUN_TEST(test_compare_greater_equal);
    /* Control flow */
    RUN_TEST(test_if_taken_branch);
    RUN_TEST(test_if_not_taken_branch);
    RUN_TEST(test_if_else);
    RUN_TEST(test_loop_exitwhen);
    RUN_TEST(test_loop_sum);
    /* Variables */
    RUN_TEST(test_local_variable_set);
    RUN_TEST(test_global_variable);
    RUN_TEST(test_global_variable_mutation);
    /* Functions */
    RUN_TEST(test_function_with_args);
    RUN_TEST(test_function_two_args);
    RUN_TEST(test_recursive_factorial);
    /* Arrays */
    RUN_TEST(test_array_read_write);
    /* Natives */
    RUN_TEST(test_native_call);
    RUN_TEST(test_native_call_chain);
    /* Types */
    RUN_TEST(test_typedef);
END_SUITE()
