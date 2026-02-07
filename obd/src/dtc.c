/**
 * dtc.c — DTC (Diagnostic Trouble Code) parsing.
 *
 * When the check engine light is on, Mode 03 retrieves stored DTCs.
 * Each DTC is encoded as 2 bytes that get decoded into a 5-character
 * string like "P0301" (cylinder 1 misfire).
 *
 * Raw 2-byte encoding:
 *   Bits 15-14 (top 2 bits of byte 1): category letter
 *     00 = P (Powertrain)
 *     01 = C (Chassis)
 *     10 = B (Body)
 *     11 = U (Network)
 *   Bits 13-12: second digit of the code (0-3)
 *   Bits 11-8:  third digit (0-F, but typically 0-9)
 *   Bits 7-4:   fourth digit
 *   Bits 3-0:   fifth digit
 *
 * Example: bytes 0x01 0x03
 *   Binary: 0000 0001 0000 0011
 *   Top 2 bits: 00 → P
 *   Next 2 bits: 00 → 0
 *   Next 4 bits: 0001 → 1
 *   Next 4 bits: 0000 → 0
 *   Last 4 bits: 0011 → 3
 *   Result: P0103
 */

#include "dtc.h"
#include "hex_utils.h"
#include <obd/obd.h>
#include <string.h>
#include <stdio.h>

/* Category letters indexed by the top 2 bits (0-3) */
static const char dtc_category_chars[] = "PCBU";


obd_result_t obd_dtc_build_request(char *out, size_t out_size)
{
    if (!out || out_size == 0) {
        return OBD_ERROR_INVALID_ARG;
    }

    /* Mode 03 = "read stored DTCs" — just "03\r" */
    if (out_size < 4) { /* "03\r\0" = 4 chars */
        return OBD_ERROR_BUFFER_TOO_SMALL;
    }

    out[0] = '0';
    out[1] = '3';
    out[2] = '\r';
    out[3] = '\0';

    return OBD_OK;
}


/**
 * Parse a single DTC from two raw bytes.
 *
 * byte1 and byte2 together form the 16-bit DTC encoding.
 * We extract the category from the top 2 bits and format
 * the rest as a 4-digit hex number.
 */
static void parse_single_dtc(uint8_t byte1, uint8_t byte2, obd_dtc_t *dtc)
{
    /* Top 2 bits of byte1 → category (0-3 → P/C/B/U) */
    uint8_t cat_bits = (byte1 >> 6) & 0x03;

    /* The remaining 14 bits form the numeric code.
     * We reconstruct the 4-digit code from the individual nibbles. */
    uint8_t d2 = (byte1 >> 4) & 0x03;  /* bits 13-12: second digit */
    uint8_t d3 = byte1 & 0x0F;          /* bits 11-8: third digit */
    uint8_t d4 = (byte2 >> 4) & 0x0F;   /* bits 7-4: fourth digit */
    uint8_t d5 = byte2 & 0x0F;          /* bits 3-0: fifth digit */

    dtc->category = (obd_dtc_category_t)cat_bits;
    dtc->code = ((uint16_t)d2 << 12) | ((uint16_t)d3 << 8) |
                ((uint16_t)d4 << 4) | d5;

    /* Format as "P0301" style string */
    dtc->formatted[0] = dtc_category_chars[cat_bits];
    dtc->formatted[1] = '0' + d2;
    dtc->formatted[2] = "0123456789ABCDEF"[d3];
    dtc->formatted[3] = "0123456789ABCDEF"[d4];
    dtc->formatted[4] = "0123456789ABCDEF"[d5];
    dtc->formatted[5] = '\0';
}


/*
 * Parse a Mode 03 response into a list of DTCs.
 *
 * Response format: "43 XX XX XX XX XX XX"
 *   0x43 = response to Mode 03 (0x03 + 0x40)
 *   Then pairs of bytes, each pair = one DTC
 *   0x00 0x00 = padding (no more DTCs)
 *
 * The response might also have a count byte after 0x43 in some
 * implementations, but the most common format is just the header
 * followed by DTC byte pairs.
 */
obd_result_t obd_dtc_parse_response(const char *response, obd_dtc_list_t *out)
{
    uint8_t bytes[64];
    size_t byte_count = 0;
    size_t i;
    obd_result_t r;

    if (!response || !out) {
        return OBD_ERROR_INVALID_ARG;
    }

    memset(out, 0, sizeof(*out));

    r = obd_hex_to_bytes(response, bytes, sizeof(bytes), &byte_count);
    if (r != OBD_OK) {
        return r;
    }

    /* Need at least the header byte (0x43) */
    if (byte_count < 1) {
        return OBD_ERROR_PARSE_FAILED;
    }

    /* Verify response header is 0x43 (Mode 03 response) */
    if (bytes[0] != 0x43) {
        return OBD_ERROR_PARSE_FAILED;
    }

    /* Parse DTC pairs starting after the header byte.
     * Each DTC is 2 bytes. 0x00 0x00 = padding/no DTC. */
    i = 1;
    while (i + 1 < byte_count && out->count < OBD_MAX_DTCS) {
        uint8_t b1 = bytes[i];
        uint8_t b2 = bytes[i + 1];

        /* Skip 0x0000 padding (means "no more DTCs") */
        if (b1 == 0x00 && b2 == 0x00) {
            i += 2;
            continue;
        }

        parse_single_dtc(b1, b2, &out->dtcs[out->count]);
        out->count++;
        i += 2;
    }

    return OBD_OK;
}


obd_result_t obd_dtc_format(const obd_dtc_t *dtc, char *out, size_t out_size)
{
    if (!dtc || !out || out_size == 0) {
        return OBD_ERROR_INVALID_ARG;
    }

    if (out_size < OBD_DTC_CODE_LENGTH) {
        return OBD_ERROR_BUFFER_TOO_SMALL;
    }

    /* The formatted string is already stored in the dtc struct,
     * but this function lets you copy it to any buffer. */
    memcpy(out, dtc->formatted, OBD_DTC_CODE_LENGTH);

    return OBD_OK;
}
