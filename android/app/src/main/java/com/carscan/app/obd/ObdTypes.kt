package com.carscan.app.obd

/**
 * Decoded sensor value from the OBD-II library.
 * Produced by ObdNative.decodeSensor().
 */
data class SensorValue(
    val pid: Int,
    val value: Float,
    val name: String,
    val unit: String
)

/**
 * A single Diagnostic Trouble Code.
 * Produced by ObdNative.parseDtcResponse().
 */
data class DtcCode(
    val category: Int,    // 0=P(Powertrain), 1=C(Chassis), 2=B(Body), 3=U(Network)
    val code: Int,        // Numeric part, e.g. 0x0301
    val formatted: String // "P0301"
)
