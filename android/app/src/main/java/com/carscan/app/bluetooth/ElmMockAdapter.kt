package com.carscan.app.bluetooth

import kotlinx.coroutines.delay

/**
 * Mock ELM327 adapter for testing without real Bluetooth hardware.
 * Returns realistic OBD-II responses with a small simulated delay.
 */
class ElmMockAdapter : ElmConnection {

    override var isConnected: Boolean = false
        private set

    override suspend fun connect() {
        delay(200)
        isConnected = true
    }

    override suspend fun disconnect() {
        isConnected = false
    }

    override suspend fun sendCommand(command: String): String {
        delay(80) // Simulate adapter response time

        val cmd = command.trimEnd('\r', '\n', ' ')

        return when {
            // AT commands
            cmd.equals("ATZ", ignoreCase = true) ->
                "ATZ\r\rELM327 v1.5\r\r>"
            cmd.equals("ATE0", ignoreCase = true) ->
                "ATE0\rOK\r\r>"
            cmd.equals("ATL0", ignoreCase = true) ->
                "OK\r\r>"
            cmd.equals("ATSP0", ignoreCase = true) ->
                "OK\r\r>"
            cmd.equals("ATH0", ignoreCase = true) ->
                "OK\r\r>"
            cmd.equals("ATH1", ignoreCase = true) ->
                "OK\r\r>"

            // Mode 01 PID requests (live sensor data)
            cmd.equals("010C", ignoreCase = true) ->
                "010C\r41 0C 1A F8\r\r>"   // RPM: 1726
            cmd.equals("010D", ignoreCase = true) ->
                "010D\r41 0D 3C\r\r>"       // Speed: 60 km/h
            cmd.equals("0105", ignoreCase = true) ->
                "0105\r41 05 7B\r\r>"       // Coolant: 83 C
            cmd.equals("0111", ignoreCase = true) ->
                "0111\r41 11 33\r\r>"       // Throttle: 20%
            cmd.equals("010F", ignoreCase = true) ->
                "010F\r41 0F 46\r\r>"       // Intake temp: 30 C
            cmd.equals("0110", ignoreCase = true) ->
                "0110\r41 10 01 A4\r\r>"    // MAF: 4.20 g/s
            cmd.equals("010A", ignoreCase = true) ->
                "010A\r41 0A 64\r\r>"       // Fuel pressure: 300 kPa
            cmd.equals("0104", ignoreCase = true) ->
                "0104\r41 04 4C\r\r>"       // Engine load: 29.8%

            // Mode 03: stored DTCs
            cmd.equals("03", ignoreCase = true) ->
                "03\r43 01 03 01 04 00 00\r\r>"  // P0103, P0104

            // Mode 09 PID 02: VIN
            cmd.equals("0902", ignoreCase = true) ->
                "0902\r" +
                "49 02 01 57 42 41 33\r" +
                "49 02 02 42 35 46 4B\r" +
                "49 02 03 37 46 4E 31\r" +
                "49 02 04 32 33 34 35\r" +
                "49 02 05 36 00 00 00\r\r>"

            // Unknown command
            else -> "$cmd\rNO DATA\r\r>"
        }
    }
}
