/**
 * obd_types.h — All public types, enums, and structs for the OBD-II library.
 *
 * Design philosophy:
 *   - Zero malloc: every struct is meant to be stack-allocated by the caller.
 *   - Fixed-size buffers: we know the max sizes from the OBD-II spec, so we
 *     hardcode them. This avoids dynamic memory entirely.
 *   - Simple error codes: every function returns obd_result_t so the caller
 *     always knows if something went wrong and why.
 */

#ifndef OBD_TYPES_H
#define OBD_TYPES_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── Error codes ─────────────────────────────────────────────────────────────
 *
 * Every public function returns one of these. Negative = error, zero = OK.
 * The caller checks the return value before using any output parameters.
 */
typedef enum {
    OBD_OK                  =  0,  /* Success */
    OBD_ERROR_INVALID_ARG   = -1,  /* NULL pointer or bad parameter */
    OBD_ERROR_BUFFER_TOO_SMALL = -2,  /* Output buffer not big enough */
    OBD_ERROR_INVALID_HEX   = -3,  /* Non-hex character in input */
    OBD_ERROR_NO_DATA       = -4,  /* ELM327 responded "NO DATA" */
    OBD_ERROR_ELM_ERROR     = -5,  /* ELM327 responded with "?" or error */
    OBD_ERROR_PARSE_FAILED  = -6,  /* Couldn't parse response format */
    OBD_ERROR_UNKNOWN_PID   = -7,  /* PID not in our lookup table */
} obd_result_t;


/* ── ELM327 response types ──────────────────────────────────────────────────
 *
 * When the ELM327 adapter sends a response, it could be:
 *   - Actual OBD data (hex bytes like "41 0C 1A F8")
 *   - "NO DATA" (vehicle didn't respond to that PID)
 *   - "?" (adapter didn't understand our command)
 *   - "OK" (AT command succeeded)
 *   - An echo of the command we sent (if echo wasn't disabled)
 *   - The ">" prompt meaning the adapter is ready for the next command
 */
typedef enum {
    OBD_ELM_RESPONSE_DATA,      /* Got hex data bytes */
    OBD_ELM_RESPONSE_OK,        /* AT command acknowledged */
    OBD_ELM_RESPONSE_NO_DATA,   /* Vehicle didn't respond */
    OBD_ELM_RESPONSE_ERROR,     /* ELM reported an error */
    OBD_ELM_RESPONSE_PROMPT,    /* ">" ready for next command */
    OBD_ELM_RESPONSE_UNKNOWN,   /* Couldn't classify this response */
} obd_elm_response_type_t;


/* ── Max sizes ───────────────────────────────────────────────────────────────
 *
 * These come from the OBD-II and ELM327 specs:
 *   - Max 7 data bytes per OBD response frame
 *   - VIN is always exactly 17 characters
 *   - DTC code is 5 chars like "P0301" + null terminator
 *   - ELM327 responses are usually under 256 bytes
 *   - A single Mode 03 response can return at most ~32 DTCs realistically
 */
#define OBD_MAX_DATA_BYTES      7
#define OBD_VIN_LENGTH         17
#define OBD_DTC_CODE_LENGTH     6   /* "P0301" + '\0' */
#define OBD_MAX_DTCS           32
#define OBD_MAX_RESPONSE_LEN  256
#define OBD_MAX_COMMAND_LEN    16


/* ── PID response ────────────────────────────────────────────────────────────
 *
 * When you ask the car "what's your RPM?" (Mode 01, PID 0C), the raw
 * response looks like "41 0C 1A F8". This struct holds the parsed pieces:
 *   - mode: 0x41 means "response to Mode 01" (0x40 + mode number)
 *   - pid:  0x0C is the PID we asked about
 *   - data: {0x1A, 0xF8} are the actual value bytes
 *   - data_len: how many value bytes (varies by PID, 1-4 typically)
 */
typedef struct {
    uint8_t mode;
    uint8_t pid;
    uint8_t data[OBD_MAX_DATA_BYTES];
    size_t  data_len;
} obd_pid_response_t;


/* ── Sensor value ────────────────────────────────────────────────────────────
 *
 * After parsing the raw bytes, we apply a formula to get a human-readable
 * value. For example, RPM bytes {0x1A, 0xF8} become 1726.0 RPM using the
 * formula ((A*256)+B)/4.
 *
 * The name and unit come from a lookup table built into the library.
 */
typedef struct {
    uint8_t pid;
    float   value;
    char    name[32];     /* e.g., "Engine RPM" */
    char    unit[8];      /* e.g., "rpm" */
} obd_sensor_value_t;


/* ── DTC categories ──────────────────────────────────────────────────────────
 *
 * Every DTC (Diagnostic Trouble Code) starts with a letter:
 *   P = Powertrain (engine, transmission) — most common
 *   C = Chassis (ABS, steering)
 *   B = Body (airbags, windows, seats)
 *   U = Network (CAN bus communication)
 *
 * The first 2 bits of the raw DTC bytes determine the category:
 *   00 = P, 01 = C, 10 = B, 11 = U
 */
typedef enum {
    OBD_DTC_POWERTRAIN = 0,  /* P codes — engine/transmission */
    OBD_DTC_CHASSIS    = 1,  /* C codes — ABS, steering */
    OBD_DTC_BODY       = 2,  /* B codes — airbags, windows */
    OBD_DTC_NETWORK    = 3,  /* U codes — CAN bus */
} obd_dtc_category_t;


/* ── DTC struct ──────────────────────────────────────────────────────────────
 *
 * A single diagnostic trouble code. The raw 2-byte value from the car gets
 * parsed into a category + numeric code, then formatted as a string like
 * "P0301" (cylinder 1 misfire).
 */
typedef struct {
    obd_dtc_category_t category;
    uint16_t           code;            /* Numeric part, e.g., 0x0301 */
    char               formatted[OBD_DTC_CODE_LENGTH]; /* "P0301\0" */
} obd_dtc_t;


/* ── DTC list ────────────────────────────────────────────────────────────────
 *
 * Mode 03 can return multiple DTCs at once. This struct holds them all.
 * count tells you how many entries in dtcs[] are valid.
 */
typedef struct {
    obd_dtc_t dtcs[OBD_MAX_DTCS];
    size_t    count;
} obd_dtc_list_t;


#ifdef __cplusplus
}
#endif

#endif /* OBD_TYPES_H */
