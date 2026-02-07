/**
 * test_elm327.c — Tests for ELM327 AT command and response handling.
 */

#include <obd/obd.h>
#include "test_assert.h"
#include "test_data.h"
#include <stdio.h>
#include <string.h>

/* ── Test: AT command strings ──────────────────────────────────────── */
static int test_at_commands(void)
{
    TEST_ASSERT(strcmp(obd_elm327_cmd_reset(), "ATZ\r") == 0,
                "reset should be ATZ\\r");
    TEST_ASSERT(strcmp(obd_elm327_cmd_echo_off(), "ATE0\r") == 0,
                "echo off should be ATE0\\r");
    TEST_ASSERT(strcmp(obd_elm327_cmd_linefeed_off(), "ATL0\r") == 0,
                "linefeed off should be ATL0\\r");
    TEST_ASSERT(strcmp(obd_elm327_cmd_protocol_auto(), "ATSP0\r") == 0,
                "protocol auto should be ATSP0\\r");
    TEST_ASSERT(strcmp(obd_elm327_cmd_headers_on(), "ATH1\r") == 0,
                "headers on should be ATH1\\r");
    TEST_ASSERT(strcmp(obd_elm327_cmd_headers_off(), "ATH0\r") == 0,
                "headers off should be ATH0\\r");

    printf("  PASS: AT command strings\n");
    return 0;
}

/* ── Test: response classification ─────────────────────────────────── */
static int test_classify_response(void)
{
    /* Data responses (start with hex bytes) */
    TEST_ASSERT(obd_elm327_classify_response("41 0C 1A F8") == OBD_ELM_RESPONSE_DATA,
                "hex data should be classified as DATA");
    TEST_ASSERT(obd_elm327_classify_response("43 01 03 01 04") == OBD_ELM_RESPONSE_DATA,
                "DTC response should be classified as DATA");
    TEST_ASSERT(obd_elm327_classify_response("49 02 01 57 42") == OBD_ELM_RESPONSE_DATA,
                "VIN response should be classified as DATA");

    /* OK responses */
    TEST_ASSERT(obd_elm327_classify_response("OK") == OBD_ELM_RESPONSE_OK,
                "OK should be classified as OK");
    TEST_ASSERT(obd_elm327_classify_response("ELM327 v1.5") == OBD_ELM_RESPONSE_OK,
                "ELM version should be classified as OK");

    /* NO DATA */
    TEST_ASSERT(obd_elm327_classify_response("NO DATA") == OBD_ELM_RESPONSE_NO_DATA,
                "NO DATA should be classified as NO_DATA");

    /* Errors */
    TEST_ASSERT(obd_elm327_classify_response("?") == OBD_ELM_RESPONSE_ERROR,
                "? should be classified as ERROR");
    TEST_ASSERT(obd_elm327_classify_response("ERROR") == OBD_ELM_RESPONSE_ERROR,
                "ERROR should be classified as ERROR");
    TEST_ASSERT(obd_elm327_classify_response("UNABLE TO CONNECT") == OBD_ELM_RESPONSE_ERROR,
                "UNABLE TO CONNECT should be classified as ERROR");
    TEST_ASSERT(obd_elm327_classify_response("STOPPED") == OBD_ELM_RESPONSE_ERROR,
                "STOPPED should be classified as ERROR");

    /* Prompt */
    TEST_ASSERT(obd_elm327_classify_response(">") == OBD_ELM_RESPONSE_PROMPT,
                "> should be classified as PROMPT");

    /* Unknown */
    TEST_ASSERT(obd_elm327_classify_response("") == OBD_ELM_RESPONSE_UNKNOWN,
                "empty string should be UNKNOWN");
    TEST_ASSERT(obd_elm327_classify_response(NULL) == OBD_ELM_RESPONSE_UNKNOWN,
                "NULL should be UNKNOWN");

    /* Leading whitespace should be skipped */
    TEST_ASSERT(obd_elm327_classify_response("  OK") == OBD_ELM_RESPONSE_OK,
                "OK with leading spaces should still be OK");
    TEST_ASSERT(obd_elm327_classify_response("\r\n41 0C") == OBD_ELM_RESPONSE_DATA,
                "data with leading \\r\\n should still be DATA");

    printf("  PASS: response classification\n");
    return 0;
}

