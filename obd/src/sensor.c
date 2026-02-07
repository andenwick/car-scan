/**
 * sensor.c — OBD-II sensor value decoding (raw bytes → engineering units).
 *
 * Each PID has a formula defined by the SAE J1979 standard. The raw data
 * bytes (conventionally called A, B, C, D) get plugged into the formula
 * to produce a human-readable value.
 *
 * For example, PID 0x0C (Engine RPM):
 *   Raw bytes: A=0x1A (26), B=0xF8 (248)
 *   Formula:   ((A * 256) + B) / 4
 *   Result:    ((26 * 256) + 248) / 4 = 6904 / 4 = 1726.0 RPM
 *
 * All formulas are stored in a lookup table. To add a new PID, just add
 * an entry to the table — no code changes needed.
 */

#include "sensor.h"
#include <obd/obd.h>
#include <string.h>

/* ── Formula function type ───────────────────────────────────────────────
 *
 * Each PID has a function that takes the raw data bytes and returns a float.
 * We use function pointers in the lookup table to call the right formula.
 *
 * Parameters:
 *   data     — raw bytes from the PID response (A, B, C, D)
 *   data_len — how many bytes are available
 *
 * Returns: the decoded value as a float.
 */
typedef float (*sensor_formula_fn)(const uint8_t *data, size_t data_len);


/* ── Individual formula functions ────────────────────────────────────────
 *
 * Each implements one formula from the OBD-II spec.
 * The naming convention is: formula_<what it calculates>
 *
 * Contract: these functions assume data_len >= byte_count has been
 * validated by obd_sensor_decode() before calling. Do not call directly.
 */

/* A * 100 / 255 — Percentage (0-100%)
 * Used for: engine load, throttle position, EGR, etc. */
static float formula_percent(const uint8_t *data, size_t data_len)
{
    (void)data_len;
    return (float)data[0] * 100.0f / 255.0f;
}

/* A - 40 — Temperature with -40 offset (range: -40 to 215°C)
 * Used for: coolant temp, intake air temp.
 * The spec uses +40 offset so a single unsigned byte covers -40°C to 215°C. */
static float formula_temp_offset40(const uint8_t *data, size_t data_len)
{
    (void)data_len;
    return (float)data[0] - 40.0f;
}

/* ((A * 256) + B) / 4 — Engine RPM (0 to 16383.75 rpm)
 * Why /4? The car reports RPM in quarter-revolution increments for precision. */
static float formula_rpm(const uint8_t *data, size_t data_len)
{
    (void)data_len;
    return ((float)data[0] * 256.0f + (float)data[1]) / 4.0f;
}

/* A — Direct value (0 to 255 km/h)
 * Used for: vehicle speed. The value is already in km/h. */
static float formula_direct(const uint8_t *data, size_t data_len)
{
    (void)data_len;
    return (float)data[0];
}

/* A / 2 - 64 — Timing advance (-64 to 63.5 degrees before TDC)
 * TDC = Top Dead Center, the reference point for ignition timing. */
static float formula_timing_advance(const uint8_t *data, size_t data_len)
{
    (void)data_len;
    return (float)data[0] / 2.0f - 64.0f;
}

/* ((A * 256) + B) / 100 — MAF air flow rate (0 to 655.35 g/s)
 * MAF = Mass Air Flow sensor, measures how much air enters the engine. */
static float formula_maf(const uint8_t *data, size_t data_len)
{
    (void)data_len;
    return ((float)data[0] * 256.0f + (float)data[1]) / 100.0f;
}

/* A * 3 — Fuel gauge pressure (0 to 765 kPa)
 * kPa = kilopascals, a unit of pressure. */
static float formula_fuel_pressure(const uint8_t *data, size_t data_len)
{
    (void)data_len;
    return (float)data[0] * 3.0f;
}

/* A / 200 — O2 sensor voltage (0 to 1.275 V)
 * O2 sensors measure exhaust oxygen to tune the air/fuel mixture. */
static float formula_o2_voltage(const uint8_t *data, size_t data_len)
{
    (void)data_len;
    return (float)data[0] / 200.0f;
}

/* ((A * 256) + B) * 0.079577 — Runtime in seconds → minutes
 * Actually: raw value is in seconds. We report seconds directly. */
static float formula_runtime(const uint8_t *data, size_t data_len)
{
    (void)data_len;
    return (float)data[0] * 256.0f + (float)data[1];
}

/* A — Number of DTCs + MIL status (encoded in PID 0x01)
 * Bit 7 of A = MIL on/off, bits 0-6 = number of DTCs.
 * We return just the DTC count for simplicity. */
static float formula_dtc_count(const uint8_t *data, size_t data_len)
{
    (void)data_len;
    return (float)(data[0] & 0x7F);  /* Mask off bit 7 (MIL indicator) */
}

