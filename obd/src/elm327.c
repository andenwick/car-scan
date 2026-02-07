/**
 * elm327.c — ELM327 adapter command building and response parsing.
 *
 * The ELM327 is a microcontroller chip inside the Bluetooth OBD adapter.
 * It acts as a bridge between Bluetooth and the car's OBD-II port.
 *
 * Before you can talk OBD-II to the car, you initialize the ELM327
 * with "AT" commands (inherited from old telephone modem days).
 *
 * After initialization, the adapter echoes back your commands, appends
 * the car's response, then shows a ">" prompt. This module handles
 * building those AT command strings and cleaning up the messy responses.
 */

#include "elm327.h"
#include "hex_utils.h"
#include <obd/obd.h>
#include <string.h>

/* ── AT command builders ─────────────────────────────────────────────────
 *
 * Each returns a pointer to a string literal. String literals live in
 * the binary's read-only data section — they exist for the entire
 * program lifetime. No malloc, no free, no worries.
 *
 * The \r (carriage return) at the end is the ELM327's "end of command"
 * marker. It's how the adapter knows you're done typing.
 */

const char *obd_elm327_cmd_reset(void)         { return "ATZ\r"; }
const char *obd_elm327_cmd_echo_off(void)      { return "ATE0\r"; }
const char *obd_elm327_cmd_linefeed_off(void)  { return "ATL0\r"; }
const char *obd_elm327_cmd_protocol_auto(void) { return "ATSP0\r"; }
const char *obd_elm327_cmd_headers_on(void)    { return "ATH1\r"; }
const char *obd_elm327_cmd_headers_off(void)   { return "ATH0\r"; }


/* ── Response classifier ─────────────────────────────────────────────────
 *
 * The ELM327 can respond with many different things. Before we try to
 * parse hex data, we need to know WHAT we got. This function checks
 * for known response patterns.
 *
 * We check the most specific patterns first (exact matches like "OK"),
 * then fall back to checking if it looks like hex data.
 */
obd_elm_response_type_t obd_elm327_classify_response(const char *response)
{
    const char *p;

    if (!response) {
        return OBD_ELM_RESPONSE_UNKNOWN;
    }

    /* Skip leading whitespace/control characters */
    p = response;
    while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') {
        p++;
    }

    /* Empty after stripping whitespace */
    if (*p == '\0') {
        return OBD_ELM_RESPONSE_UNKNOWN;
    }

    /* Check for the ">" prompt (adapter ready for next command).
     * The prompt might appear alone or at the end of other output. */
    if (*p == '>') {
        return OBD_ELM_RESPONSE_PROMPT;
    }

    /* Check for "OK" (AT command acknowledgement) */
    if (strncmp(p, "OK", 2) == 0) {
        return OBD_ELM_RESPONSE_OK;
    }

    /* Check for "NO DATA" (car didn't respond to the PID request) */
    if (strncmp(p, "NO DATA", 7) == 0) {
        return OBD_ELM_RESPONSE_NO_DATA;
    }

    /* Check for "?" (adapter didn't understand our command),
     * "ERROR", "UNABLE TO CONNECT", "BUS INIT" errors, etc. */
    if (*p == '?') {
        return OBD_ELM_RESPONSE_ERROR;
    }
    if (strncmp(p, "ERROR", 5) == 0 ||
        strncmp(p, "UNABLE TO CONNECT", 17) == 0 ||
        strncmp(p, "BUS INIT", 8) == 0 ||
        strncmp(p, "CAN ERROR", 9) == 0 ||
        strncmp(p, "STOPPED", 7) == 0) {
        return OBD_ELM_RESPONSE_ERROR;
    }

    /* Check for ELM version string (response to ATZ reset).
     * Starts with "ELM" or "ELM327" — treat as OK since it means
     * the adapter successfully reset and is telling us its version. */
    if (strncmp(p, "ELM", 3) == 0) {
        return OBD_ELM_RESPONSE_OK;
    }

    /* If it starts with a hex digit, assume it's data.
     * OBD responses always start with hex bytes like "41 0C..." */
    if (hex_char_to_nibble(*p) >= 0) {
        return OBD_ELM_RESPONSE_DATA;
    }

    return OBD_ELM_RESPONSE_UNKNOWN;
}


