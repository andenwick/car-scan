/**
 * test_hex_utils.c — Tests for hex string <-> byte array conversion.
 *
 * We use simple assert()-style macros instead of a test framework.
 * Each test function returns 0 on success, 1 on failure.
 * main() runs all tests and returns 0 if all pass (ctest checks this).
 */

#include <obd/obd.h>
#include "test_assert.h"
#include "test_data.h"
#include <stdio.h>
#include <string.h>

/* ── Test: hex string with spaces → byte array ─────────────────────── */
static int test_hex_to_bytes_spaced(void)
{
    uint8_t buf[16];
    size_t len = 0;
    obd_result_t r;

    r = obd_hex_to_bytes(TEST_HEX_STRING_SPACED, buf, sizeof(buf), &len);
    TEST_ASSERT(r == OBD_OK, "hex_to_bytes should succeed");
    TEST_ASSERT(len == TEST_HEX_EXPECTED_LEN, "should parse 4 bytes");
    TEST_ASSERT(buf[0] == TEST_HEX_EXPECTED_BYTE_0, "byte 0 should be 0x41");
    TEST_ASSERT(buf[1] == TEST_HEX_EXPECTED_BYTE_1, "byte 1 should be 0x0C");
    TEST_ASSERT(buf[2] == TEST_HEX_EXPECTED_BYTE_2, "byte 2 should be 0x1A");
    TEST_ASSERT(buf[3] == TEST_HEX_EXPECTED_BYTE_3, "byte 3 should be 0xF8");

    printf("  PASS: hex_to_bytes with spaces\n");
    return 0;
}

/* ── Test: hex string without spaces → same byte array ─────────────── */
static int test_hex_to_bytes_no_spaces(void)
{
    uint8_t buf[16];
    size_t len = 0;
    obd_result_t r;

    r = obd_hex_to_bytes(TEST_HEX_STRING_NO_SPACES, buf, sizeof(buf), &len);
    TEST_ASSERT(r == OBD_OK, "hex_to_bytes should succeed without spaces");
    TEST_ASSERT(len == TEST_HEX_EXPECTED_LEN, "should parse 4 bytes");
    TEST_ASSERT(buf[0] == TEST_HEX_EXPECTED_BYTE_0, "byte 0 should be 0x41");
    TEST_ASSERT(buf[3] == TEST_HEX_EXPECTED_BYTE_3, "byte 3 should be 0xF8");

    printf("  PASS: hex_to_bytes without spaces\n");
    return 0;
}

/* ── Test: byte array → hex string ─────────────────────────────────── */
static int test_bytes_to_hex(void)
{
    uint8_t bytes[] = {0x41, 0x0C, 0x1A, 0xF8};
    char buf[32];
    obd_result_t r;

    r = obd_bytes_to_hex(bytes, 4, buf, sizeof(buf));
    TEST_ASSERT(r == OBD_OK, "bytes_to_hex should succeed");
    TEST_ASSERT(strcmp(buf, "41 0C 1A F8") == 0, "should produce '41 0C 1A F8'");

    printf("  PASS: bytes_to_hex\n");
    return 0;
}

/* ── Test: round-trip (hex → bytes → hex) ──────────────────────────── */
static int test_roundtrip(void)
{
    uint8_t bytes[16];
    char hex_out[64];
    size_t len = 0;
    obd_result_t r;

    r = obd_hex_to_bytes("DE AD BE EF", bytes, sizeof(bytes), &len);
    TEST_ASSERT(r == OBD_OK, "hex_to_bytes should succeed");
    TEST_ASSERT(len == 4, "should have 4 bytes");

    r = obd_bytes_to_hex(bytes, len, hex_out, sizeof(hex_out));
    TEST_ASSERT(r == OBD_OK, "bytes_to_hex should succeed");
    TEST_ASSERT(strcmp(hex_out, "DE AD BE EF") == 0, "round-trip should preserve data");

    printf("  PASS: round-trip hex->bytes->hex\n");
    return 0;
}

/* ── Test: strip whitespace ────────────────────────────────────────── */
static int test_strip_whitespace(void)
{
    char buf1[] = "41 0C 1A F8\r\n";
    char buf2[] = "  NO DATA  \r\n";
    char buf3[] = "ABCD";

    obd_strip_whitespace(buf1);
    TEST_ASSERT(strcmp(buf1, "410C1AF8") == 0, "should strip spaces and \\r\\n");

    obd_strip_whitespace(buf2);
    TEST_ASSERT(strcmp(buf2, "NODATA") == 0, "should strip all whitespace");

    obd_strip_whitespace(buf3);
    TEST_ASSERT(strcmp(buf3, "ABCD") == 0, "no-op on string without whitespace");

    printf("  PASS: strip_whitespace\n");
    return 0;
}

/* ── Test: error cases ─────────────────────────────────────────────── */
static int test_error_cases(void)
{
    uint8_t buf[4];
    size_t len = 0;
    obd_result_t r;

    /* NULL input */
    r = obd_hex_to_bytes(NULL, buf, sizeof(buf), &len);
    TEST_ASSERT(r == OBD_ERROR_INVALID_ARG, "NULL input should error");

    /* Invalid hex character */
    r = obd_hex_to_bytes("41 GG", buf, sizeof(buf), &len);
    TEST_ASSERT(r == OBD_ERROR_INVALID_HEX, "invalid hex should error");

    /* Buffer too small */
    r = obd_hex_to_bytes("41 0C 1A F8", buf, 2, &len);
    TEST_ASSERT(r == OBD_ERROR_BUFFER_TOO_SMALL, "small buffer should error");

    /* Odd number of hex chars */
    r = obd_hex_to_bytes("41 0", buf, sizeof(buf), &len);
    TEST_ASSERT(r == OBD_ERROR_INVALID_HEX, "odd hex chars should error");

    /* Empty string (valid, produces 0 bytes) */
    r = obd_hex_to_bytes("", buf, sizeof(buf), &len);
    TEST_ASSERT(r == OBD_OK, "empty string is valid");
    TEST_ASSERT(len == 0, "empty string produces 0 bytes");

    printf("  PASS: error cases\n");
    return 0;
}

/* ── Test: lowercase hex input ─────────────────────────────────────── */
static int test_lowercase_hex(void)
{
    uint8_t buf[4];
    size_t len = 0;
    obd_result_t r;

    r = obd_hex_to_bytes("de ad be ef", buf, sizeof(buf), &len);
    TEST_ASSERT(r == OBD_OK, "lowercase hex should work");
    TEST_ASSERT(len == 4, "should parse 4 bytes");
    TEST_ASSERT(buf[0] == 0xDE, "byte 0 should be 0xDE");
    TEST_ASSERT(buf[3] == 0xEF, "byte 3 should be 0xEF");

    printf("  PASS: lowercase hex\n");
    return 0;
}

int main(void)
{
    int failures = 0;

    printf("=== hex_utils tests ===\n");
    failures += test_hex_to_bytes_spaced();
    failures += test_hex_to_bytes_no_spaces();
    failures += test_bytes_to_hex();
    failures += test_roundtrip();
    failures += test_strip_whitespace();
    failures += test_error_cases();
    failures += test_lowercase_hex();

    printf("\n%s (%d test functions)\n",
           failures == 0 ? "ALL PASSED" : "SOME FAILED", 7);
    return failures;
}
