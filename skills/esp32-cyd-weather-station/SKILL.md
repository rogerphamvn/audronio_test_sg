---
name: esp32-cyd-weather-station
description: >
  Complete development, setup, configuration, flashing, and troubleshooting guide for ESP32-2432S028
  (CYD - Cheap Yellow Display, 2.8-inch SPI TFT LCD 240x320 ILI9341_2) using Arduino IDE or arduino-cli.
  Covers TFT_eSPI pinouts, the normally-black LCD inverted color fix, the CH340 manual bootloader timing combo,
  NTP time synchronization (GMT+7), IP-based geolocation, and real-time Open-Meteo weather and rainfall metrics.
---

# ESP32-2432S028 (CYD 2.8") Weather Station & Display Skill

This skill captures the end-to-end workflow, architectural decisions, hardware quirks, and troubleshooting methods for developing and flashing firmware onto the **ESP32-2432S028** (popularly known as the **CYD - Cheap Yellow Display**).

---

## 1. Hardware Specifications & Pinout

* **MCU:** ESP32-D0WD-V3 (Xtensa Dual-Core 32-bit LX6 @ 240 MHz, 4MB SPI Flash).
* **Display:** 2.8-inch SPI TFT LCD (240x320 pixels), Controller: `ILI9341_2` (Normally-Black panel).
* **Touch Controller:** XPT2046 (SPI).
* **USB-to-UART:** CH340C (Standard default port `COM3` on Windows).
* **Wireless:** Wi-Fi 802.11 b/g/n (2.4 GHz ONLY). **Notice:** Does NOT support 5 GHz Wi-Fi bands.

### Critical Pinout Mapping (TFT SPI)
| Function | ESP32 GPIO | Notes |
| :--- | :--- | :--- |
| `TFT_MOSI` | `GPIO 13` | SPI Data Out |
| `TFT_MISO` | `GPIO 12` | SPI Data In |
| `TFT_SCLK` | `GPIO 14` | SPI Clock |
| `TFT_CS`   | `GPIO 15` | Chip Select (Active LOW) |
| `TFT_DC`   | `GPIO 2`  | Data / Command Select |
| `TFT_RST`  | `-1`      | Tied to Board EN / Reset pin |
| `TFT_BL`   | `GPIO 21` | Backlight LED (HIGH = On, LOW = Off) |

---

## 2. TFT_eSPI Configuration (`User_Setup.h`)

For the display to work with `TFT_eSPI` (v2.5.x+), the driver configuration file (`Arduino/libraries/TFT_eSPI/User_Setup.h`) must define:

```c
#define ILI9341_2_DRIVER
#define TFT_INVERSION_ON

#define TFT_MISO 12
#define TFT_MOSI 13
#define TFT_SCLK 14
#define TFT_CS   15
#define TFT_DC    2
#define TFT_RST  -1
#define TFT_BL   21
#define TFT_BACKLIGHT_ON HIGH

#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_FONT6
#define LOAD_FONT7
#define LOAD_FONT8
#define SMOOTH_FONT

#define SPI_FREQUENCY  40000000
#define SPI_READ_FREQUENCY  20000000
```

---

## 3. Critical Hardware Quirks & Solutions

### Quirk A: Inverted / Negative Colors
* **Symptom:** Sky displays cyan/green instead of blue, red appears as light blue/cyan, colors look like photographic film negatives.
* **Root Cause:** The 2.8" panel on the CYD is a **normally-black** IPS/TN panel requiring the display inversion register `INVON` (0x21).
* **Fix:** Always invoke `tft.invertDisplay(true);` right after `tft.init();` in code, and ensure `#define TFT_INVERSION_ON` is enabled in `User_Setup.h`.

### Quirk B: Bootloader Mode on CH340 (`Wrong boot mode detected 0x13`)
* **Symptom:** Running `arduino-cli upload` or `esptool.py` fails with `Wrong boot mode detected (0x13)`.
* **Root Cause:** The CYD board does not have an automatic transistor DTR/RTS pulse circuit pulling GPIO0 LOW during reset when firmware is already running.
* **Fix:** Use the 3-step manual button combo:
  1. Press and **HOLD** the `BOOT` button (located on the bottom edge).
  2. Press and **RELEASE** the `RST` button (located on the top edge).
  3. **RELEASE** the `BOOT` button.
* **Flasher Rule:** When the board is in download mode (`boot:0x3`), always use `--before no-reset` in `esptool` so the upload tool does not issue a reset pulse that kicks the chip back into user code.

### Quirk C: Wi-Fi Band Restriction
* **Symptom:** Firmware gets stuck on `WiFi.begin()` with 30 dots and reports connection failure.
* **Root Cause:** Attempting to connect to a 5 GHz-only SSID (e.g. `EverGrowth 5G`).
* **Fix:** Connect to the 2.4 GHz SSID (e.g. `EverGrowth`). Always execute `WiFi.disconnect(true, true);` before `WiFi.begin()` to clear stale NVS Wi-Fi configurations.

