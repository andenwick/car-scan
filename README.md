# CarScan

OBD-II diagnostic tool. A C library that parses raw OBD-II protocol data, paired with an Android app (Kotlin) that handles Bluetooth communication via ELM327 adapters.

Every car made after 1996 has an OBD-II port. Plug in a $10 Bluetooth adapter, and this reads engine data, diagnostic trouble codes, and VIN information from the car's computer.

## How It Works

The car's ECU speaks hex — responses like `41 0C 1A F8`. The C library translates those into structured data (engine RPM, coolant temp, vehicle speed, etc). The Android app connects to the ELM327 adapter over Bluetooth and calls the C library through JNI.

## C Library (`obd/`)

Pure C, zero heap allocation. Caller provides all buffers, so memory leaks aren't possible.

**Modules:**
- `elm327` — ELM327 adapter protocol (AT commands, initialization, reset)
- `pid` — Parameter ID parsing (RPM, speed, coolant temp, throttle, etc.)
- `dtc` — Diagnostic Trouble Code decoding (P/C/B/U codes with descriptions)
- `vin` — Vehicle Identification Number extraction
- `sensor` — Real-time sensor data with unit conversion
- `hex_utils` — Hex string parsing and validation

**Build:**
```bash
# Windows
.\obd\build.ps1

# CMake
cd obd && mkdir build && cd build
cmake .. && cmake --build .
```

**Tests:**
```bash
cd obd/build && ctest
```

## Android App (`android/`)

Kotlin app with:
- Bluetooth device scanning and ELM327 connection
- Live sensor data display (RPM, speed, temperature)
- DTC reading and clearing
- Mock adapter for development without hardware

## Project Structure

```
obd/
├── include/obd/       # Public API headers (obd.h, obd_types.h)
├── src/               # Implementation (elm327, pid, dtc, vin, sensor, hex_utils)
├── tests/             # Unit tests per module
├── docs/              # Design docs explaining each module
└── CMakeLists.txt     # Build system

android/
├── app/src/main/
│   ├── cpp/           # JNI bridge (C library ↔ Kotlin)
│   ├── java/          # Kotlin app (Bluetooth, UI, OBD types)
│   └── res/           # Android resources
└── build.gradle.kts
```

## License

MIT
