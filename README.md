# Camera LoRa Controller

Wireless Bluetooth Low Energy (BLE) camera controller built for the **Heltec WiFi LoRa 32 V3** ESP32 board. Control your BLE-enabled camera with a simple button interface and real-time OLED display feedback.

## Features

- 🎥 **BLE Camera Control** - Connect to and control BLE-enabled cameras
- 📺 **OLED Status Display** - Real-time visual feedback on 128x64 OLED screen
- 🔘 **Simple Button Interface** - Single button for all operations
- ⏰ **Long Press Shutdown** - Hold button for 3 seconds to power down
- 🔋 **Power Efficient** - Deep sleep mode for battery conservation

## Hardware Requirements

- **Heltec WiFi LoRa 32 V3** board
- USB-C cable for programming and power
- BLE-enabled camera (compatible with START/STOP commands)

## Pin Configuration

| Peripheral | Pin |
|------------|-----|
| OLED SDA   | GPIO 17 |
| OLED SCL   | GPIO 18 |
| OLED RST   | GPIO 21 |
| Button     | GPIO 0 (PRG button) |

## BLE Commands

The controller sends these BLE commands to the camera:

- `START` - Begin recording
- `STOP` - Stop recording

## Usage

### Button Controls

**Short Press:**
- **IDLE/Disconnected** → Start BLE scan for camera
- **Scanning** → Cancel scan
- **Connected** → Start recording
- **Recording** → Stop recording

**Long Press (3 seconds):**
- **Any State** → Shutdown device (enters deep sleep)

### Display States

The OLED display shows the current operation status:

- `Ready - Press button to scan` - Idle, ready to connect
- `Scanning... - Looking for camera` - Searching for BLE devices
- `Connecting... - Please wait` - Establishing BLE connection
- `Connected! - Press to record` - Ready to control camera
- `Recording... - Press to stop` - Camera is recording
- `Stopping... - Please wait` - Stop command sent
- `Saving... - Processing file` - Camera saving file
- `Disconnected - Press to reconnect` - Lost BLE connection
- `Error: [message]` - Error condition
- `Shutting down... - Goodbye!` - Entering deep sleep

## Setup & Installation

### 1. Configure Camera Settings

Edit `src/main.cpp` and update these lines with your camera's BLE details:

```cpp
#define CAMERA_SERVICE_UUID "YOUR_CAMERA_SERVICE_UUID"  // Replace with actual UUID
#define CAMERA_CHAR_UUID "YOUR_CAMERA_CHAR_UUID"        // Replace with actual UUID
#define CAMERA_DEVICE_NAME "Camera"                      // Replace with actual device name
```

### 2. Build & Upload

```bash
# Install PlatformIO if not already installed
# https://platformio.org/install/cli

# Build the project
pio run

# Upload to board (hold BOOT button during upload if needed)
pio run -t upload

# Monitor serial output
pio device monitor
```

### 3. Troubleshooting Upload

If upload fails, manually enter bootloader mode:

1. Hold **BOOT** button
2. Press and release **RST** button while holding BOOT
3. Release **BOOT** button
4. Run `pio run -t upload`

## Development

### Project Structure

```
.
├── src/
│   └── main.cpp          # Main application code
├── include/              # Project headers (if needed)
├── lib/                  # Private libraries
├── platformio.ini        # Build configuration
└── .github/
    └── copilot-instructions.md  # AI coding assistant guide
```

### Building

```bash
# Clean build
pio run -t clean

# Build only
pio run

# Upload and monitor
pio run -t upload && pio device monitor
```

### Dependencies

Automatically managed by PlatformIO:

- `ESP8266 and ESP32 OLED driver for SSD1306 displays` - OLED display support
- Built-in ESP32 Arduino BLE libraries

## Technical Details

### State Machine

The controller uses a state machine with these states:

```
IDLE → SCANNING → CONNECTING → CONNECTED → RECORDING → STOPPING → SAVING → CONNECTED
                                     ↓
                               DISCONNECTED
                                     ↓
                                ERROR_STATE
```

### BLE Connection Flow

1. **Scan** - Search for BLE devices (10 second timeout)
2. **Match** - Find device by name containing `CAMERA_DEVICE_NAME`
3. **Connect** - Establish BLE client connection
4. **Discover** - Get service and characteristic
5. **Command** - Write commands to characteristic

### Power Management

- **Active Mode** - Normal operation with BLE and OLED active
- **Deep Sleep** - Triggered by long press, requires RST button to wake

## Serial Monitor

Connect at 115200 baud to see debug output:

```
Camera LoRa Controller Starting...
BLE Initialized
Found BLE Device: [device info]
Camera device found!
Connected to camera
Sending command: START
```

## License

MIT License - See LICENSE file for details

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Test on hardware
5. Submit a pull request

## Support

For issues or questions:
- Open an issue on GitHub
- Check PlatformIO documentation for build problems
- Verify BLE UUIDs match your camera's specifications

## Credits

Built with:
- [PlatformIO](https://platformio.org/)
- [ESP32 Arduino Core](https://github.com/espressif/arduino-esp32)
- [ESP8266 and ESP32 OLED Driver](https://github.com/ThingPulse/esp8266-oled-ssd1306)
