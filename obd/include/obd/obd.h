/**
 * obd.h — Single public API header for the OBD-II protocol library.
 *
 * Usage: #include <obd/obd.h>
 *
 * This library is pure parsing — it has NO I/O, NO Bluetooth, NO threads.
 * You send commands over Bluetooth yourself, then pass the raw response
 * strings to these functions to parse them into structured data.
 *
 * Flow:
 *   1. Use obd_elm327_cmd_*() to get command strings to send to the adapter
 *   2. Send those strings over Bluetooth (your job)
 *   3. Pass the adapter's response to the appropriate parse function
 *   4. Use obd_sensor_decode() to convert raw bytes to human-readable values
 */

#ifndef OBD_H
#define OBD_H

#include "obd_types.h"

#ifdef __cplusplus
extern "C" {
#endif


/* ═══════════════════════════════════════════════════════════════════════════
 *  Hex Utilities
 *
 *  The ELM327 adapter speaks in hex strings like "41 0C 1A F8".
 *  These functions convert between hex strings and byte arrays.
 * ═══════════════════════════════════════════════════════════════════════════ */

/**
 * Convert a hex string to a byte array.
 *
 * Input:  "41 0C 1A F8" (spaces are skipped automatically)
 * Output: {0x41, 0x0C, 0x1A, 0xF8}, out_len=4
 *
 * @param hex       Input hex string (spaces/tabs allowed between bytes)
 * @param out       Output byte buffer (caller-provided)
 * @param out_size  Size of output buffer in bytes
 * @param out_len   Receives the number of bytes written
 * @return OBD_OK or OBD_ERROR_*
 */
obd_result_t obd_hex_to_bytes(const char *hex, uint8_t *out, size_t out_size,
                              size_t *out_len);

/**
 * Convert a byte array to a hex string (uppercase, space-separated).
 *
 * Input:  {0x41, 0x0C}, len=2
 * Output: "41 0C"
 *
 * @param bytes     Input byte array
 * @param len       Number of bytes
 * @param out       Output string buffer (caller-provided)
 * @param out_size  Size of output buffer (need 3*len for "XX " format)
 * @return OBD_OK or OBD_ERROR_*
 */
obd_result_t obd_bytes_to_hex(const uint8_t *bytes, size_t len,
                              char *out, size_t out_size);

/**
 * Strip whitespace (spaces, tabs, \r, \n) from a string in-place.
 *
 * "41 0C 1A F8\r\n" → "410C1AF8"
 *
 * Useful for normalizing ELM327 responses before parsing.
 */
void obd_strip_whitespace(char *str);


/* ═══════════════════════════════════════════════════════════════════════════
 *  ELM327 AT Commands
 *
 *  The ELM327 is the Bluetooth adapter chip. Before talking OBD-II to the
 *  car, you need to initialize the adapter with AT commands.
 *
 *  Each function returns a pointer to a static string literal — no
 *  allocation, never free these. Just send them over Bluetooth as-is.
 * ═══════════════════════════════════════════════════════════════════════════ */

/** "ATZ\r" — Reset the adapter to factory defaults */
const char *obd_elm327_cmd_reset(void);

/** "ATE0\r" — Turn off command echo (adapter won't repeat what you send) */
const char *obd_elm327_cmd_echo_off(void);

/** "ATL0\r" — Turn off linefeeds (cleaner responses) */
const char *obd_elm327_cmd_linefeed_off(void);

/** "ATSP0\r" — Auto-detect the vehicle's OBD protocol */
const char *obd_elm327_cmd_protocol_auto(void);

/** "ATH1\r" — Show header bytes in responses (useful for multi-ECU) */
const char *obd_elm327_cmd_headers_on(void);

/** "ATH0\r" — Hide header bytes (cleaner for simple queries) */
const char *obd_elm327_cmd_headers_off(void);

/**
 * Classify an ELM327 response string.
 *
 * Looks at the raw text from the adapter and determines what kind of
 * response it is: data bytes, "OK", "NO DATA", error, or prompt.
 *
 * @param response  Raw response string from the ELM327
 * @return The response type classification
 */
obd_elm_response_type_t obd_elm327_classify_response(const char *response);

/**
 * Clean an ELM327 response by stripping echo, prompt, and whitespace.
 *
 * Raw adapter output often looks like: "010C\r41 0C 1A F8\r\r>"
 * This function extracts just the data part: "41 0C 1A F8"
 *
 * @param raw       Raw response from the adapter
 * @param out       Output buffer for the cleaned response
 * @param out_size  Size of output buffer
 * @return OBD_OK or OBD_ERROR_*
 */
obd_result_t obd_elm327_clean_response(const char *raw, char *out,
                                       size_t out_size);


/* ═══════════════════════════════════════════════════════════════════════════
 *  PID Request/Response (Mode 01 & 02)
 *
 *  PIDs (Parameter IDs) are how you ask the car for specific data.
 *  Mode 01 = live data, Mode 02 = freeze frame (snapshot at time of DTC).
 *
 *  Example: Mode 01, PID 0C = Engine RPM
 *           You send "010C\r", car responds "41 0C 1A F8"
 * ═══════════════════════════════════════════════════════════════════════════ */

/**
 * Build a PID request command string.
 *
 * @param mode  OBD mode (0x01 for live data, 0x02 for freeze frame)
 * @param pid   The PID to request (e.g., 0x0C for RPM)
 * @param out   Output buffer for the command (e.g., "010C\r")
 * @param out_size  Size of output buffer (OBD_MAX_COMMAND_LEN is enough)
 * @return OBD_OK or OBD_ERROR_*
 */
obd_result_t obd_pid_build_request(uint8_t mode, uint8_t pid,
                                   char *out, size_t out_size);

/**
 * Parse a PID response string into structured data.
 *
 * Input:  "41 0C 1A F8" (cleaned response, no echo/prompt)
 * Output: mode=0x41, pid=0x0C, data={0x1A, 0xF8}, data_len=2
 *
 * @param response  Cleaned hex response string
 * @param out       Output struct (caller-provided)
 * @return OBD_OK or OBD_ERROR_*
 */
obd_result_t obd_pid_parse_response(const char *response,
                                    obd_pid_response_t *out);


/* ═══════════════════════════════════════════════════════════════════════════
 *  Sensor Decoding
 *
 *  After parsing a PID response, these functions apply the standard OBD-II
 *  formulas to convert raw bytes into engineering units.
 *
 *  Example: PID 0x0C, data={0x1A, 0xF8}
 *           Formula: ((A*256)+B)/4 = ((26*256)+248)/4 = 1726.0 RPM
 * ═══════════════════════════════════════════════════════════════════════════ */

/**
 * Decode a PID response into a human-readable sensor value.
 *
 * @param response  Parsed PID response from obd_pid_parse_response()
 * @param out       Output sensor value with name, unit, and scaled value
 * @return OBD_OK or OBD_ERROR_UNKNOWN_PID if PID isn't in our table
 */
obd_result_t obd_sensor_decode(const obd_pid_response_t *response,
                               obd_sensor_value_t *out);

/**
 * Get the human-readable name of a PID (without decoding a value).
 *
 * @param pid       The PID number
 * @param name      Output buffer for the name string
 * @param name_size Size of output buffer
 * @return OBD_OK or OBD_ERROR_UNKNOWN_PID
 */
obd_result_t obd_sensor_get_name(uint8_t pid, char *name, size_t name_size);


/* ═══════════════════════════════════════════════════════════════════════════
 *  DTC (Diagnostic Trouble Codes) — Mode 03
 *
 *  When the check engine light is on, Mode 03 reads stored DTCs.
 *  Each DTC is 2 bytes that encode a 5-character code like "P0301".
 * ═══════════════════════════════════════════════════════════════════════════ */

/**
 * Build a Mode 03 (stored DTCs) request command.
 *
 * @param out       Output buffer (receives "03\r")
 * @param out_size  Size of output buffer
 * @return OBD_OK or OBD_ERROR_*
 */
obd_result_t obd_dtc_build_request(char *out, size_t out_size);

/**
 * Parse a Mode 03 response into a list of DTCs.
 *
 * Input:  "43 01 03 01 04 00 00" (Mode 03 response)
 *         0x43 = response header (0x40 + 0x03)
 *         Each DTC is 2 bytes: 0x0103 → P0103, 0x0104 → P0104
 *         0x0000 = padding (ignored)
 *
 * @param response  Cleaned hex response string
 * @param out       Output DTC list (caller-provided)
 * @return OBD_OK or OBD_ERROR_*
 */
obd_result_t obd_dtc_parse_response(const char *response,
                                    obd_dtc_list_t *out);

/**
 * Format a DTC struct into a human-readable string like "P0301".
 *
 * @param dtc   The DTC to format
 * @param out   Output buffer (at least OBD_DTC_CODE_LENGTH bytes)
 * @param out_size  Size of output buffer
 * @return OBD_OK or OBD_ERROR_*
 */
obd_result_t obd_dtc_format(const obd_dtc_t *dtc, char *out, size_t out_size);


/* ═══════════════════════════════════════════════════════════════════════════
 *  VIN (Vehicle Identification Number) — Mode 09
 *
 *  The VIN is a 17-character string that uniquely identifies the vehicle.
 *  Mode 09, PID 02 retrieves it. The response comes in multiple lines
 *  because 17 bytes doesn't fit in one OBD frame.
 * ═══════════════════════════════════════════════════════════════════════════ */

/**
 * Build a Mode 09 PID 02 (VIN) request command.
 *
 * @param out       Output buffer (receives "0902\r")
 * @param out_size  Size of output buffer
 * @return OBD_OK or OBD_ERROR_*
 */
obd_result_t obd_vin_build_request(char *out, size_t out_size);

/**
 * Parse a Mode 09 VIN response into a 17-character string.
 *
 * Handles multi-line responses like:
 *   "49 02 01 57 42 41 33\r\n49 02 02 42 35 46 4B\r\n..."
 *
 * @param response  Raw multi-line hex response
 * @param vin       Output buffer (at least OBD_VIN_LENGTH + 1 bytes)
 * @param vin_size  Size of output buffer
 * @return OBD_OK or OBD_ERROR_*
 */
obd_result_t obd_vin_parse_response(const char *response,
                                    char *vin, size_t vin_size);


#ifdef __cplusplus
}
#endif

#endif /* OBD_H */
