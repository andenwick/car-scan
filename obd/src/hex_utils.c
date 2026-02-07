/**
 * hex_utils.c — Hex string <-> byte array conversion.
 *
 * The ELM327 adapter speaks in hex text strings like "41 0C 1A F8".
 * We need to convert these to actual byte arrays for math, and back
 * to strings for display. These are the lowest-level utility functions
 * that every other module depends on.
 */

#include "hex_utils.h"
#include <obd/obd.h>
#include <string.h>

/* Convert a single hex char to its numeric value (0-15).
 * Returns -1 for invalid characters. */
int hex_char_to_nibble(char c)
{
    if (c >= '0' && c <= '9') return c - '0';        /* '0'=0 ... '9'=9 */
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;   /* 'A'=10 ... 'F'=15 */
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;   /* 'a'=10 ... 'f'=15 */
    return -1;
}

int is_whitespace(char c)
{
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

/*
 * Convert hex string to byte array.
 *
 * Walk through the string character by character:
 *   - Skip whitespace (spaces between hex pairs)
 *   - Take two hex characters at a time (high nibble + low nibble)
 *   - Combine them into one byte: high*16 + low
 *
 * "41 0C" → skip space → '4','1' = 0x41, '0','C' = 0x0C
 */
obd_result_t obd_hex_to_bytes(const char *hex, uint8_t *out, size_t out_size,
                              size_t *out_len)
{
    size_t i, o;
    int high, low;

    if (!hex || !out || !out_len) {
        return OBD_ERROR_INVALID_ARG;
    }

    o = 0;
    i = 0;
    while (hex[i] != '\0') {
        /* Skip whitespace between hex byte pairs */
        if (is_whitespace(hex[i])) {
            i++;
            continue;
        }

        /* We need two hex characters for one byte */
        high = hex_char_to_nibble(hex[i]);
        if (high < 0) {
            return OBD_ERROR_INVALID_HEX;
        }

        i++;
        if (hex[i] == '\0') {
            /* Odd number of hex characters — trailing nibble */
            return OBD_ERROR_INVALID_HEX;
        }

        low = hex_char_to_nibble(hex[i]);
        if (low < 0) {
            return OBD_ERROR_INVALID_HEX;
        }
        i++;

        /* Check output buffer space */
        if (o >= out_size) {
            return OBD_ERROR_BUFFER_TOO_SMALL;
        }

        /* Combine high and low nibbles into one byte:
         * high nibble goes in upper 4 bits, low nibble in lower 4 bits.
         * Example: high=4, low=1 → 4*16 + 1 = 0x41 */
        out[o] = (uint8_t)((high << 4) | low);
        o++;
    }

    *out_len = o;
    return OBD_OK;
}

/*
 * Convert byte array to hex string (uppercase, space-separated).
 *
 * Each byte becomes two hex chars plus a space: "41 0C 1A F8"
 * (No trailing space — the last byte doesn't get one.)
 *
 * Buffer size needed: len*3 for "XX " per byte, but last byte has no space,
 * so len*3 - 1 + 1 (null terminator) = len*3. But we check len*3 to be safe.
 */
obd_result_t obd_bytes_to_hex(const uint8_t *bytes, size_t len,
                              char *out, size_t out_size)
{
    /* Lookup table: index → hex character. Faster than computing each time. */
    static const char hex_chars[] = "0123456789ABCDEF";
    size_t needed;
    size_t i, pos;

    if (!bytes || !out) {
        return OBD_ERROR_INVALID_ARG;
    }

    if (len == 0) {
        if (out_size > 0) out[0] = '\0';
        return OBD_OK;
    }

    /* Need "XX" per byte + " " separator between bytes + null terminator */
    needed = len * 3; /* "XX " = 3 chars per byte, last space becomes '\0' */
    if (out_size < needed) {
        return OBD_ERROR_BUFFER_TOO_SMALL;
    }

    pos = 0;
    for (i = 0; i < len; i++) {
        /* Upper nibble: shift right 4 bits to get high hex digit */
        out[pos++] = hex_chars[(bytes[i] >> 4) & 0x0F];
        /* Lower nibble: mask with 0x0F to get low hex digit */
        out[pos++] = hex_chars[bytes[i] & 0x0F];

        /* Add space separator (but not after the last byte) */
        if (i < len - 1) {
            out[pos++] = ' ';
        }
    }
    out[pos] = '\0';

    return OBD_OK;
}

/*
 * Strip all whitespace from a string in-place.
 *
 * Uses a two-pointer technique:
 *   - 'read' walks through every character
 *   - 'write' only advances for non-whitespace characters
 *   - At the end, write < read (or equal), so the string is shorter/same
 *
 * "41 0C\r\n" → "410C" (in the same buffer, no allocation)
 */
void obd_strip_whitespace(char *str)
{
    size_t read, write;

    if (!str) return;

    write = 0;
    for (read = 0; str[read] != '\0'; read++) {
        if (!is_whitespace(str[read])) {
            str[write] = str[read];
            write++;
        }
    }
    str[write] = '\0';
}
