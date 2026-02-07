/**
 * test_sensor.c — Tests for sensor value decoding (raw bytes → engineering units).
 *
 * We test by parsing a known hex response, then decoding it, and checking
 * the resulting value matches our hand-calculated expected value.
 */

#include <obd/obd.h>
#include "test_assert.h"
#include "test_data.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

/* Float comparison with small tolerance (floating point isn't exact) */
#define FLOAT_NEAR(a, b, tolerance) (fabs((double)(a) - (double)(b)) < (tolerance))

/* Helper: parse a hex response string AND decode it in one step.
 * Returns 0 on success, 1 on failure. */
static int parse_and_decode(const char *hex, obd_sensor_value_t *out)
{
    obd_pid_response_t pid_resp;
    obd_result_t r;

    r = obd_pid_parse_response(hex, &pid_resp);
    if (r != OBD_OK) return 1;

    r = obd_sensor_decode(&pid_resp, out);
    if (r != OBD_OK) return 1;

    return 0;
}

/* ── Test: RPM decoding ────────────────────────────────────────────── */
static int test_rpm(void)
{
    obd_sensor_value_t val;

    /* "41 0C 1A F8" → RPM = ((0x1A * 256) + 0xF8) / 4 = 1726.0 */
    TEST_ASSERT(parse_and_decode(TEST_CLEAN_RPM, &val) == 0, "RPM decode should succeed");
    TEST_ASSERT(FLOAT_NEAR(val.value, TEST_EXPECTED_RPM, 0.1f), "RPM should be 1726.0");
    TEST_ASSERT(strcmp(val.name, "Engine RPM") == 0, "name should be 'Engine RPM'");
    TEST_ASSERT(strcmp(val.unit, "rpm") == 0, "unit should be 'rpm'");
    TEST_ASSERT(val.pid == 0x0C, "PID should be 0x0C");

    printf("  PASS: RPM decoding (1726.0 rpm)\n");
    return 0;
}

/* ── Test: vehicle speed ───────────────────────────────────────────── */
static int test_speed(void)
{
    obd_sensor_value_t val;

    /* "41 0D 3C" → Speed = 0x3C = 60 km/h */
    TEST_ASSERT(parse_and_decode(TEST_CLEAN_SPEED, &val) == 0, "speed decode should succeed");
    TEST_ASSERT(FLOAT_NEAR(val.value, TEST_EXPECTED_SPEED, 0.1f), "speed should be 60.0");
    TEST_ASSERT(strcmp(val.unit, "km/h") == 0, "unit should be 'km/h'");

    printf("  PASS: speed decoding (60 km/h)\n");
    return 0;
}

/* ── Test: coolant temperature ─────────────────────────────────────── */
static int test_coolant(void)
{
    obd_sensor_value_t val;

    /* "41 05 7B" → Temp = 0x7B - 40 = 123 - 40 = 83°C */
    TEST_ASSERT(parse_and_decode(TEST_CLEAN_COOLANT, &val) == 0, "coolant decode should succeed");
    TEST_ASSERT(FLOAT_NEAR(val.value, TEST_EXPECTED_COOLANT, 0.1f), "coolant should be 83.0");
    TEST_ASSERT(strcmp(val.unit, "C") == 0, "unit should be 'C'");

    printf("  PASS: coolant temperature (83 C)\n");
    return 0;
}

/* ── Test: throttle position ───────────────────────────────────────── */
static int test_throttle(void)
{
    obd_sensor_value_t val;

    /* "41 11 33" → Throttle = 0x33 * 100 / 255 = 51 * 100 / 255 = 20.0% */
    TEST_ASSERT(parse_and_decode(TEST_CLEAN_THROTTLE, &val) == 0, "throttle decode should succeed");
    TEST_ASSERT(FLOAT_NEAR(val.value, TEST_EXPECTED_THROTTLE, 0.1f), "throttle should be 20.0");
    TEST_ASSERT(strcmp(val.unit, "%") == 0, "unit should be '%'");

    printf("  PASS: throttle position (20.0%%)\n");
    return 0;
}

