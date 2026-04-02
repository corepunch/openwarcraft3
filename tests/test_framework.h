/*
 * test_framework.h — Minimal, dependency-free test macros.
 *
 * _tests_run and _tests_failed are defined in test_main.c and
 * extern-declared here so all translation units share one counter.
 *
 * Usage:
 *   ASSERT(condition)          — fail if condition is false
 *   ASSERT_EQ_INT(a, b)        — fail if integers differ
 *   ASSERT_EQ_FLOAT(a, b, eps) — fail if floats differ by more than eps
 *   ASSERT_STR_EQ(a, b)        — fail if strings differ
 *   ASSERT_NULL(ptr)           — fail if ptr is not NULL
 *   ASSERT_NOT_NULL(ptr)       — fail if ptr is NULL
 *   RUN_TEST(fn)               — call fn(), report pass/fail
 *   TEST_RESULTS()             — print summary, return exit code
 */
#ifndef test_framework_h
#define test_framework_h

#include <stdio.h>
#include <string.h>
#include <math.h>

/* Defined once in test_main.c; all other translation units see them via
 * this extern declaration. */
extern int _tests_run;
extern int _tests_failed;

#define ASSERT(cond) do { \
    _tests_run++; \
    if (!(cond)) { \
        fprintf(stderr, "    FAIL [%s:%d]: %s\n", __FILE__, __LINE__, #cond); \
        _tests_failed++; \
    } \
} while (0)

#define ASSERT_EQ_INT(a, b) do { \
    _tests_run++; \
    long long _a = (long long)(a), _b = (long long)(b); \
    if (_a != _b) { \
        fprintf(stderr, "    FAIL [%s:%d]: %s == %s  (%lld != %lld)\n", \
                __FILE__, __LINE__, #a, #b, _a, _b); \
        _tests_failed++; \
    } \
} while (0)

#define ASSERT_EQ_FLOAT(a, b, eps) do { \
    _tests_run++; \
    float _a = (float)(a), _b = (float)(b); \
    if (fabsf(_a - _b) > (float)(eps)) { \
        fprintf(stderr, "    FAIL [%s:%d]: %s == %s  (%f != %f)\n", \
                __FILE__, __LINE__, #a, #b, (double)_a, (double)_b); \
        _tests_failed++; \
    } \
} while (0)

#define ASSERT_STR_EQ(a, b) do { \
    _tests_run++; \
    const char *_a = (a), *_b = (b); \
    if (!_a || !_b || strcmp(_a, _b) != 0) { \
        fprintf(stderr, "    FAIL [%s:%d]: %s == %s  (\"%s\" != \"%s\")\n", \
                __FILE__, __LINE__, #a, #b, \
                _a ? _a : "(null)", _b ? _b : "(null)"); \
        _tests_failed++; \
    } \
} while (0)

#define ASSERT_NULL(ptr) do { \
    _tests_run++; \
    if ((ptr) != NULL) { \
        fprintf(stderr, "    FAIL [%s:%d]: %s expected NULL\n", \
                __FILE__, __LINE__, #ptr); \
        _tests_failed++; \
    } \
} while (0)

#define ASSERT_NOT_NULL(ptr) do { \
    _tests_run++; \
    if ((ptr) == NULL) { \
        fprintf(stderr, "    FAIL [%s:%d]: %s is NULL\n", \
                __FILE__, __LINE__, #ptr); \
        _tests_failed++; \
    } \
} while (0)

/* Run a single test function and report pass/fail on stdout. */
#define RUN_TEST(fn) do { \
    int _before = _tests_failed; \
    printf("  %-52s", #fn " ..."); \
    fflush(stdout); \
    fn(); \
    printf("%s\n", (_tests_failed == _before) ? "PASS" : "FAIL"); \
} while (0)

/* Print summary and return 0 (success) or 1 (failure) as main() exit code. */
#define TEST_RESULTS() do { \
    int _passed = _tests_run - _tests_failed; \
    printf("\n%d/%d assertions passed", _passed, _tests_run); \
    if (_tests_failed) printf(", %d failed", _tests_failed); \
    printf("\n"); \
    return (_tests_failed > 0) ? 1 : 0; \
} while (0)

#endif /* test_framework_h */
