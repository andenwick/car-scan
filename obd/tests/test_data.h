/**
 * test_data.h — Canned ELM327/OBD-II responses for testing.
 *
 * These are real response formats from actual ELM327 adapters.
 * Every test file includes this header to get realistic test data
 * without needing a real car or adapter.
 *
 * Format notes:
 *   - \r = carriage return (ELM327 uses this as line terminator)
 *   - > = prompt character (adapter ready for next command)
 *   - The adapter echoes back your command before the response
 *     (unless echo is disabled with ATE0)
 */

#ifndef TEST_DATA_H
#define TEST_DATA_H

/* ── Raw adapter responses (with echo + prompt) ────────────────────────
 * These simulate what the adapter ACTUALLY sends over Bluetooth before
 * we clean it up. Includes command echo, response data, and > prompt.
 */
#define TEST_RAW_RPM_RESPONSE       "010C\r41 0C 1A F8\r\r>"
#define TEST_RAW_SPEED_RESPONSE     "010D\r41 0D 3C\r\r>"
#define TEST_RAW_COOLANT_RESPONSE   "0105\r41 05 7B\r\r>"
#define TEST_RAW_NO_DATA_RESPONSE   "0100\rNO DATA\r\r>"
#define TEST_RAW_ERROR_RESPONSE     "ATZZ\r?\r\r>"
#define TEST_RAW_OK_RESPONSE        "ATE0\rOK\r\r>"
#define TEST_RAW_RESET_RESPONSE     "ATZ\r\rELM327 v1.5\r\r>"

/* ── Cleaned hex responses (just the data, no echo/prompt) ─────────────
 * These are what you get AFTER calling obd_elm327_clean_response().
 * Ready to pass to obd_pid_parse_response(), obd_dtc_parse_response(), etc.
 */

/* Mode 01 PID responses (live sensor data) */
#define TEST_CLEAN_RPM              "41 0C 1A F8"   /* RPM: ((0x1A*256)+0xF8)/4 = 1726.0 */
#define TEST_CLEAN_SPEED            "41 0D 3C"       /* Speed: 0x3C = 60 km/h */
#define TEST_CLEAN_COOLANT          "41 05 7B"       /* Coolant: 0x7B-40 = 83°C */
#define TEST_CLEAN_THROTTLE         "41 11 33"       /* Throttle: 0x33*100/255 = 20.0% */
#define TEST_CLEAN_INTAKE_TEMP      "41 0F 46"       /* Intake: 0x46-40 = 30°C */
#define TEST_CLEAN_MAF              "41 10 01 A4"    /* MAF: ((1*256)+164)/100 = 4.20 g/s */
#define TEST_CLEAN_FUEL_PRESSURE    "41 0A 64"       /* Fuel pressure: 0x64*3 = 300 kPa */
#define TEST_CLEAN_ENGINE_LOAD      "41 04 4C"       /* Engine load: 0x4C*100/255 = 29.8% */
#define TEST_CLEAN_TIMING_ADVANCE   "41 0E 80"       /* Timing: 0x80/2-64 = 0.0° */
#define TEST_CLEAN_FUEL_TRIM       "41 06 80"       /* Fuel trim: (0x80-128)*100/128 = 0.0% */
#define TEST_CLEAN_O2_VOLTAGE      "41 14 C8"       /* O2 voltage: 0xC8/200 = 1.0 V */
#define TEST_CLEAN_RUNTIME         "41 1F 01 00"    /* Runtime: (1*256)+0 = 256 sec */
#define TEST_CLEAN_DTC_COUNT       "41 01 83 00 00 00"  /* DTC count: 0x83&0x7F = 3 */

/* Mode 03 DTC responses */
#define TEST_CLEAN_DTC_TWO_CODES    "43 01 03 01 04 00 00"  /* P0103, P0104 */
#define TEST_CLEAN_DTC_MIXED_CODES  "43 01 03 41 04 80 00"  /* P0103, C0104, B0000 (padding) */
#define TEST_CLEAN_DTC_NO_CODES     "43 00 00 00 00 00 00"  /* No stored DTCs */
#define TEST_CLEAN_DTC_U_CODE       "43 C1 23 00 00 00 00"  /* U0123 */

/* Mode 09 VIN responses (multi-line)
 * VIN: "WBA3B5FK7FN123456" (a typical BMW VIN)
 * W=0x57 B=0x42 A=0x41 3=0x33 B=0x42 5=0x35 F=0x46 K=0x4B
 * 7=0x37 F=0x46 N=0x4E 1=0x31 2=0x32 3=0x33 4=0x34 5=0x35 6=0x36
 */
#define TEST_CLEAN_VIN_MULTILINE \
    "49 02 01 57 42 41 33\r"  \
    "49 02 02 42 35 46 4B\r"  \
    "49 02 03 37 46 4E 31\r"  \
    "49 02 04 32 33 34 35\r"  \
    "49 02 05 36 00 00 00"

#define TEST_EXPECTED_VIN           "WBA3B5FK7FN123456"

/* ── Hex conversion test data ──────────────────────────────────────────── */
#define TEST_HEX_STRING_SPACED      "41 0C 1A F8"
#define TEST_HEX_STRING_NO_SPACES   "410C1AF8"
#define TEST_HEX_EXPECTED_BYTE_0    0x41
#define TEST_HEX_EXPECTED_BYTE_1    0x0C
#define TEST_HEX_EXPECTED_BYTE_2    0x1A
#define TEST_HEX_EXPECTED_BYTE_3    0xF8
#define TEST_HEX_EXPECTED_LEN       4

/* Expected decoded sensor values (for verifying math) */
#define TEST_EXPECTED_RPM           1726.0f
#define TEST_EXPECTED_SPEED         60.0f
#define TEST_EXPECTED_COOLANT       83.0f
#define TEST_EXPECTED_THROTTLE      20.0f   /* 0x33=51, 51*100/255 = 20.0 */
#define TEST_EXPECTED_INTAKE_TEMP   30.0f
#define TEST_EXPECTED_MAF           4.20f
#define TEST_EXPECTED_FUEL_PRESSURE 300.0f
#define TEST_EXPECTED_TIMING_ADV   0.0f
#define TEST_EXPECTED_FUEL_TRIM    0.0f
#define TEST_EXPECTED_O2_VOLTAGE   1.0f
#define TEST_EXPECTED_RUNTIME      256.0f
#define TEST_EXPECTED_DTC_COUNT    3.0f

#endif /* TEST_DATA_H */
