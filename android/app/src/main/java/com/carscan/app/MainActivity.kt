package com.carscan.app

import android.content.Intent
import android.os.Bundle
import android.widget.Button
import android.widget.TextView
import android.widget.LinearLayout
import android.view.Gravity
import androidx.appcompat.app.AppCompatActivity
import androidx.lifecycle.lifecycleScope
import com.carscan.app.bluetooth.ElmMockAdapter
import com.carscan.app.obd.ObdNative
import com.carscan.app.ui.LiveDataActivity
import kotlinx.coroutines.launch

/**
 * Main screen: connect to adapter (mock) and navigate to live data.
 */
class MainActivity : AppCompatActivity() {

    private val adapter = ElmMockAdapter()
    private lateinit var statusText: TextView

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        val layout = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            gravity = Gravity.CENTER
            setPadding(48, 48, 48, 48)
            setBackgroundColor(0xFF1A1A2E.toInt())
        }

        statusText = TextView(this).apply {
            text = "CarScan OBD-II"
            textSize = 24f
            setTextColor(0xFFE0E0E0.toInt())
            gravity = Gravity.CENTER
        }

        val connectBtn = Button(this).apply {
            text = "Connect (Mock)"
            textSize = 18f
            setOnClickListener { onConnect() }
        }

        val versionText = TextView(this).apply {
            text = "Native lib: ${ObdNative.cmdReset().trim()}"
            textSize = 12f
            setTextColor(0xFF808080.toInt())
            gravity = Gravity.CENTER
        }

        layout.addView(statusText, lp())
        layout.addView(connectBtn, lp().apply { topMargin = 48 })
        layout.addView(versionText, lp().apply { topMargin = 24 })

        setContentView(layout)
    }

    private fun onConnect() {
        statusText.text = "Connecting..."
        lifecycleScope.launch {
            adapter.connect()

            // Run ELM327 init sequence
            val cmds = listOf(
                ObdNative.cmdReset(),
                ObdNative.cmdEchoOff(),
                ObdNative.cmdLinefeedOff(),
                ObdNative.cmdProtocolAuto()
            )
            for (cmd in cmds) {
                adapter.sendCommand(cmd)
            }

            statusText.text = "Connected"
            startActivity(Intent(this@MainActivity, LiveDataActivity::class.java))
        }
    }

    private fun lp() = LinearLayout.LayoutParams(
        LinearLayout.LayoutParams.WRAP_CONTENT,
        LinearLayout.LayoutParams.WRAP_CONTENT
    ).apply { gravity = Gravity.CENTER_HORIZONTAL }
}