/* ── Test: intake air temperature ──────────────────────────────────── */
static int test_intake_temp(void)
{
    obd_sensor_value_t val;

    /* "41 0F 46" → Intake = 0x46 - 40 = 70 - 40 = 30°C */
    TEST_ASSERT(parse_and_decode(TEST_CLEAN_INTAKE_TEMP, &val) == 0, "intake decode should succeed");
    TEST_ASSERT(FLOAT_NEAR(val.value, TEST_EXPECTED_INTAKE_TEMP, 0.1f), "intake should be 30.0");

    printf("  PASS: intake air temperature (30 C)\n");
    return 0;
}

/* ── Test: MAF air flow ────────────────────────────────────────────── */
static int test_maf(void)
{
    obd_sensor_value_t val;

    /* "41 10 01 A4" → MAF = ((1 * 256) + 164) / 100 = 420 / 100 = 4.20 g/s */
    TEST_ASSERT(parse_and_decode(TEST_CLEAN_MAF, &val) == 0, "MAF decode should succeed");
    TEST_ASSERT(FLOAT_NEAR(val.value, TEST_EXPECTED_MAF, 0.01f), "MAF should be 4.20");
    TEST_ASSERT(strcmp(val.unit, "g/s") == 0, "unit should be 'g/s'");

    printf("  PASS: MAF air flow (4.20 g/s)\n");
    return 0;
}

/* ── Test: fuel pressure ───────────────────────────────────────────── */
static int test_fuel_pressure(void)
{
    obd_sensor_value_t val;

    /* "41 0A 64" → Fuel pressure = 0x64 * 3 = 100 * 3 = 300 kPa */
    TEST_ASSERT(parse_and_decode(TEST_CLEAN_FUEL_PRESSURE, &val) == 0, "fuel pressure decode should succeed");
    TEST_ASSERT(FLOAT_NEAR(val.value, TEST_EXPECTED_FUEL_PRESSURE, 0.1f), "fuel pressure should be 300.0");

    printf("  PASS: fuel pressure (300 kPa)\n");
    return 0;
}

/* ── Test: timing advance ────────────────────────────────────────── */
static int test_timing_advance(void)
{
    obd_sensor_value_t val;

    /* "41 0E 80" → Timing advance = 0x80/2 - 64 = 0.0 degrees */
    TEST_ASSERT(parse_and_decode(TEST_CLEAN_TIMING_ADVANCE, &val) == 0, "timing advance decode should succeed");
    TEST_ASSERT(FLOAT_NEAR(val.value, TEST_EXPECTED_TIMING_ADV, 0.1f), "timing advance should be 0.0");
    TEST_ASSERT(strcmp(val.unit, "deg") == 0, "unit should be 'deg'");

    printf("  PASS: timing advance (0.0 deg)\n");
    return 0;
}

/* ── Test: fuel trim ──────────────────────────────────────────────── */
static int test_fuel_trim(void)
{
    obd_sensor_value_t val;

    /* "41 06 80" → Fuel trim = (0x80-128)*100/128 = 0.0% */
    TEST_ASSERT(parse_and_decode(TEST_CLEAN_FUEL_TRIM, &val) == 0, "fuel trim decode should succeed");
    TEST_ASSERT(FLOAT_NEAR(val.value, TEST_EXPECTED_FUEL_TRIM, 0.1f), "fuel trim should be 0.0");
    TEST_ASSERT(strcmp(val.unit, "%") == 0, "unit should be '%'");

    printf("  PASS: fuel trim (0.0%%)\n");
    return 0;
}

