#pragma once
// ---------------------------------------------------------------------------
// Display + touch bring-up. This is the ONLY part that depends on Waveshare's
// board support files, so it is isolated here.
//
// It follows the pattern of Waveshare's "09_lvgl_Porting" Arduino demo
// (ESP32_Display_Panel + lvgl_v8_port). If your copy of that demo's setup()
// differs from this (library versions change), paste ITS panel/LVGL init lines
// into display_init() and leave everything else alone. The rest of this
// project only needs:   lvgl_port_lock(ms) / lvgl_port_unlock()  and LVGL.
//
// Files you must copy from Waveshare's 09_lvgl_Porting sketch folder into this
// sketch folder:
//   lv_conf.h, lvgl_v8_port.h, lvgl_v8_port.cpp,
//   ESP_Panel_Board_Custom.h, ESP_Panel_Board_Supported.h, ESP_Panel_Conf.h
// and in ESP_Panel_Board_Custom.h set:
//   #define ESP_PANEL_USE_1024_600_LCD (1)   // you have the "5B" (1024x600)
//   #define ESP_OPEN_TOUCH             (1)
// ---------------------------------------------------------------------------
#include <Arduino.h>
#include <ESP_Panel_Library.h>
#include <lvgl.h>
#include "lvgl_v8_port.h"

static bool display_init() {
  ESP_Panel *panel = new ESP_Panel();
  panel->init();      // also brings up the I2C bus, CH422G and GT911 touch
  panel->begin();     // starts the RGB panel and backlight
  return lvgl_port_init(panel->getLcd(), panel->getTouch());
}
