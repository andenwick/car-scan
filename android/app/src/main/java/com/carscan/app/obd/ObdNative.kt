package com.carscan.app.obd

/**
 * JNI bridge to the OBD-II C library.
 *
 * Each external fun maps to a C function in jni_bridge.c.
 * The native library is loaded once when this object is first accessed.
 */
object ObdNative {
    init {
        System.loadLibrary("carscan-native")
    }

    /* ── ELM327 AT commands ──────────────────────────────────────────── */

    /** "ATZ\r" -- Reset the adapter */
    external fun cmdReset(): String

    /** "ATE0\r" -- Turn off command echo */
    external fun cmdEchoOff(): String

    /** "ATL0\r" -- Turn off linefeeds */
    external fun cmdLinefeedOff(): String

    /** "ATSP0\r" -- Auto-detect OBD protocol */
    external fun cmdProtocolAuto(): String

    /** "ATH1\r" -- Show header bytes */
    external fun cmdHeadersOn(): String

    /** "ATH0\r" -- Hide header bytes */
    external fun cmdHeadersOff(): String

    /* ── Response handling ────────────────────────────────────────────── */

    /** Classify an ELM327 response string. Returns a RESPONSE_* constant. */
    external fun classifyResponse(response: String): Int

    /** Clean raw adapter response, returning just the data hex. Null on error. */
    external fun cleanResponse(raw: String): String?

    /* ── PID / Sensor ─────────────────────────────────────────────────── */

    /** Build a PID request command string. Null on error. */
    external fun buildPidRequest(mode: Int, pid: Int): String?

    /** Parse + decode a cleaned hex response into a SensorValue. Null on error. */
    external fun decodeSensor(cleanedHex: String): SensorValue?

    /** Get the human-readable name of a PID. Null if unknown. */
    external fun getSensorName(pid: Int): String?

    /* ── DTC ──────────────────────────────────────────────────────────── */

    /** Build a Mode 03 (stored DTCs) request command. Null on error. */
    external fun buildDtcRequest(): String?

    /** Parse a Mode 03 response into an array of DtcCode. */
    external fun parseDtcResponse(cleanedHex: String): Array<DtcCode>

    /* ── VIN ──────────────────────────────────────────────────────────── */

    /** Build a Mode 09 PID 02 (VIN) request command. Null on error. */
    external fun buildVinRequest(): String?

    /** Parse a VIN response into a 17-character string. Null on error. */
    external fun parseVinResponse(cleanedHex: String): String?

    /* ── Response type constants (match obd_elm_response_type_t) ──────── */

    const val RESPONSE_DATA = 0
    const val RESPONSE_OK = 1
    const val RESPONSE_NO_DATA = 2
    const val RESPONSE_ERROR = 3
    const val RESPONSE_PROMPT = 4
    const val RESPONSE_UNKNOWN = 5
}