/* ── Test: response cleaning — normal PID response ─────────────────── */
static int test_clean_response_normal(void)
{
    char out[OBD_MAX_RESPONSE_LEN];
    obd_result_t r;

    /* Raw RPM response with echo and prompt */
    r = obd_elm327_clean_response(TEST_RAW_RPM_RESPONSE, out, sizeof(out));
    TEST_ASSERT(r == OBD_OK, "should succeed for normal response");
    TEST_ASSERT(strcmp(out, TEST_CLEAN_RPM) == 0, "should extract '41 0C 1A F8'");

    /* Raw speed response */
    r = obd_elm327_clean_response(TEST_RAW_SPEED_RESPONSE, out, sizeof(out));
    TEST_ASSERT(r == OBD_OK, "should succeed for speed response");
    TEST_ASSERT(strcmp(out, TEST_CLEAN_SPEED) == 0, "should extract '41 0D 3C'");

    /* Raw coolant response */
    r = obd_elm327_clean_response(TEST_RAW_COOLANT_RESPONSE, out, sizeof(out));
    TEST_ASSERT(r == OBD_OK, "should succeed for coolant response");
    TEST_ASSERT(strcmp(out, TEST_CLEAN_COOLANT) == 0, "should extract '41 05 7B'");

    printf("  PASS: clean response — normal PID\n");
    return 0;
}

/* ── Test: response cleaning — error conditions ────────────────────── */
static int test_clean_response_errors(void)
{
    char out[OBD_MAX_RESPONSE_LEN];
    obd_result_t r;

    /* NO DATA response */
    r = obd_elm327_clean_response(TEST_RAW_NO_DATA_RESPONSE, out, sizeof(out));
    TEST_ASSERT(r == OBD_ERROR_NO_DATA, "should return NO_DATA error");

    /* Error response */
    r = obd_elm327_clean_response(TEST_RAW_ERROR_RESPONSE, out, sizeof(out));
    TEST_ASSERT(r == OBD_ERROR_ELM_ERROR, "should return ELM_ERROR");

    /* NULL input */
    r = obd_elm327_clean_response(NULL, out, sizeof(out));
    TEST_ASSERT(r == OBD_ERROR_INVALID_ARG, "NULL should return INVALID_ARG");

    printf("  PASS: clean response — error conditions\n");
    return 0;
}

/* ── Test: response cleaning — AT command OK response ──────────────── */
static int test_clean_response_ok(void)
{
    char out[OBD_MAX_RESPONSE_LEN];
    obd_result_t r;

    /* OK response has no data — should fail with PARSE_FAILED (no hex data found) */
    r = obd_elm327_clean_response(TEST_RAW_OK_RESPONSE, out, sizeof(out));
    TEST_ASSERT(r == OBD_ERROR_PARSE_FAILED, "OK response has no data to extract");

    printf("  PASS: clean response — AT OK\n");
    return 0;
}

/* ── Test: BUS INIT and CAN ERROR classification ──────────────────── */
static int test_classify_bus_errors(void)
{
    TEST_ASSERT(obd_elm327_classify_response("BUS INIT: ...ERROR") == OBD_ELM_RESPONSE_ERROR,
                "BUS INIT should be classified as ERROR");
    TEST_ASSERT(obd_elm327_classify_response("CAN ERROR") == OBD_ELM_RESPONSE_ERROR,
                "CAN ERROR should be classified as ERROR");

    printf("  PASS: BUS INIT and CAN ERROR classification\n");
    return 0;
}

/* ── Test: clean_response with buffer too small ───────────────────── */
static int test_clean_response_buffer_too_small(void)
{
    char out[4]; /* Too small to hold "41 0C 1A F8" */
    obd_result_t r;

    r = obd_elm327_clean_response(TEST_RAW_RPM_RESPONSE, out, sizeof(out));
    TEST_ASSERT(r == OBD_ERROR_BUFFER_TOO_SMALL, "tiny buffer should return BUFFER_TOO_SMALL");

    printf("  PASS: clean response — buffer too small\n");
    return 0;
}

int main(void)
{
    int failures = 0;

    printf("=== elm327 tests ===\n");
    failures += test_at_commands();
    failures += test_classify_response();
    failures += test_clean_response_normal();
    failures += test_clean_response_errors();
    failures += test_clean_response_ok();
    failures += test_classify_bus_errors();
    failures += test_clean_response_buffer_too_small();

    printf("\n%s (%d test functions)\n",
           failures == 0 ? "ALL PASSED" : "SOME FAILED", 7);
    return failures;
}
