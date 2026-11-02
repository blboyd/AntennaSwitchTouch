#pragma once
// Minimal output-only driver for the external I2C expander.
//
// It talks through the ESP-IDF I2C driver on the SAME port the touch
// controller and CH422G already use, so transactions are serialised by the
// driver's own bus lock. Do not call Wire.begin() on GPIO8/9 in this sketch.
#include "config.h"

static bool expWriteReg(uint8_t reg, uint8_t val) {
  uint8_t buf[2] = {reg, val};
  esp_err_t r = i2c_master_write_to_device(EXP_I2C_PORT, EXP_ADDR, buf, 2,
                                           pdMS_TO_TICKS(50));
  if (r != ESP_OK) {
    Serial.printf("[exp] I2C write to 0x%02X failed: %s\n", EXP_ADDR,
                  esp_err_to_name(r));
    return false;
  }
  return true;
}

// Pin levels (bit0..bit3) that put the relay for antenna `id` on and every
// other relay off. id = -1 -> everything off.
static uint8_t levelsFor(int id) {
  uint8_t on = (id >= 0 && id < NUM_ANT) ? (1u << ANT_BIT[id]) : 0;
#if RELAY_ACTIVE_LOW
  return (~on) & 0x0F;
#else
  return on & 0x0F;
#endif
}

// Write levels for pins 0-3.
static bool expWriteOutputs(uint8_t levels) {
#if EXPANDER_TYPE == EXP_PCA9536
  return expWriteReg(0x01, 0xF0 | (levels & 0x0F));   // output port register
#elif EXPANDER_TYPE == EXP_AW9523
  return expWriteReg(0x02, 0xF0 | (levels & 0x0F));   // output port 0 register
#endif
}

// Configure pins 0-3 as outputs. The output register is written BEFORE the
// direction register so the relays never see a spurious "on" level.
static bool expBegin() {
#if EXPANDER_TYPE == EXP_PCA9536
  if (!expWriteOutputs(levelsFor(-1))) return false;  // idle = all relays off
  return expWriteReg(0x03, 0xF0);                     // config: 0 = output
#elif EXPANDER_TYPE == EXP_AW9523
  expWriteReg(0x7F, 0x00);                            // soft reset
  delay(2);
  if (!expWriteOutputs(levelsFor(-1))) return false;
  bool ok = expWriteReg(0x04, 0xF0);                  // P0 config: 0 = output
  ok &= expWriteReg(0x11, 0x10);                      // P0 push-pull
  return ok;
#endif
}
