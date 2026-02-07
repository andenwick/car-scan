/**
 * test_vin.c — Tests for VIN (Vehicle Identification Number) decoding.
 */

#include <obd/obd.h>
#include "test_assert.h"
#include "test_data.h"
#include <stdio.h>
#include <string.h>

/* ── Test: build VIN request ───────────────────────────────────────── */
static int test_build_request(void)
{
    char buf[OBD_MAX_COMMAND_LEN];
    obd_result_t r;

    r = obd_vin_build_request(buf, sizeof(buf));
    TEST_ASSERT(r == OBD_OK, "build should succeed");
    TEST_ASSERT(strcmp(buf, "0902\r") == 0, "should produce '0902\\r'");

    /* Buffer too small */
    r = obd_vin_build_request(buf, 3);
    TEST_ASSERT(r == OBD_ERROR_BUFFER_TOO_SMALL, "small buffer should error");

    printf("  PASS: build VIN request\n");
    return 0;
}

/* ── Test: parse multi-line VIN response ───────────────────────────── */
static int test_parse_vin(void)
{
    char vin[OBD_VIN_LENGTH + 1];
    obd_result_t r;

    r = obd_vin_parse_response(TEST_CLEAN_VIN_MULTILINE, vin, sizeof(vin));
    TEST_ASSERT(r == OBD_OK, "VIN parse should succeed");
    TEST_ASSERT(strlen(vin) == OBD_VIN_LENGTH, "VIN should be 17 characters");
    TEST_ASSERT(strcmp(vin, TEST_EXPECTED_VIN) == 0, "VIN should match expected");

    printf("  PASS: parse VIN '%s'\n", vin);
    return 0;
}

/* ── Test: error cases ─────────────────────────────────────────────── */
static int test_errors(void)
{
    char vin[OBD_VIN_LENGTH + 1];
    obd_result_t r;

    /* NULL input */
    r = obd_vin_parse_response(NULL, vin, sizeof(vin));
    TEST_ASSERT(r == OBD_ERROR_INVALID_ARG, "NULL should error");

    /* Buffer too small */
    r = obd_vin_parse_response(TEST_CLEAN_VIN_MULTILINE, vin, 5);
    TEST_ASSERT(r == OBD_ERROR_BUFFER_TOO_SMALL, "small buffer should error");

    /* Incomplete response (only 1 line, not enough for 17 chars) */
    r = obd_vin_parse_response("49 02 01 57 42 41 33", vin, sizeof(vin));
    TEST_ASSERT(r == OBD_ERROR_PARSE_FAILED, "incomplete VIN should fail");

    printf("  PASS: error cases\n");
    return 0;
}

int main(void)
{
    int failures = 0;

    printf("=== vin tests ===\n");
    failures += test_build_request();
    failures += test_parse_vin();
    failures += test_errors();

    printf("\n%s (%d test functions)\n",
           failures == 0 ? "ALL PASSED" : "SOME FAILED", 3);
    return failures;
}
