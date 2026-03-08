/*
 * test_harness.h — Minimal C test harness for openwarcraft3 unit tests.
 *
 * Usage:
 *   #include "test_harness.h"
 *
 *   int main(void) {
 *       ASSERT(1 + 1 == 2);
 *       ASSERT_FLOAT_EQ(0.1f + 0.2f, 0.3f);
 *       ASSERT_STR_EQ("hello", "hello");
 *       TEST_SUMMARY();
 *       return TEST_RESULT();
 *   }
 */
#ifndef test_harness_h
#define test_harness_h

#include <stdio.h>
#include <math.h>
#include <string.h>

static int tests_run    = 0;
static int tests_passed = 0;
static int tests_failed = 0;

/* Assert a boolean expression. */
#define ASSERT(expr) \
    do { \
        tests_run++; \
        if (expr) { \
            tests_passed++; \
        } else { \
            tests_failed++; \
            fprintf(stderr, "FAIL: %s:%d: %s\n", __FILE__, __LINE__, #expr); \
        } \
    } while (0)

/* Assert two floats are equal within a small tolerance. */
#define ASSERT_FLOAT_EQ(a, b) \
    do { \
        tests_run++; \
        float _a = (float)(a), _b = (float)(b); \
        if (fabsf(_a - _b) < 1e-5f) { \
            tests_passed++; \
        } else { \
            tests_failed++; \
            fprintf(stderr, "FAIL: %s:%d: " #a " ~= " #b \
                    " (got %.7f, expected %.7f)\n", \
                    __FILE__, __LINE__, (double)_a, (double)_b); \
        } \
    } while (0)

/* Assert two C strings are equal. */
#define ASSERT_STR_EQ(a, b) \
    do { \
        tests_run++; \
        const char *_a = (a), *_b = (b); \
        if (_a && _b && strcmp(_a, _b) == 0) { \
            tests_passed++; \
        } else { \
            tests_failed++; \
            fprintf(stderr, "FAIL: %s:%d: " #a " == " #b \
                    " (got \"%s\", expected \"%s\")\n", \
                    __FILE__, __LINE__, \
                    _a ? _a : "(null)", _b ? _b : "(null)"); \
        } \
    } while (0)

/* Print a summary line and set the exit code. */
#define TEST_SUMMARY() \
    do { \
        if (tests_failed == 0) { \
            printf("OK   %s: %d/%d tests passed\n", \
                   __FILE__, tests_passed, tests_run); \
        } else { \
            printf("FAIL %s: %d/%d tests passed (%d failed)\n", \
                   __FILE__, tests_passed, tests_run, tests_failed); \
        } \
    } while (0)

/* Return 0 on success, 1 on any failure — suitable for main(). */
#define TEST_RESULT() (tests_failed ? 1 : 0)

#endif /* test_harness_h */
