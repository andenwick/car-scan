package com.carscan.app.ui

import android.content.Intent
import android.graphics.Typeface
import android.os.Bundle
import android.view.Gravity
import android.widget.Button
import android.widget.LinearLayout
import android.widget.ScrollView
import android.widget.TextView
import androidx.appcompat.app.AppCompatActivity
import androidx.lifecycle.lifecycleScope
import com.carscan.app.bluetooth.ElmMockAdapter
import com.carscan.app.obd.ObdNative
import com.carscan.app.obd.SensorValue
import kotlinx.coroutines.delay
import kotlinx.coroutines.isActive
import kotlinx.coroutines.launch

/**
 * Live sensor data display.
 * Polls mock adapter for RPM, speed, coolant temp, and throttle position.
 */
class LiveDataActivity : AppCompatActivity() {

    private val adapter = ElmMockAdapter()
    private val sensorViews = mutableMapOf<Int, TextView>()

    // PIDs to poll: RPM, Speed, Coolant, Throttle
    private val pollPids = listOf(0x0C, 0x0D, 0x05, 0x11)

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        val scroll = ScrollView(this).apply {
            setBackgroundColor(0xFF1A1A2E.toInt())
        }

        val layout = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            setPadding(32, 32, 32, 32)
        }

        // Title
        layout.addView(TextView(this).apply {
            text = "Live Data"
            textSize = 22f
            setTextColor(0xFFE0E0E0.toInt())
            gravity = Gravity.CENTER
            setPadding(0, 0, 0, 24)
        })

        // Create a card for each sensor
        for (pid in pollPids) {
            val name = ObdNative.getSensorName(pid) ?: "PID 0x${pid.toString(16)}"
            val card = createSensorCard(pid, name)
            layout.addView(card)
        }

        // Buttons row
        val btnRow = LinearLayout(this).apply {
            orientation = LinearLayout.HORIZONTAL
            gravity = Gravity.CENTER
            setPadding(0, 24, 0, 0)
        }

        btnRow.addView(Button(this).apply {
            text = "Read DTCs"
            setOnClickListener {
                startActivity(Intent(this@LiveDataActivity, DtcActivity::class.java))
            }
        })

        layout.addView(btnRow)
        scroll.addView(layout)
        setContentView(scroll)

        // Connect and start polling
        lifecycleScope.launch {
            adapter.connect()
            pollSensors()
        }
    }

    private fun createSensorCard(pid: Int, name: String): LinearLayout {
        val card = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            setBackgroundColor(0xFF16213E.toInt())
            setPadding(24, 16, 24, 16)
            val params = LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                LinearLayout.LayoutParams.WRAP_CONTENT
            )
            params.bottomMargin = 16
            layoutParams = params
        }

        card.addView(TextView(this).apply {
            text = name
            textSize = 14f
            setTextColor(0xFF808080.toInt())
        })

        val valueView = TextView(this).apply {
            text = "--"
            textSize = 28f
            setTextColor(0xFF00E676.toInt()) // Green for normal values
            typeface = Typeface.MONOSPACE
        }
        sensorViews[pid] = valueView
        card.addView(valueView)

        return card
    }

    private suspend fun pollSensors() {
        while (lifecycleScope.coroutineContext.isActive) {
            for (pid in pollPids) {
                val request = ObdNative.buildPidRequest(0x01, pid) ?: continue
                val raw = adapter.sendCommand(request)
                val cleaned = ObdNative.cleanResponse(raw) ?: continue
                val sensor = ObdNative.decodeSensor(cleaned) ?: continue

                sensorViews[pid]?.text = formatSensor(sensor)
            }
            delay(500) // Poll every 500ms
        }
    }

    private fun formatSensor(s: SensorValue): String {
        return if (s.value == s.value.toLong().toFloat()) {
            "${s.value.toLong()} ${s.unit}"
        } else {
            "${"%.1f".format(s.value)} ${s.unit}"
        }
    }
}