/* ── Response cleaner ────────────────────────────────────────────────────
 *
 * Raw adapter output looks like this:
 *   "010C\r41 0C 1A F8\r\r>"
 *
 * Breaking that down:
 *   "010C\r"        — echo of the command we sent
 *   "41 0C 1A F8"   — the actual data we want
 *   "\r\r"          — trailing carriage returns
 *   ">"             — prompt character
 *
 * This function extracts just "41 0C 1A F8" by:
 *   1. Finding the first line that looks like OBD data (starts with hex)
 *   2. Copying data lines, stopping at prompt or non-data lines
 *   3. Trimming trailing whitespace
 *
 * Multi-line responses (like VIN) are preserved with \r separators.
 */
obd_result_t obd_elm327_clean_response(const char *raw, char *out,
                                       size_t out_size)
{
    const char *p;
    const char *line_start;
    size_t out_pos;
    int found_data;

    if (!raw || !out || out_size == 0) {
        return OBD_ERROR_INVALID_ARG;
    }

    out_pos = 0;
    found_data = 0;
    p = raw;

    while (*p != '\0') {
        /* Find start of current line (skip leading \r \n) */
        while (*p == '\r' || *p == '\n') {
            p++;
        }
        if (*p == '\0' || *p == '>') break;

        line_start = p;

        /* Find end of current line */
        while (*p != '\0' && *p != '\r' && *p != '\n' && *p != '>') {
            p++;
        }

        /* We have a line from line_start to p. Check if it's data. */
        {
            /* Skip lines that are echoed commands (AT commands or short
             * hex that matches what we sent), "OK", "NO DATA", etc.
             * Data lines start with a valid OBD response byte (0x40+). */
            const char *lp = line_start;
            obd_elm_response_type_t type;

            /* Skip leading spaces in the line */
            while (lp < p && (*lp == ' ' || *lp == '\t')) {
                lp++;
            }

            if (lp >= p) {
                continue; /* Empty line */
            }

            /* Classify this individual line */
            {
                /* Make a temporary null-terminated copy of the line */
                size_t line_len = (size_t)(p - lp);
                char line_buf[OBD_MAX_RESPONSE_LEN];
                if (line_len >= sizeof(line_buf)) {
                    line_len = sizeof(line_buf) - 1;
                }
                memcpy(line_buf, lp, line_len);
                line_buf[line_len] = '\0';
                type = obd_elm327_classify_response(line_buf);
            }

            if (type == OBD_ELM_RESPONSE_DATA) {
                /* Filter out echoed commands. OBD responses always have
                 * a mode byte >= 0x40 (response = request + 0x40).
                 * Echoed requests have mode < 0x40 (01, 02, 03, 09).
                 * So if the first byte < 0x40, it's an echo, skip it.
                 *
                 * Parse the first two hex chars to get the first byte. */
                int hi = hex_char_to_nibble(lp[0]);
                int lo = (lp + 1 < p) ? hex_char_to_nibble(lp[1]) : -1;
                if (hi < 0 || lo < 0) {
                    continue; /* Not a valid hex byte pair, skip */
                }
                {
                    int first_byte = (hi << 4) | lo;
                    if (first_byte < 0x40) {
                        continue; /* Echo of our request command, skip */
                    }
                }

                {
                size_t line_len = (size_t)(p - lp);

                /* Add \r separator between multi-line data */
                if (found_data && out_pos < out_size - 1) {
                    out[out_pos++] = '\r';
                }

                /* Copy the data line to output */
                if (out_pos + line_len >= out_size) {
                    return OBD_ERROR_BUFFER_TOO_SMALL;
                }
                memcpy(out + out_pos, lp, line_len);
                out_pos += line_len;
                found_data = 1;
                }
            }
            /* Non-data lines (echo, OK, prompt, etc.) are skipped */
        }
    }

    if (!found_data) {
        /* Check if the raw response indicates a known non-data condition */
        if (strstr(raw, "NO DATA") != NULL) {
            out[0] = '\0';
            return OBD_ERROR_NO_DATA;
        }
        if (strstr(raw, "?") != NULL || strstr(raw, "ERROR") != NULL) {
            out[0] = '\0';
            return OBD_ERROR_ELM_ERROR;
        }
        out[0] = '\0';
        return OBD_ERROR_PARSE_FAILED;
    }

    /* Null-terminate and trim trailing spaces */
    out[out_pos] = '\0';
    while (out_pos > 0 && (out[out_pos - 1] == ' ' || out[out_pos - 1] == '\t')) {
        out_pos--;
        out[out_pos] = '\0';
    }

    return OBD_OK;
}
