#pragma once
// Touchscreen UI: a header (title, Wi-Fi/IP, I/O error banner) and a 2x2 grid
// of large antenna buttons. Layout is computed from the actual screen size so
// it works on both the 800x480 and 1024x600 versions of the board.
//
// All functions here must be called with the LVGL lock held
// (lvgl_port_lock / lvgl_port_unlock).
#include <lvgl.h>
#include "config.h"

// Defined in the .ino. Safe to call from the LVGL task (it only sets a flag).
void requestSelect(int id);

// Fonts: need LV_FONT_MONTSERRAT_36 / _20 enabled in lv_conf.h for the best
// look; falls back to the default font if they are not.
#if LV_FONT_MONTSERRAT_36
  #define FONT_BIG (&lv_font_montserrat_36)
#else
  #define FONT_BIG LV_FONT_DEFAULT
#endif
#if LV_FONT_MONTSERRAT_20
  #define FONT_SMALL (&lv_font_montserrat_20)
#else
  #define FONT_SMALL LV_FONT_DEFAULT
#endif

#define COL_BG      0x121212
#define COL_OFF     0x263238   // inactive button
#define COL_PRESSED 0x455a64
#define COL_ON      0x2e7d32   // active antenna
#define COL_ACCENT  0x00bcd4
#define COL_ERR     0xf44336

static lv_obj_t *s_btn[NUM_ANT];
static lv_obj_t *s_name[NUM_ANT];
static lv_obj_t *s_state[NUM_ANT];
static lv_obj_t *s_wifi;
static lv_obj_t *s_warn;

static void onBtnClicked(lv_event_t *e) {
  requestSelect((int)(intptr_t)lv_event_get_user_data(e));
}

static void ui_create() {
  lv_obj_t *scr = lv_scr_act();
  lv_obj_set_style_bg_color(scr, lv_color_hex(COL_BG), 0);
  lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

  const int W = lv_disp_get_hor_res(NULL);
  const int H = lv_disp_get_ver_res(NULL);
  const int HDR = 70, GAP = 20;
  const int bw = (W - 3 * GAP) / 2;
  const int bh = (H - HDR - 3 * GAP) / 2;

  lv_obj_t *title = lv_label_create(scr);
  lv_label_set_text(title, "Antenna Switch");
  lv_obj_set_style_text_font(title, FONT_BIG, 0);
  lv_obj_set_style_text_color(title, lv_color_hex(COL_ACCENT), 0);
  lv_obj_align(title, LV_ALIGN_TOP_LEFT, GAP, 14);

  s_wifi = lv_label_create(scr);
  lv_label_set_text(s_wifi, "");
  lv_obj_set_style_text_font(s_wifi, FONT_SMALL, 0);
  lv_obj_set_style_text_color(s_wifi, lv_color_hex(0xe0e0e0), 0);
  lv_obj_align(s_wifi, LV_ALIGN_TOP_RIGHT, -GAP, 22);

  s_warn = lv_label_create(scr);
  lv_label_set_text(s_warn, "I/O expander not found");
  lv_obj_set_style_text_font(s_warn, FONT_SMALL, 0);
  lv_obj_set_style_text_color(s_warn, lv_color_hex(COL_ERR), 0);
  lv_obj_align(s_warn, LV_ALIGN_TOP_MID, 0, 22);
  lv_obj_add_flag(s_warn, LV_OBJ_FLAG_HIDDEN);

  for (int i = 0; i < NUM_ANT; i++) {
    int x = GAP + (i % 2) * (bw + GAP);
    int y = HDR + GAP + (i / 2) * (bh + GAP);

    lv_obj_t *b = lv_btn_create(scr);
    s_btn[i] = b;
    lv_obj_set_size(b, bw, bh);
    lv_obj_set_pos(b, x, y);
    lv_obj_set_style_radius(b, 16, 0);
    lv_obj_set_style_shadow_width(b, 0, 0);
    lv_obj_set_style_bg_color(b, lv_color_hex(COL_OFF), 0);
    lv_obj_set_style_bg_color(b, lv_color_hex(COL_PRESSED), LV_STATE_PRESSED);
    lv_obj_add_event_cb(b, onBtnClicked, LV_EVENT_CLICKED, (void *)(intptr_t)i);

    lv_obj_t *num = lv_label_create(b);
    lv_label_set_text_fmt(num, "ANTENNA %d", i + 1);
    lv_obj_set_style_text_font(num, FONT_SMALL, 0);
    lv_obj_set_style_text_color(num, lv_color_hex(0xb0bec5), 0);
    lv_obj_align(num, LV_ALIGN_TOP_MID, 0, 16);

    s_name[i] = lv_label_create(b);
    lv_label_set_long_mode(s_name[i], LV_LABEL_LONG_WRAP);
    lv_obj_set_width(s_name[i], bw - 40);
    lv_obj_set_style_text_align(s_name[i], LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(s_name[i], FONT_BIG, 0);
    lv_obj_set_style_text_color(s_name[i], lv_color_white(), 0);
    lv_obj_align(s_name[i], LV_ALIGN_CENTER, 0, 0);

    s_state[i] = lv_label_create(b);
    lv_obj_set_style_text_font(s_state[i], FONT_SMALL, 0);
    lv_obj_set_style_text_color(s_state[i], lv_color_white(), 0);
    lv_obj_align(s_state[i], LV_ALIGN_BOTTOM_MID, 0, -16);
  }
}

// active: 0..NUM_ANT-1, or -1 for none.
static void ui_refresh(int active, const char names[][NAME_LEN + 1],
                       bool ioOk, const char *wifiText) {
  for (int i = 0; i < NUM_ANT; i++) {
    bool on = (i == active);
    lv_label_set_text(s_name[i], names[i]);
    lv_obj_set_style_bg_color(s_btn[i], lv_color_hex(on ? COL_ON : COL_OFF), 0);
    lv_label_set_text(s_state[i], on ? "ACTIVE" : "Tap to select");
  }
  lv_label_set_text(s_wifi, wifiText);
  if (ioOk) lv_obj_add_flag(s_warn, LV_OBJ_FLAG_HIDDEN);
  else      lv_obj_clear_flag(s_warn, LV_OBJ_FLAG_HIDDEN);
}