/* ((A - 128) * 100) / 128 — Fuel trim percentage (-100% to 99.2%)
 * Fuel trim = how much the ECU adjusts fuel delivery from the base map.
 * Negative = running rich (too much fuel), Positive = running lean. */
static float formula_fuel_trim(const uint8_t *data, size_t data_len)
{
    (void)data_len;
    return ((float)data[0] - 128.0f) * 100.0f / 128.0f;
}


/* ── Sensor lookup table ─────────────────────────────────────────────────
 *
 * Each entry maps a PID number to its name, unit, byte count, and formula.
 * To support a new PID, just add a row — no other code changes needed.
 *
 * byte_count is how many data bytes the PID response contains.
 * This is used for validation (making sure we got enough bytes).
 */
typedef struct {
    uint8_t pid;
    const char *name;
    const char *unit;
    uint8_t byte_count;  /* Number of data bytes in response */
    sensor_formula_fn formula;
} sensor_entry_t;

static const sensor_entry_t sensor_table[] = {
    /* PID   Name                          Unit     Bytes  Formula */
    { 0x01, "DTC Count",                  "",       4,    formula_dtc_count },
    { 0x04, "Engine Load",                "%",      1,    formula_percent },
    { 0x05, "Coolant Temperature",        "C",      1,    formula_temp_offset40 },
    { 0x06, "Short Term Fuel Trim B1",    "%",      1,    formula_fuel_trim },
    { 0x07, "Long Term Fuel Trim B1",     "%",      1,    formula_fuel_trim },
    { 0x0A, "Fuel Pressure",              "kPa",    1,    formula_fuel_pressure },
    { 0x0B, "Intake Manifold Pressure",   "kPa",    1,    formula_direct },
    { 0x0C, "Engine RPM",                 "rpm",    2,    formula_rpm },
    { 0x0D, "Vehicle Speed",              "km/h",   1,    formula_direct },
    { 0x0E, "Timing Advance",             "deg",    1,    formula_timing_advance },
    { 0x0F, "Intake Air Temperature",     "C",      1,    formula_temp_offset40 },
    { 0x10, "MAF Air Flow Rate",          "g/s",    2,    formula_maf },
    { 0x11, "Throttle Position",          "%",      1,    formula_percent },
    { 0x14, "O2 Sensor 1 Voltage",        "V",      1,    formula_o2_voltage },
    { 0x1F, "Run Time Since Start",       "sec",    2,    formula_runtime },
};

/* Number of entries in the table (computed at compile time) */
#define SENSOR_TABLE_SIZE (sizeof(sensor_table) / sizeof(sensor_table[0]))


/*
 * Find a sensor entry by PID number.
 * Returns NULL if the PID isn't in our table.
 * Linear search is fine — the table is small (~15 entries).
 */
static const sensor_entry_t *find_sensor_entry(uint8_t pid)
{
    size_t i;
    for (i = 0; i < SENSOR_TABLE_SIZE; i++) {
        if (sensor_table[i].pid == pid) {
            return &sensor_table[i];
        }
    }
    return NULL;
}


/*
 * Decode a parsed PID response into a human-readable sensor value.
 *
 * Steps:
 *   1. Look up the PID in the sensor table
 *   2. Verify we have enough data bytes
 *   3. Call the formula function to compute the value
 *   4. Fill in name and unit from the table
 */
obd_result_t obd_sensor_decode(const obd_pid_response_t *response,
                               obd_sensor_value_t *out)
{
    const sensor_entry_t *entry;

    if (!response || !out) {
        return OBD_ERROR_INVALID_ARG;
    }

    entry = find_sensor_entry(response->pid);
    if (!entry) {
        return OBD_ERROR_UNKNOWN_PID;
    }

    if (response->data_len < entry->byte_count) {
        return OBD_ERROR_PARSE_FAILED;
    }

    memset(out, 0, sizeof(*out));
    out->pid = response->pid;
    out->value = entry->formula(response->data, response->data_len);

    /* Copy name and unit using strncpy for safety.
     * strncpy pads with zeros if the source is shorter than n.
     * We explicitly null-terminate in case the source is exactly n chars. */
    strncpy(out->name, entry->name, sizeof(out->name) - 1);
    out->name[sizeof(out->name) - 1] = '\0';
    strncpy(out->unit, entry->unit, sizeof(out->unit) - 1);
    out->unit[sizeof(out->unit) - 1] = '\0';

    return OBD_OK;
}


/*
 * Get just the name of a PID (without decoding a value).
 * Useful for building UI labels before you have data.
 */
obd_result_t obd_sensor_get_name(uint8_t pid, char *name, size_t name_size)
{
    const sensor_entry_t *entry;

    if (!name || name_size == 0) {
        return OBD_ERROR_INVALID_ARG;
    }

    entry = find_sensor_entry(pid);
    if (!entry) {
        return OBD_ERROR_UNKNOWN_PID;
    }

    strncpy(name, entry->name, name_size - 1);
    name[name_size - 1] = '\0';
    return OBD_OK;
}
