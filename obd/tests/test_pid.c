/**
 * test_pid.c — Tests for PID request building and response parsing.
 */

#include <obd/obd.h>
#include "test_assert.h"
#include "test_data.h"
#include <stdio.h>
#include <string.h>

/* ── Test: build PID request ───────────────────────────────────────── */
static int test_build_request(void)
{
    char buf[OBD_MAX_COMMAND_LEN];
    obd_result_t r;

    /* Mode 01, PID 0C (RPM) → "010C\r" */
    r = obd_pid_build_request(0x01, 0x0C, buf, sizeof(buf));
    TEST_ASSERT(r == OBD_OK, "build request should succeed");
    TEST_ASSERT(strcmp(buf, "010C\r") == 0, "should produce '010C\\r'");

    /* Mode 01, PID 0D (Speed) → "010D\r" */
    r = obd_pid_build_request(0x01, 0x0D, buf, sizeof(buf));
    TEST_ASSERT(r == OBD_OK, "build speed request should succeed");
    TEST_ASSERT(strcmp(buf, "010D\r") == 0, "should produce '010D\\r'");

    /* Mode 02, PID 0C (freeze frame RPM) → "020C\r" */
    r = obd_pid_build_request(0x02, 0x0C, buf, sizeof(buf));
    TEST_ASSERT(r == OBD_OK, "build freeze frame request should succeed");
    TEST_ASSERT(strcmp(buf, "020C\r") == 0, "should produce '020C\\r'");

    /* Error: buffer too small */
    r = obd_pid_build_request(0x01, 0x0C, buf, 3);
    TEST_ASSERT(r == OBD_ERROR_BUFFER_TOO_SMALL, "small buffer should error");

    /* Error: NULL output */
    r = obd_pid_build_request(0x01, 0x0C, NULL, sizeof(buf));
    TEST_ASSERT(r == OBD_ERROR_INVALID_ARG, "NULL output should error");

    printf("  PASS: build PID request\n");
    return 0;
}

/* ── Test: parse PID response ──────────────────────────────────────── */
static int test_parse_response(void)
{
    obd_pid_response_t resp;
    obd_result_t r;

    /* Parse RPM response: "41 0C 1A F8" */
    r = obd_pid_parse_response(TEST_CLEAN_RPM, &resp);
    TEST_ASSERT(r == OBD_OK, "parse RPM should succeed");
    TEST_ASSERT(resp.mode == 0x41, "mode should be 0x41");
    TEST_ASSERT(resp.pid == 0x0C, "PID should be 0x0C");
    TEST_ASSERT(resp.data_len == 2, "should have 2 data bytes");
    TEST_ASSERT(resp.data[0] == 0x1A, "data[0] should be 0x1A");
    TEST_ASSERT(resp.data[1] == 0xF8, "data[1] should be 0xF8");

    /* Parse speed response: "41 0D 3C" */
    r = obd_pid_parse_response(TEST_CLEAN_SPEED, &resp);
    TEST_ASSERT(r == OBD_OK, "parse speed should succeed");
    TEST_ASSERT(resp.mode == 0x41, "mode should be 0x41");
    TEST_ASSERT(resp.pid == 0x0D, "PID should be 0x0D");
    TEST_ASSERT(resp.data_len == 1, "should have 1 data byte");
    TEST_ASSERT(resp.data[0] == 0x3C, "data[0] should be 0x3C (60)");

    /* Parse coolant response: "41 05 7B" */
    r = obd_pid_parse_response(TEST_CLEAN_COOLANT, &resp);
    TEST_ASSERT(r == OBD_OK, "parse coolant should succeed");
    TEST_ASSERT(resp.pid == 0x05, "PID should be 0x05");
    TEST_ASSERT(resp.data[0] == 0x7B, "data[0] should be 0x7B (123)");

    printf("  PASS: parse PID response\n");
    return 0;
}

/* ── Test: error cases ─────────────────────────────────────────────── */
static int test_parse_errors(void)
{
    obd_pid_response_t resp;
    obd_result_t r;

    /* NULL input */
    r = obd_pid_parse_response(NULL, &resp);
    TEST_ASSERT(r == OBD_ERROR_INVALID_ARG, "NULL input should error");

    /* Too short (only 1 byte — need at least mode + PID = 2) */
    r = obd_pid_parse_response("41", &resp);
    TEST_ASSERT(r == OBD_ERROR_PARSE_FAILED, "1 byte should fail");

    /* Invalid hex */
    r = obd_pid_parse_response("ZZ XX", &resp);
    TEST_ASSERT(r == OBD_ERROR_INVALID_HEX, "invalid hex should error");

    printf("  PASS: parse error cases\n");
    return 0;
}

int main(void)
{
    int failures = 0;

    printf("=== pid tests ===\n");
    failures += test_build_request();
    failures += test_parse_response();
    failures += test_parse_errors();

    printf("\n%s (%d test functions)\n",
           failures == 0 ? "ALL PASSED" : "SOME FAILED", 3);
    return failures;
}
