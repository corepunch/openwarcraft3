/*
 * test_main.c — Top-level test runner.
 *
 * Calls each suite's runner function in turn and exits with a non-zero
 * status code if any assertion failed.
 *
 * _tests_run and _tests_failed are defined here and extern-declared in
 * test_framework.h so all test translation units share a single counter.
 */

#include <stdio.h>
#include "test_framework.h"

/* Global assertion counters — shared across all test files. */
int _tests_run    = 0;
int _tests_failed = 0;

/* Forward declarations — each test file defines one of these. */
void run_slk_tests(void);
void run_unit_tests(void);
void run_movement_tests(void);
void run_collision_tests(void);
void run_net_tests(void);
void run_jass_tests(void);
void run_api_tests(void);
void run_game_tests(void);
void run_combat_tests(void);

int main(void) {
    printf("=== OpenWarcraft3 Unit Tests ===\n\n");

    printf("[SLK data / unit stats]\n");
    run_slk_tests();
    printf("\n");

    printf("[Unit lifecycle]\n");
    run_unit_tests();
    printf("\n");

    printf("[Unit movement / pathfinding]\n");
    run_movement_tests();
    printf("\n");

    printf("[Collision resolution / TGA loader]\n");
    run_collision_tests();
    printf("\n");

    printf("[Network layer / loopback / address parsing]\n");
    run_net_tests();
    printf("\n");

    printf("[JASS interpreter]\n");
    run_jass_tests();
    printf("\n");

    printf("[JASS native API]\n");
    run_api_tests();
    printf("\n");

    printf("[Game utilities]\n");
    run_game_tests();
    printf("\n");

    printf("[Combat, animation, ability, resources]\n");
    run_combat_tests();
    printf("\n");

    TEST_RESULTS();
}