---

## 4. Arduino IDE 2.x Board Configuration (`sketch.yaml`)

To avoid the `Missing FQBN` error in Arduino IDE, save a `sketch.yaml` file in each sketch directory:

```yaml
default_fqbn: esp32:esp32:esp32:PartitionScheme=huge_app,UploadSpeed=921600
default_port: COM3
```

* **Board:** `ESP32 Dev Module` (`esp32:esp32:esp32`).
* **Partition Scheme:** `Huge APP (3MB No OTA / 1MB SPIFFS)` (`PartitionScheme=huge_app`) — essential for sketches importing WiFi, HTTPClient, and ArduinoJson.
* **Upload Speed:** `921600`.

---

## 5. Automated Step-by-Step Implementation

### Step 1: Hardware & Display Verification (`01_Test_Man_Hinh`)
Verify:
1. Backlight GPIO 21 turns ON.
2. 3 primary color fills (RED, GREEN, BLUE) are distinct and non-inverted.
3. Seconds counter updates on the display.

### Step 2: Complete Real-Time Weather Station (`02_Tram_Thoi_Tiet_WiFi`)
Architecture:
1. **Network Sync:**
   - Clears stale WiFi credentials and connects to 2.4 GHz network.
   - Synchronizes RTC via NTP pool (`pool.ntp.org`, `time.google.com`) with `configTime(7 * 3600, 0, ...)`.
2. **Dynamic Geolocation (Plain HTTP):**
   - Queries `http://ip-api.com/json/` to fetch actual city name, country code, latitude, and longitude.
   - Using plain HTTP avoids TLS/SSL handshake RAM overhead (`WiFiClientSecure` stack exhaustion).
3. **Weather & Rainfall Metrics:**
   - Queries Open-Meteo API: `http://api.open-meteo.com/v1/forecast?latitude=...&longitude=...&current=temperature_2m,relative_humidity_2m,rain,weather_code&timezone=Asia%2FBangkok`.
   - Parses `temperature_2m` (°C), `relative_humidity_2m` (%), `rain` (mm/h), and WMO `weather_code`.
4. **Flicker-Free UI Rendering:**
   - Static borders and cards are drawn once (`drawStaticUI`).
   - Clock digits overwrite previous values using text background colors (`tft.setTextColor(TFT_YELLOW, COLOR_CARD_BG)`).
   - Weather metrics refresh in the background every 15 minutes.

### Step 3: Smart Photo Frame from Mobile Phone (`03_Khung_Anh_Dien_Thoai`)
Architecture:
1. **Local Web Server & Dynamic QR Code Display:**
   - Hosts a lightweight HTTP Web Server on port 80.
   - Dynamically draws an on-screen QR Code using `qrcode.h` with the device's local IP (`http://<ip>`).
2. **Client-Side Canvas Processing (Mobile Browser):**
   - User scans the QR code with iOS/Android camera, opening the portal in Safari/Chrome.
   - HTML5 Canvas pre-processes, rotates, zooms, and crops the image to exact 240x320 pixels on the phone.
   - Compresses to ~20 KB JPEG, preventing ESP32 memory exhaustion from high-resolution phone cameras.
3. **Fast JPEG Decoding & Persistent Flash Storage:**
   - ESP32 receives image via HTTP multipart POST and saves to `/photo.jpg` in LittleFS.
   - Decodes and renders directly to the TFT LCD via `TJpg_Decoder` in under 0.5s.
   - Photos persist across power cycles. Includes `/reset` endpoint to return to the QR pairing screen.

---

## 6. Automated Python Flashing Tool (`tools/flash_tool.py`)

A non-blocking auto-retry listener that catches the ESP32 download bootloader immediately upon button combo:

```python
import subprocess, time, sys

cmd = [
    "uv", "tool", "run", "esptool",
    "--port", "COM3",
    "--baud", "921600",
    "--before", "no-reset",
    "--after", "hard-reset",
    "write-flash", "-z",
    "--flash_mode", "dio",
    "--flash_freq", "80m",
    "--flash_size", "4MB",
    "0x1000", "bootloader.bin",
    "0x8000", "partitions.bin",
    "0xe000", "boot_app0.bin",
    "0x10000", "app.bin"
]

start_time = time.time()
while time.time() - start_time < 90:
    res = subprocess.run(cmd, capture_output=True, text=True)
    if "Hash of data verified" in res.stdout or res.returncode == 0:
        print("\n>>> FLASH SUCCESSFUL! <<<")
        sys.exit(0)
    time.sleep(0.3)
```
