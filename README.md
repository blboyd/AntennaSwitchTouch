# ESP32 Antenna Switch: Touchscreen + Web

Modified from [TechMindsYT/ESP32-Antenna-Switch](https://github.com/TechMindsYT/ESP32-Antenna-Switch)
for the **Waveshare ESP32-S3-Touch-LCD-5B** (1024x600, GT911 touch).

Four large touch buttons select the antenna. The original web interface
(buttons, settings page, rename antennas) still works, and both stay in sync.
Selecting on the screen updates open browsers instantly and vice versa.
Names and last-used antenna are saved to flash.

> Not tested on hardware. Compile and bench-test with LEDs on the expander
> outputs before connecting the antenna switch.

## Files

| File | Purpose |
|---|---|
| `AntennaSwitchTouch.ino` | Setup/loop, web server, WebSocket, switching logic |
| `config.h` | Wi-Fi, expander type, relay polarity, pin order |
| `expander.h` | Output driver for the external I2C expander |
| `ui.h` | LVGL touchscreen UI |
| `web_pages.h` | Web pages (from the original project) |
| `display_port.h` | Display/touch bring-up (depends on Waveshare files) |

The sketch folder must be named `AntennaSwitchTouch`.

## 1. Choose the I2C expander (address clash warning)

Waveshare lists these I2C addresses as already used on this board:
`0x20-0x27`, `0x30-0x3F`, `0x51`, `0x5D` (CH422G, GT911 touch, RTC).
That excludes the usual **PCF8574, PCF8574A, MCP23017, PCA9554**.

| Chip | Address | Notes |
|---|---|---|
| **PCA9536** (recommended) | fixed `0x41` | 4 outputs, exact fit. Set `EXPANDER_TYPE EXP_PCA9536` |
| AW9523 (e.g. Adafruit breakout) | `0x58` | 16 outputs. Set `EXPANDER_TYPE EXP_AW9523` |

The AW9523 register sequence is the less certain of the two. Prefer the PCA9536.

Also: the board has only two isolated outputs (DO0/DO1, open-drain, 5-36 V),
so the external expander is the right call for four relays.

## 2. Wiring

Use the board's **I2C terminal** (SDA = GPIO8, SCL = GPIO9, shared with touch).

- Expander SDA/SCL/GND/VCC to the I2C terminal. Keep the VOUT jumper at **3.3 V** (default).
- Expander pins 0-3 to relay board IN1-IN4 (Ant 1-4). Reorder in `ANT_BIT[]` if needed.
- Power the relay board from a supply suited to it, not from the I2C VOUT pin.
- Relay board polarity: `RELAY_ACTIVE_LOW` (default `1`, as in the original project).

## 3. Arduino IDE setup

- Board package **esp32 by Espressif >= 3.0.0**, board **ESP32S3 Dev Module**
- Flash 16 MB, PSRAM **OPI 8 MB**, USB CDC On Boot enabled, a larger partition
  scheme (match Waveshare's screenshots on their wiki)
- Libraries: `ESP32_Display_Panel` (>= 0.1.4), `ESP32_IO_Expander` (>= 0.0.4),
  `lvgl` **8.4.0**
- Web server libraries: use the maintained **ESPAsyncWebServer and AsyncTCP forks**
  (Library Manager: "ESP Async WebServer" and "Async TCP", from ESP32Async).
  The old me-no-dev versions used by the original project often do not compile on
  core 3.x.

## 4. Pull in Waveshare's board support files

Download Waveshare's demo (link on the wiki: ESP32-S3-Touch-LCD-5) and copy these
from `Arduino/09_lvgl_Porting` into this sketch folder:

`lv_conf.h`, `lvgl_v8_port.h`, `lvgl_v8_port.cpp`, `ESP_Panel_Board_Custom.h`,
`ESP_Panel_Board_Supported.h`, `ESP_Panel_Conf.h`

Then in `ESP_Panel_Board_Custom.h`:

```c
#define ESP_PANEL_USE_1024_600_LCD (1)
#define ESP_OPEN_TOUCH             (1)
```

In `lv_conf.h`, enable `LV_FONT_MONTSERRAT_36` and `LV_FONT_MONTSERRAT_20`
(optional; the UI falls back to the default font).

If the demo's `setup()` differs from `display_init()` in `display_port.h`,
paste the demo's panel/LVGL init lines into that function.

## 5. Configure and flash

Edit `WIFI_SSID` / `WIFI_PASS` in `config.h`, flash, and open the IP address
shown at the top-right of the screen in a browser.

## Notes

- Wi-Fi no longer blocks boot. The touchscreen works with no network.
- Switching is break-before-make with a 20 ms pause (`SWITCH_BREAK_MS`).
- If the expander does not answer, a red "I/O expander not found" banner shows on
  the screen and web page, and the selection is retried on the next tap.
- Heavy Wi-Fi traffic can cause RGB-panel shimmer on ESP32-S3 boards. Waveshare's
  config uses a bounce buffer for this. See their wiki FAQ if you see drift.
- Antenna names are shown with LVGL's default font, so non-ASCII characters
  will not render on the touchscreen (they work on the web page).
