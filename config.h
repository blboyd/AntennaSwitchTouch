#pragma once
#include <Arduino.h>
#include "driver/i2c.h"

// ======================= Wi-Fi =======================
#define WIFI_SSID "ENTER SSID HERE"
#define WIFI_PASS "ENTER PASSWORD HERE"

// ======================= Antennas ====================
#define NUM_ANT   4      // the UI and web page are laid out for 4
#define NAME_LEN  30     // max characters in an antenna name

// Relay board polarity. Most cheap 4-way relay boards are ACTIVE LOW
// (pin pulled low = relay on), which is what the original project assumed.
#define RELAY_ACTIVE_LOW 1

// Break-before-make delay when changing antenna (ms). All relays are turned
// off, then the new one is turned on (same behaviour as the original code,
// but with a defined pause). Set 0 to switch in a single I2C write.
#define SWITCH_BREAK_MS 20

// ======================= I/O expander ================
// IMPORTANT: this board already uses I2C addresses 0x20-0x27, 0x30-0x3F,
// 0x51 and 0x5D (CH422G, GT911 touch, RTC). Do NOT use a PCF8574 / PCF8574A /
// MCP23017 / PCA9554 - they live in that range. Supported chips here:
//
//   EXP_PCA9536  4-bit expander, fixed address 0x41   <- exact fit for 4 relays
//   EXP_AW9523   16-bit expander, 0x58-0x5B (Adafruit AW9523 breakout, 0x58)
#define EXP_PCA9536 1
#define EXP_AW9523  2
#define EXPANDER_TYPE EXP_PCA9536

#if EXPANDER_TYPE == EXP_PCA9536
  #define EXP_ADDR 0x41
#elif EXPANDER_TYPE == EXP_AW9523
  #define EXP_ADDR 0x58      // change if you moved the A0/A1 solder jumpers
#else
  #error "Unknown EXPANDER_TYPE"
#endif

// The I2C port that the board's touch controller + CH422G are on (GPIO8 SDA /
// GPIO9 SCL). Waveshare's demos use port 0.
#define EXP_I2C_PORT I2C_NUM_0

// Which expander pin (0-3) drives each antenna's relay input.
// Change the order here if you wired them differently.
static const uint8_t ANT_BIT[NUM_ANT] = {0, 1, 2, 3};
