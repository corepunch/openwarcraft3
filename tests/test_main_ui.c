/*
 * test_main_ui.c — UI-focused test runner.
 *
 * This is the enforcement runner for UI parser/layout/serialization/sprite
 * paths and is intended for CI gates on UI-impacting changes.
 */

#include <stdio.h>
#include "test_framework.h"

int _tests_run = 0;
int _tests_failed = 0;

void run_ui_fdf_tests(void);
void run_ui_serialize_tests(void);
void run_ui_layout_tests(void);
void run_ui_e2e_tests(void);
void run_ui_oracle_tests(void);
void run_tool_common_tests(void);

int main(void) {
    printf("=== OpenWarcraft3 UI Test Gate ===\n\n");

    printf("[UI FDF parser / frame graph]\n");
    run_ui_fdf_tests();
    printf("\n");

    printf("[UI serialization / delta]\n");
    run_ui_serialize_tests();
    printf("\n");

    printf("[UI layout solver]\n");
    run_ui_layout_tests();
    printf("\n");

    printf("[UI end-to-end server->client]\n");
    run_ui_e2e_tests();
    printf("\n");

    printf("[UI tool-backed oracle]\n");
    run_ui_oracle_tests();
    printf("\n");

    printf("[Tool common helpers]\n");
    run_tool_common_tests();
    printf("\n");

    TEST_RESULTS();
}
