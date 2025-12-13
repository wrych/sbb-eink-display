# SBB E-Ink Display

A WiFi-connected train departure board using an ESP32-S3 and a 4.2" E-Ink display, showing real-time text departures from the Swiss Federal Railways (SBB).

## Hardware Setup

**Components:**
*   **Controller:** ESP32-S3 SuperMini
*   **Display:** WeAct Studio 4.2" E-Ink Display (Black/White/Red)

**Wiring (Right-Side Cluster):**

| ESP32-S3 Pin | E-Ink Pin | Description |
| :--- | :--- | :--- |
| **GPIO 5** | CS | Chip Select |
| **GPIO 17** | DC | Data/Command |
| **GPIO 16** | RST | Reset |
| **GPIO 4** | BUSY | Busy Signal |
| **GPIO 18** | CLK | SPI Clock |
| **GPIO 7** | DIN | SPI MOSI |
| **GPIO 10** | Button | Config Button (Active High) |
| **3.3V** | VCC | Power |
| **GND** | GND | Ground |

## Installation & Programming

1.  **Software:** Install [Visual Studio Code](https://code.visualstudio.com/) and the [PlatformIO](https://platformio.org/) extension.
2.  **Open Project:** Open this folder in VS Code.
3.  **Drivers:** Ensure you have the drivers for the ESP32-S3 (USB-CDC).
4.  **Flash:**
    *   Connect the ESP32-S3 via USB-C.
    *   Click the **PlatformIO Icon** (Alien face) in the left sidebar.
    *   Under `esp32-s3-supermini`, click **Upload**.
    *   *Note: If it fails to connect, hold the BOOT button on the ESP32 while plugging it in to enter bootloader mode.*

## Configuration (Changing Station & WiFi)

The device is configured wirelessly using Bluetooth Low Energy (BLE). You can use any BLE tool, but **nRF Connect for Mobile** (Android/iOS) is recommended.

### 1. Enter Configuration Mode
You can enter configuration mode in two ways:
*   **Boot:** Hold the button (GPIO 10) while powering on/resetting.
*   **Runtime:** Hold the button for **> 3 seconds** while the device is running.
*   *Fallback:* If WiFi connection fails (timeout 30s), it will automatically enter config mode.

*The screen will display the Configuration UI.*

### 2. Connect via BLE
1.  Open the **nRF Connect** app on your phone.
2.  Scan for devices.
3.  Connect to **"SBB_Display_Config"**.

### 3. Change Settings
Navigate to the Unknown Service (UUID: `...7670`) and write to the following characteristics:

| Setting | UUID Ends With | Type | Description |
| :--- | :--- | :--- | :--- |
| **SSID** | `...7671` | Read/Write | WiFi Name |
| **Password** | `...7672` | Write Only | WiFi Password |
| **Station** | `...7673` | Read/Write | SBB Station Name (e.g. "Zürich HB") |
| **Refresh** | `...7674` | Read/Write | Update interval in Minutes |
| **Action** | `...7675` | Write | Command trigger |

**To Save & Reboot:**
1.  Write your new values to the respective characteristics.
2.  Select the **Action** characteristic (`...7675`).
3.  Write the string **"SAVE"** (needs to be text/UTF8).
4.  The device will save settings to internal storage and restart.

## Usage
Once configured, the device will:
1.  Connect to WiFi.
2.  Fetch data from `transport.opendata.ch`.
3.  Update the E-Ink display (Full Refresh).
4.  Sleep for the configured Refresh interval (default 5 min).
5.  Repeat.

**Manual Refresh:** Short press the button (50ms - 5s) to trigger an immediate update.

## Troubleshooting
*   **Screen not updating:** Check the Serial Monitor (115200 baud). The "BUSY" pin might be stuck if wiring is loose.
*   **Red LED:** The onboard LED usually indicates power/status depending on the board variant. Use Serial for debug logs.
