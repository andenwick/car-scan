package com.carscan.app.bluetooth

/**
 * Interface for communicating with an ELM327 adapter.
 * Implementations: ElmMockAdapter (testing), future real Bluetooth adapter.
 */
interface ElmConnection {
    val isConnected: Boolean
    suspend fun connect()
    suspend fun disconnect()
    suspend fun sendCommand(command: String): String
}
