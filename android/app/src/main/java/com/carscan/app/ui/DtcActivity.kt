package com.carscan.app.ui

import android.graphics.Typeface
import android.os.Bundle
import android.view.Gravity
import android.widget.LinearLayout
import android.widget.ScrollView
import android.widget.TextView
import androidx.appcompat.app.AppCompatActivity
import androidx.lifecycle.lifecycleScope
import com.carscan.app.bluetooth.ElmMockAdapter
import com.carscan.app.obd.DtcCode
import com.carscan.app.obd.ObdNative
import kotlinx.coroutines.launch

/**
 * Display stored DTCs (Diagnostic Trouble Codes).
 */
class DtcActivity : AppCompatActivity() {

    private val adapter = ElmMockAdapter()
    private lateinit var contentLayout: LinearLayout

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        val scroll = ScrollView(this).apply {
            setBackgroundColor(0xFF1A1A2E.toInt())
        }

        contentLayout = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            setPadding(32, 32, 32, 32)
        }

        contentLayout.addView(TextView(this).apply {
            text = "Diagnostic Trouble Codes"
            textSize = 22f
            setTextColor(0xFFE0E0E0.toInt())
            gravity = Gravity.CENTER
            setPadding(0, 0, 0, 24)
        })

        contentLayout.addView(TextView(this).apply {
            text = "Scanning..."
            textSize = 16f
            setTextColor(0xFF808080.toInt())
            tag = "status"
        })

        scroll.addView(contentLayout)
        setContentView(scroll)

        lifecycleScope.launch {
            adapter.connect()
            readDtcs()
        }
    }

    private suspend fun readDtcs() {
        val request = ObdNative.buildDtcRequest() ?: return
        val raw = adapter.sendCommand(request)
        val cleaned = ObdNative.cleanResponse(raw) ?: return
        val dtcs = ObdNative.parseDtcResponse(cleaned)

        // Remove "Scanning..." text
        val statusView = contentLayout.findViewWithTag<TextView>("status")
        if (statusView != null) contentLayout.removeView(statusView)

        if (dtcs.isEmpty()) {
            contentLayout.addView(TextView(this).apply {
                text = "No stored DTCs"
                textSize = 18f
                setTextColor(0xFF00E676.toInt())
            })
            return
        }

        contentLayout.addView(TextView(this).apply {
            text = "${dtcs.size} code(s) found"
            textSize = 14f
            setTextColor(0xFFFF9800.toInt()) // Amber for attention
            setPadding(0, 0, 0, 16)
        })

        for (dtc in dtcs) {
            contentLayout.addView(createDtcCard(dtc))
        }
    }

    private fun createDtcCard(dtc: DtcCode): LinearLayout {
        val card = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            setBackgroundColor(0xFF16213E.toInt())
            setPadding(24, 16, 24, 16)
            val params = LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                LinearLayout.LayoutParams.WRAP_CONTENT
            )
            params.bottomMargin = 12
            layoutParams = params
        }

        card.addView(TextView(this).apply {
            text = dtc.formatted
            textSize = 24f
            setTextColor(0xFFFF5252.toInt()) // Red for DTCs
            typeface = Typeface.MONOSPACE
        })

        val categoryName = when (dtc.category) {
            0 -> "Powertrain"
            1 -> "Chassis"
            2 -> "Body"
            3 -> "Network"
            else -> "Unknown"
        }

        card.addView(TextView(this).apply {
            text = categoryName
            textSize = 14f
            setTextColor(0xFF808080.toInt())
        })

        return card
    }
}
