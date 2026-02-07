/**
 * test_assert.h — Shared test assertion macro for all test files.
 *
 * Simple assert()-style macro: prints file:line on failure and returns 1.
 * Each test function returns 0 on success, 1 on failure.
 * main() runs all tests and returns 0 if all pass (ctest checks this).
 */

#ifndef TEST_ASSERT_H
#define TEST_ASSERT_H

#include <stdio.h>

/* Simple test assertion macro: prints file:line on failure and returns 1 */
#define TEST_ASSERT(condition, msg) \
    do { \
        if (!(condition)) { \
            printf("FAIL [%s:%d]: %s\n", __FILE__, __LINE__, msg); \
            return 1; \
        } \
    } while (0)

#endif /* TEST_ASSERT_H */
