// test_jass_assertions.j — JASS-side tests for BJassAssert / BJassError.
//
// Run via:
//   openwarcraft3 -data=<dir> -jass_test=tests/fixtures/test_jass_assertions.j
//
// The entrypoint is run_tests (default for -jass_test mode).
// Each helper function name starts with test_ and calls BJassAssert to verify
// conditions.  If any assertion fails the coroutine aborts and the runner
// exits non-zero.

native BJassAssert  takes boolean cond, string msg returns nothing
native BJassError   takes string msg returns nothing
native ExecuteFunc  takes string funcName returns nothing

// ---------------------------------------------------------------------------
// Basic assertion primitives
// ---------------------------------------------------------------------------

function test_assert_true takes nothing returns nothing
    call BJassAssert(true, "true should pass")
endfunction

function test_assert_arithmetic takes nothing returns nothing
    local integer a = 3
    local integer b = 4
    call BJassAssert(a + b == 7, "3 + 4 == 7")
endfunction

function test_assert_string_equality takes nothing returns nothing
    local string s = "hello"
    call BJassAssert(s == "hello", "string equality")
endfunction

function test_assert_boolean_logic takes nothing returns nothing
    call BJassAssert(true and true,  "true and true")
    call BJassAssert(not false,       "not false")
    call BJassAssert(false or true,   "false or true")
endfunction

function test_assert_real_arithmetic takes nothing returns nothing
    local real a = 1.5
    local real b = 2.5
    // JASS reals use == which is exact for these values
    call BJassAssert(a + b == 4.0, "1.5 + 2.5 == 4.0")
endfunction

function test_assert_comparison_operators takes nothing returns nothing
    call BJassAssert(1 < 2,  "1 < 2")
    call BJassAssert(2 > 1,  "2 > 1")
    call BJassAssert(2 >= 2, "2 >= 2")
    call BJassAssert(1 <= 1, "1 <= 1")
    call BJassAssert(1 != 2, "1 != 2")
endfunction

function test_assert_loop_count takes nothing returns nothing
    local integer sum = 0
    local integer i = 1
    loop
        exitwhen i > 5
        set sum = sum + i
        set i = i + 1
    endloop
    call BJassAssert(sum == 15, "sum 1..5 == 15")
endfunction

function test_assert_if_else takes nothing returns nothing
    local integer x = 10
    local string result = "wrong"
    if x > 5 then
        set result = "big"
    else
        set result = "small"
    endif
    call BJassAssert(result == "big", "x > 5 branch taken")
endfunction

// ---------------------------------------------------------------------------
// Coroutine / ExecuteFunc
// ---------------------------------------------------------------------------

globals
    integer g_co_ran = 0
endglobals

function helper_set_co_ran takes nothing returns nothing
    set g_co_ran = 1
endfunction

function test_execute_func_dispatch takes nothing returns nothing
    set g_co_ran = 0
    call ExecuteFunc("helper_set_co_ran")
    // ExecuteFunc creates a coroutine — it will run in the same event pump pass.
    // We cannot observe the result inline; the pump handles it.
    // The coroutine count test in C-side coverage verifies deeper scheduling.
endfunction

// ---------------------------------------------------------------------------
// BJassError / negative path (these must be called in isolation because they
// abort the current coroutine; they are not called from run_tests directly)
// ---------------------------------------------------------------------------

function test_bjerror_aborts takes nothing returns nothing
    call BJassError("intentional abort from test_bjerror_aborts")
    // unreachable — BJassError never returns
    call BJassAssert(false, "should not reach here")
endfunction

// ---------------------------------------------------------------------------
// Entry point — called by the in-game test runner
// ---------------------------------------------------------------------------

function run_tests takes nothing returns nothing
    call BJassAssert(true,  "sanity: assert(true) passes")
    call test_assert_true()
    call test_assert_arithmetic()
    call test_assert_string_equality()
    call test_assert_boolean_logic()
    call test_assert_real_arithmetic()
    call test_assert_comparison_operators()
    call test_assert_loop_count()
    call test_assert_if_else()
    call test_execute_func_dispatch()
endfunction
