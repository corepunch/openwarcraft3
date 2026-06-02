/*
 * test_commands_main.c — focused console / filesystem command test runner.
 */

#include <stdio.h>

#include "test_framework.h"

int _tests_run    = 0;
int _tests_failed = 0;

void run_command_tests(void);

int main(void) {
    printf("=== OpenWarcraft3 Command Tests ===\n\n");

    printf("[Console commands / map FS]\n");
    run_command_tests();
    printf("\n");

    TEST_RESULTS();
}
