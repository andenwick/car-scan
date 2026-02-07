/**
 * vin.c — VIN (Vehicle Identification Number) multi-frame decoding.
 *
 * The VIN is a 17-character string like "WBA3B5FK7FN123456" that
 * uniquely identifies every vehicle ever made. It encodes:
 *   - Manufacturer (characters 1-3): WBA = BMW
 *   - Vehicle attributes (4-8): model, engine, body style
 *   - Check digit (9): error detection
 *   - Model year (10): F = 2015
 *   - Assembly plant (11): N = specific factory
 *   - Serial number (12-17): unique to this car
 *
 * OBD-II retrieves the VIN via Mode 09, PID 02. But 17 characters
 * don't fit in a single OBD frame (~4-5 bytes per frame), so the
 * response comes in multiple lines:
 *
 *   "49 02 01 57 42 41 33\r"   ← line 1: header, sequence 01, data bytes
 *   "49 02 02 42 35 46 4B\r"   ← line 2: header, sequence 02, data bytes
 *   "49 02 03 37 46 4E 31\r"   ← line 3: header, sequence 03, data bytes
 *   "49 02 04 32 33 34 35\r"   ← line 4: header, sequence 04, data bytes
 *   "49 02 05 36 00 00 00"     ← line 5: header, sequence 05, data + padding
 *
 * Each line: 0x49 (response to Mode 09), 0x02 (PID 02 = VIN),
 * sequence number, then 4 bytes of VIN data (ASCII codes).
 * 57 = 'W', 42 = 'B', 41 = 'A', 33 = '3', etc.
 *
 * We need to collect all the data bytes across all lines,
 * skip the null padding, and assemble the 17-character string.
 */

#include "vin.h"
#include "hex_utils.h"
#include <obd/obd.h>
#include <string.h>

obd_result_t obd_vin_build_request(char *out, size_t out_size)
{
    if (!out || out_size == 0) {
        return OBD_ERROR_INVALID_ARG;
    }

    /* "0902\r\0" = 6 characters */
    if (out_size < 6) {
        return OBD_ERROR_BUFFER_TOO_SMALL;
    }

    out[0] = '0';
    out[1] = '9';
    out[2] = '0';
    out[3] = '2';
    out[4] = '\r';
    out[5] = '\0';

    return OBD_OK;
}


/*
 * Parse a multi-line VIN response.
 *
 * Strategy:
 *   1. Split the response by \r into individual lines
 *   2. For each line, convert hex to bytes
 *   3. Verify header bytes (0x49 0x02)
 *   4. Skip the sequence number byte
 *   5. Append remaining bytes as ASCII characters to the VIN string
 *   6. Stop at 17 characters (ignore null padding bytes)
 */
obd_result_t obd_vin_parse_response(const char *response,
                                    char *vin, size_t vin_size)
{
    size_t vin_pos;
    const char *line_start;
    const char *p;

    if (!response || !vin || vin_size == 0) {
        return OBD_ERROR_INVALID_ARG;
    }

    if (vin_size < OBD_VIN_LENGTH + 1) {
        return OBD_ERROR_BUFFER_TOO_SMALL;
    }

    memset(vin, 0, vin_size);
    vin_pos = 0;
    p = response;

    /* Process each line */
    while (*p != '\0' && vin_pos < OBD_VIN_LENGTH) {
        uint8_t line_bytes[16];
        size_t line_byte_count = 0;
        size_t data_start;
        size_t j;
        char line_hex[OBD_MAX_RESPONSE_LEN];
        size_t line_len;
        obd_result_t r;

        /* Skip line separators */
        while (*p == '\r' || *p == '\n') {
            p++;
        }
        if (*p == '\0') break;

        /* Find end of this line */
        line_start = p;
        while (*p != '\0' && *p != '\r' && *p != '\n') {
            p++;
        }

        /* Copy line to temporary buffer for hex conversion */
        line_len = (size_t)(p - line_start);
        if (line_len >= sizeof(line_hex)) {
            line_len = sizeof(line_hex) - 1;
        }
        memcpy(line_hex, line_start, line_len);
        line_hex[line_len] = '\0';

        /* Convert hex line to bytes */
        r = obd_hex_to_bytes(line_hex, line_bytes, sizeof(line_bytes),
                             &line_byte_count);
        if (r != OBD_OK) {
            continue; /* Skip malformed lines */
        }

        /* Verify header: must start with 0x49 0x02 (Mode 09, PID 02 response) */
        if (line_byte_count < 4 || line_bytes[0] != 0x49 || line_bytes[1] != 0x02) {
            continue; /* Not a VIN response line */
        }

        /* Byte 2 is the sequence number (01, 02, 03, etc.) — skip it.
         * Data starts at byte 3. */
        data_start = 3;

        /* Append data bytes as ASCII characters */
        for (j = data_start; j < line_byte_count && vin_pos < OBD_VIN_LENGTH; j++) {
            /* Skip null bytes (padding at end of last frame) */
            if (line_bytes[j] == 0x00) {
                continue;
            }
            vin[vin_pos] = (char)line_bytes[j];
            vin_pos++;
        }
    }

    vin[vin_pos] = '\0';

    /* VIN must be exactly 17 characters */
    if (vin_pos != OBD_VIN_LENGTH) {
        return OBD_ERROR_PARSE_FAILED;
    }

    return OBD_OK;
}
