# Copilot Instructions: Camera LoRa Controller

## Project Overview
Wireless camera controller using **Heltec WiFi LoRa 32** board (ESP32-based). Controls camera via **BLE (Bluetooth Low Energy)** commands with real-time status display on onboard OLED. Built with PlatformIO and Arduino framework.

## Hardware Platform
- **Target Board**: Heltec WiFi LoRa 32 (ESP32 + LoRa transceiver + 128x64 OLED display)
- **Framework**: Arduino (ESP32 variant)
- **Toolchain**: PlatformIO (managed via `platformio.ini`)
- **Key Peripherals**:
  - BLE radio (built-in ESP32 Bluetooth)
  - SSD1306 OLED display (I2C) for status feedback

## Development Workflow

### Building & Uploading
```bash
# Build the project
pio run

# Upload to connected board
pio run --target upload

# Build and upload in one command
pio run -t upload

# Monitor serial output (115200 baud default for ESP32)
pio device monitor

# Upload and monitor
pio run -t upload && pio device monitor
```

### Project Structure Conventions
- **`src/main.cpp`**: Primary application entry point with `setup()` and `loop()` functions
- **`include/`**: Project-specific header files (`.h`) - use `#include "header.h"` with quotes
- **`lib/`**: Private libraries organized as `lib/LibraryName/src/` with dedicated headers
- **`.pio/`**: Build artifacts (git-ignored) - regenerated automatically

### Code Style
- Arduino-style setup/loop pattern is standard for this framework
- Place function declarations at top of `main.cpp` or in `include/` headers
- Use `#include <Arduino.h>` (angle brackets) for framework libraries
- Use `#include "custom.h"` (quotes) for project headers

## Hardware-Specific Considerations

### ESP32 Resources
- **GPIO**: Check Heltec pinout - some pins used by LoRa (SCK, MISO, MOSI, SS, RST, DIO0)
- **OLED Display**: I2C on SDA/SCL (pins vary by board revision, typically GPIO 4/15)
- **BLE**: Built-in ESP32 Bluetooth radio - use `BLEDevice`, `BLEClient` classes
- **LoRa Radio**: SPI-based (available if needed for future expansion)
- **WiFi**: Built-in ESP32 WiFi (can coexist with BLE but manage power carefully)

### Common Pitfalls
- ESP32 Arduino uses **FreeRTOS** underneath - avoid blocking operations in `loop()`
- **BLE and WiFi share radio**: Can't scan both simultaneously on ESP32
- BLE callbacks execute in separate task - use semaphores/queues for state management
- OLED updates can block - keep display refreshes quick or use async patterns
- Serial monitor must match baud rate set in `Serial.begin()` (typically 115200)

## Adding Dependencies
Add libraries to `platformio.ini` under `[env:heltec_wifi_lora_32]`:
```ini
lib_deps = 
    heltecautomation/Heltec ESP32 Dev-Boards  ; OLED + board support
    thingpulse/ESP8266 and ESP32 OLED driver for SSD1306 displays
```

**BLE Support**: Built into ESP32 Arduino core - no additional library needed. Use:
```cpp
#include <BLEDevice.h>
#include <BLEClient.h>
#include <BLEScan.h>
```

PlatformIO auto-installs on next build. Prefer PlatformIO Library Registry over manual library placement.

## Key Files to Reference
- **`platformio.ini`**: Build configuration, board settings, dependencies
- **`src/main.cpp`**: Application logic starting point
- **`.gitignore`**: Excludes build artifacts (`.pio/`, `.vscode/` internals)

## Application Architecture

### BLE Camera Control Flow
1. **Discovery**: Scan for camera BLE device (match service UUID or device name)
2. **Connection**: Establish BLE client connection to camera peripheral
3. **Command Transmission**: Write to BLE characteristic to send commands:
   - Start recording
   - Stop recording
   - Save/capture
   - Other camera-specific commands
4. **Status Feedback**: Read BLE notifications/indications for camera state changes

### OLED Display States
Display should show current operation status:
- **"Scanning..."**: BLE device discovery in progress
- **"Connecting..."**: Establishing BLE connection
- **"Connected"**: Ready to send commands
- **"Recording"**: Camera actively recording
- **"Stopping..."**: Stop command sent, awaiting confirmation
- **"Saving..."**: Camera saving/processing file
- **"Disconnected"**: BLE connection lost or intentionally closed
- **"Error: [msg]"**: Display specific error conditions

### State Management Pattern
Use state machine approach in `loop()`:
```cpp
enum CameraState { IDLE, SCANNING, CONNECTING, CONNECTED, RECORDING, STOPPING, SAVING };
CameraState currentState = IDLE;
```
Update OLED on state transitions, handle BLE callbacks to update state asynchronously.
