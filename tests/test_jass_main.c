#include <stdio.h>

#include "test_framework.h"

int _tests_run = 0;
int _tests_failed = 0;

void run_jass_tests(void);

void InitAbilities(void) {
    /* JASS VM tests do not need the game ability registry. */
}

int main(void) {
    run_jass_tests();
    printf("%d assertions, %d failed\n", _tests_run, _tests_failed);
    return _tests_failed ? 1 : 0;
}
