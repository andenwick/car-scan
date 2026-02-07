/**
 * pid.c — OBD-II PID request building and response parsing.
 *
 * A PID (Parameter ID) is a specific data point you can read from the car.
 * For example, PID 0x0C is engine RPM, PID 0x0D is vehicle speed.
 *
 * OBD-II "modes" determine what TYPE of request:
 *   Mode 01 = live sensor data (what's happening right now)
 *   Mode 02 = freeze frame data (snapshot from when a DTC was stored)
 *   Mode 03 = stored DTCs (handled by dtc.c, not here)
 *   Mode 09 = vehicle info like VIN (handled by vin.c, not here)
 *
 * The request format is simple: mode byte + PID byte + \r
 *   "010C\r" = Mode 01, PID 0C (RPM)
 *   "020C\r" = Mode 02, PID 0C (RPM at time of freeze frame)
 *
 * The response format: response_mode + PID + data bytes
 *   "41 0C 1A F8" = Mode 01 response (0x41 = 0x01 + 0x40), PID 0C, data bytes
 */

#include "pid.h"
#include "hex_utils.h"
#include <obd/obd.h>
#include <stdio.h>
#include <string.h>

/*
 * Build a PID request command string.
 *
 * Uses snprintf to format the mode and PID as a 4-character hex string
 * followed by \r. snprintf is safe — it never writes beyond out_size.
 *
 * Example: mode=0x01, pid=0x0C → "010C\r"
 */
obd_result_t obd_pid_build_request(uint8_t mode, uint8_t pid,
                                   char *out, size_t out_size)
{
    int n;

    if (!out || out_size == 0) {
        return OBD_ERROR_INVALID_ARG;
    }

    /* Format: "MMPP\r" = 2 hex digits for mode + 2 for PID + \r + \0 = 6 chars */
    if (out_size < 6) {
        return OBD_ERROR_BUFFER_TOO_SMALL;
    }

    /* %02X = uppercase hex, zero-padded to 2 digits
     * snprintf returns the number of characters written (not counting \0) */
    n = snprintf(out, out_size, "%02X%02X\r", mode, pid);
    if (n < 0 || (size_t)n >= out_size) {
        return OBD_ERROR_BUFFER_TOO_SMALL;
    }

    return OBD_OK;
}


/*
 * Parse a PID response string into a structured result.
 *
 * Input example: "41 0C 1A F8"
 * Steps:
 *   1. Convert entire hex string to byte array: {0x41, 0x0C, 0x1A, 0xF8}
 *   2. First byte is the response mode (0x41)
 *   3. Second byte is the PID (0x0C)
 *   4. Remaining bytes are the data (0x1A, 0xF8)
 */
obd_result_t obd_pid_parse_response(const char *response,
                                    obd_pid_response_t *out)
{
    /* Buffer big enough for mode + PID + max data bytes */
    uint8_t bytes[2 + OBD_MAX_DATA_BYTES];
    size_t byte_count = 0;
    obd_result_t r;

    if (!response || !out) {
        return OBD_ERROR_INVALID_ARG;
    }

    /* Convert the hex string to raw bytes */
    r = obd_hex_to_bytes(response, bytes, sizeof(bytes), &byte_count);
    if (r != OBD_OK) {
        return r;
    }

    /* Need at least 2 bytes: mode + PID (some PIDs have 0 data bytes,
     * but practically all have at least 1) */
    if (byte_count < 2) {
        return OBD_ERROR_PARSE_FAILED;
    }

    /* Fill in the output struct */
    memset(out, 0, sizeof(*out));
    out->mode = bytes[0];
    out->pid = bytes[1];
    out->data_len = byte_count - 2;  /* Everything after mode and PID */

    /* Copy data bytes */
    if (out->data_len > OBD_MAX_DATA_BYTES) {
        out->data_len = OBD_MAX_DATA_BYTES;
    }
    if (out->data_len > 0) {
        memcpy(out->data, &bytes[2], out->data_len);
    }

    return OBD_OK;
}