/* ── Test: O2 sensor voltage ─────────────────────────────────────── */
static int test_o2_voltage(void)
{
    obd_sensor_value_t val;

    /* "41 14 C8" → O2 voltage = 0xC8/200 = 1.0 V */
    TEST_ASSERT(parse_and_decode(TEST_CLEAN_O2_VOLTAGE, &val) == 0, "O2 voltage decode should succeed");
    TEST_ASSERT(FLOAT_NEAR(val.value, TEST_EXPECTED_O2_VOLTAGE, 0.01f), "O2 voltage should be 1.0");
    TEST_ASSERT(strcmp(val.unit, "V") == 0, "unit should be 'V'");

    printf("  PASS: O2 sensor voltage (1.0 V)\n");
    return 0;
}

/* ── Test: runtime since start ───────────────────────────────────── */
static int test_runtime(void)
{
    obd_sensor_value_t val;

    /* "41 1F 01 00" → Runtime = (1*256)+0 = 256 seconds */
    TEST_ASSERT(parse_and_decode(TEST_CLEAN_RUNTIME, &val) == 0, "runtime decode should succeed");
    TEST_ASSERT(FLOAT_NEAR(val.value, TEST_EXPECTED_RUNTIME, 0.1f), "runtime should be 256.0");
    TEST_ASSERT(strcmp(val.unit, "sec") == 0, "unit should be 'sec'");

    printf("  PASS: runtime since start (256 sec)\n");
    return 0;
}

/* ── Test: DTC count ─────────────────────────────────────────────── */
static int test_dtc_count(void)
{
    obd_sensor_value_t val;

    /* "41 01 83 00 00 00" → DTC count = 0x83 & 0x7F = 3 */
    TEST_ASSERT(parse_and_decode(TEST_CLEAN_DTC_COUNT, &val) == 0, "DTC count decode should succeed");
    TEST_ASSERT(FLOAT_NEAR(val.value, TEST_EXPECTED_DTC_COUNT, 0.1f), "DTC count should be 3.0");

    printf("  PASS: DTC count (3)\n");
    return 0;
}

/* ── Test: unknown PID ─────────────────────────────────────────────── */
static int test_unknown_pid(void)
{
    obd_pid_response_t pid_resp;
    obd_sensor_value_t val;
    obd_result_t r;

    /* PID 0xFF is not in our table */
    pid_resp.mode = 0x41;
    pid_resp.pid = 0xFF;
    pid_resp.data[0] = 0x00;
    pid_resp.data_len = 1;

    r = obd_sensor_decode(&pid_resp, &val);
    TEST_ASSERT(r == OBD_ERROR_UNKNOWN_PID, "unknown PID should return error");

    printf("  PASS: unknown PID error\n");
    return 0;
}

/* ── Test: sensor get_name ─────────────────────────────────────────── */
static int test_get_name(void)
{
    char name[32];
    obd_result_t r;

    r = obd_sensor_get_name(0x0C, name, sizeof(name));
    TEST_ASSERT(r == OBD_OK, "get_name for RPM should succeed");
    TEST_ASSERT(strcmp(name, "Engine RPM") == 0, "name should be 'Engine RPM'");

    r = obd_sensor_get_name(0xFF, name, sizeof(name));
    TEST_ASSERT(r == OBD_ERROR_UNKNOWN_PID, "unknown PID should error");

    printf("  PASS: sensor get_name\n");
    return 0;
}

int main(void)
{
    int failures = 0;

    printf("=== sensor tests ===\n");
    failures += test_rpm();
    failures += test_speed();
    failures += test_coolant();
    failures += test_throttle();
    failures += test_intake_temp();
    failures += test_maf();
    failures += test_fuel_pressure();
    failures += test_timing_advance();
    failures += test_fuel_trim();
    failures += test_o2_voltage();
    failures += test_runtime();
    failures += test_dtc_count();
    failures += test_unknown_pid();
    failures += test_get_name();

    printf("\n%s (%d test functions)\n",
           failures == 0 ? "ALL PASSED" : "SOME FAILED", 14);
    return failures;
}
