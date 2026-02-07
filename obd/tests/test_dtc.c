/**
 * test_dtc.c — Tests for DTC (Diagnostic Trouble Code) parsing.
 */

#include <obd/obd.h>
#include "test_assert.h"
#include "test_data.h"
#include <stdio.h>
#include <string.h>

/* ── Test: build DTC request ───────────────────────────────────────── */
static int test_build_request(void)
{
    char buf[OBD_MAX_COMMAND_LEN];
    obd_result_t r;

    r = obd_dtc_build_request(buf, sizeof(buf));
    TEST_ASSERT(r == OBD_OK, "build should succeed");
    TEST_ASSERT(strcmp(buf, "03\r") == 0, "should produce '03\\r'");

    /* Buffer too small */
    r = obd_dtc_build_request(buf, 2);
    TEST_ASSERT(r == OBD_ERROR_BUFFER_TOO_SMALL, "small buffer should error");

    printf("  PASS: build DTC request\n");
    return 0;
}

/* ── Test: parse two P-codes ───────────────────────────────────────── */
static int test_parse_two_codes(void)
{
    obd_dtc_list_t list;
    obd_result_t r;

    /* "43 01 03 01 04 00 00" → P0103, P0104 */
    r = obd_dtc_parse_response(TEST_CLEAN_DTC_TWO_CODES, &list);
    TEST_ASSERT(r == OBD_OK, "parse should succeed");
    TEST_ASSERT(list.count == 2, "should find 2 DTCs");

    TEST_ASSERT(list.dtcs[0].category == OBD_DTC_POWERTRAIN, "DTC 0 should be P");
    TEST_ASSERT(strcmp(list.dtcs[0].formatted, "P0103") == 0, "DTC 0 should be P0103");

    TEST_ASSERT(list.dtcs[1].category == OBD_DTC_POWERTRAIN, "DTC 1 should be P");
    TEST_ASSERT(strcmp(list.dtcs[1].formatted, "P0104") == 0, "DTC 1 should be P0104");

    printf("  PASS: parse two P-codes (P0103, P0104)\n");
    return 0;
}

/* ── Test: parse mixed categories ──────────────────────────────────── */
static int test_parse_mixed_codes(void)
{
    obd_dtc_list_t list;
    obd_result_t r;
    char buf[OBD_DTC_CODE_LENGTH];

    /* "43 01 03 41 04 80 00" → P0103, C0104, then 80 00 which is B0000 */
    r = obd_dtc_parse_response(TEST_CLEAN_DTC_MIXED_CODES, &list);
    TEST_ASSERT(r == OBD_OK, "parse should succeed");
    TEST_ASSERT(list.count == 3, "should find 3 DTCs");

    /* First DTC: 01 03 → P0103 (top 2 bits = 00 → P) */
    TEST_ASSERT(list.dtcs[0].category == OBD_DTC_POWERTRAIN, "DTC 0 should be P");
    TEST_ASSERT(strcmp(list.dtcs[0].formatted, "P0103") == 0, "DTC 0 should be P0103");

    /* Second DTC: 41 04 → 0100 0001 → top 2 bits = 01 → C (Chassis)
     * d2 = 00, d3 = 0001, d4 = 0000, d5 = 0100 → C0104 */
    TEST_ASSERT(list.dtcs[1].category == OBD_DTC_CHASSIS, "DTC 1 should be C");
    TEST_ASSERT(strcmp(list.dtcs[1].formatted, "C0104") == 0, "DTC 1 should be C0104");

    /* Third DTC: 80 00 → 1000 0000 → top 2 bits = 10 → B (Body)
     * d2 = 00, d3 = 0000, d4 = 0000, d5 = 0000 → B0000 */
    TEST_ASSERT(list.dtcs[2].category == OBD_DTC_BODY, "DTC 2 should be B");
    TEST_ASSERT(strcmp(list.dtcs[2].formatted, "B0000") == 0, "DTC 2 should be B0000");

    /* Test obd_dtc_format() */
    r = obd_dtc_format(&list.dtcs[0], buf, sizeof(buf));
    TEST_ASSERT(r == OBD_OK, "format should succeed");
    TEST_ASSERT(strcmp(buf, "P0103") == 0, "format should produce P0103");

    printf("  PASS: parse mixed categories (P, C, B)\n");
    return 0;
}

/* ── Test: no DTCs stored ──────────────────────────────────────────── */
static int test_no_codes(void)
{
    obd_dtc_list_t list;
    obd_result_t r;

    /* "43 00 00 00 00 00 00" → all padding, no real DTCs */
    r = obd_dtc_parse_response(TEST_CLEAN_DTC_NO_CODES, &list);
    TEST_ASSERT(r == OBD_OK, "parse should succeed even with no codes");
    TEST_ASSERT(list.count == 0, "should find 0 DTCs");

    printf("  PASS: no DTCs stored\n");
    return 0;
}

/* ── Test: parse U-code (network) ─────────────────────────────────── */
static int test_parse_u_code(void)
{
    obd_dtc_list_t list;
    obd_result_t r;

    /* "43 C1 23 00 00 00 00" → U0123 */
    r = obd_dtc_parse_response(TEST_CLEAN_DTC_U_CODE, &list);
    TEST_ASSERT(r == OBD_OK, "parse should succeed");
    TEST_ASSERT(list.count == 1, "should find 1 DTC");

    TEST_ASSERT(list.dtcs[0].category == OBD_DTC_NETWORK, "DTC 0 should be U");
    TEST_ASSERT(strcmp(list.dtcs[0].formatted, "U0123") == 0, "DTC 0 should be U0123");

    printf("  PASS: parse U-code (U0123)\n");
    return 0;
}

/* ── Test: error cases ─────────────────────────────────────────────── */
static int test_errors(void)
{
    obd_dtc_list_t list;
    obd_result_t r;

    r = obd_dtc_parse_response(NULL, &list);
    TEST_ASSERT(r == OBD_ERROR_INVALID_ARG, "NULL should error");

    /* Wrong header (not 0x43) */
    r = obd_dtc_parse_response("41 01 03", &list);
    TEST_ASSERT(r == OBD_ERROR_PARSE_FAILED, "wrong header should fail");

    printf("  PASS: error cases\n");
    return 0;
}

int main(void)
{
    int failures = 0;

    printf("=== dtc tests ===\n");
    failures += test_build_request();
    failures += test_parse_two_codes();
    failures += test_parse_mixed_codes();
    failures += test_no_codes();
    failures += test_parse_u_code();
    failures += test_errors();

    printf("\n%s (%d test functions)\n",
           failures == 0 ? "ALL PASSED" : "SOME FAILED", 6);
    return failures;
}
