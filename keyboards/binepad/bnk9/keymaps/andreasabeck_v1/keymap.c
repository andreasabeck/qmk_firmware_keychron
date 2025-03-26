// Copyright 2023 Binepad (@binpad)
// SPDX-License-Identifier: GPL-2.0-or-later

#include QMK_KEYBOARD_H


enum custom_keycodes {
  JIGGLE = SAFE_RANGE,
  AUTOCLICK,
  LEDTEST,
  INC_CLICK_SPEED,
  DEC_CLICK_SPEED
};

#define CLICKSPEEDS 2
const uint16_t clickspeed_sets[CLICKSPEEDS] = {50, 20};
uint16_t clickspeed_set = 0;
deferred_token autoclick_token = INVALID_DEFERRED_TOKEN;
deferred_token jiggle_token = INVALID_DEFERRED_TOKEN;
report_mouse_t report = {0};

void decrement_click_speed(void){
  clickspeed_set += CLICKSPEEDS - 1;
  clickspeed_set %= CLICKSPEEDS;
}

void increment_click_speed(void){
  clickspeed_set ++;
  clickspeed_set %= CLICKSPEEDS;
}

uint32_t autoclick_callback(uint32_t trigger_time, void *cb_arg){
  tap_code(KC_BTN1);
  return clickspeed_sets[clickspeed_set];
}

void start_autoclicker(void){
  autoclick_token = defer_exec(1, autoclick_callback, NULL);
}

void stop_autoclicker(void){
  cancel_deferred_exec(autoclick_token);
  autoclick_token = INVALID_DEFERRED_TOKEN;
}


uint32_t jiggler_callback(uint32_t trigger_time, void* cb_arg) {
  // Deltas to move in a circle of radius 20 pixels over 32 frames.
  static const int8_t deltas[32] = {
    0, -1, -2, -2, -3, -3, -4, -4, -4, -4, -3, -3, -2, -2, -1, 0,
    0, 1, 2, 2, 3, 3, 4, 4, 4, 4, 3, 3, 2, 2, 1, 0};
  static uint8_t phase = 0;
  // Get x delta from table and y delta by rotating a quarter cycle.
  report.x = deltas[phase];
  report.y = deltas[(phase + 8) & 31];
  phase = (phase + 1) & 31;
  host_mouse_send(&report);
  return 16;  // Call the callback every 16 ms.
}


const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [0] = LAYOUT(
        KC_MUTE,
        JIGGLE,    LEDTEST,   AUTOCLICK,
        INC_CLICK_SPEED,    DEC_CLICK_SPEED,    KC_P6,
        LAG(KC_M),    KC_P8,    LT(1, KC_P9)
    ),
    [1] = LAYOUT(
        RGB_TOG,
        RGB_HUI,  RGB_SAI,  RGB_SPI,
        RGB_HUD,  RGB_SAD,  RGB_SPD,
        RGB_RMOD, RGB_MOD,  _______
    )
};

#if defined(ENCODER_MAP_ENABLE)
const uint16_t PROGMEM encoder_map[][NUM_ENCODERS][NUM_DIRECTIONS] = {
    [0] = { ENCODER_CCW_CW(KC_VOLD, KC_VOLU) },
    [1] = { ENCODER_CCW_CW(RGB_VAD, RGB_VAI) }
};
#endif

void keyboard_post_init_user(void) {
  // Call the post init code.
  rgblight_enable();
  rgblight_setrgb(0, 0, 0);
}

bool process_record_user(uint16_t keycode, keyrecord_t* record) {
  if (record->event.pressed) {
    switch (keycode) {
      case LEDTEST:
        rgblight_setrgb_at(79, 66, 181, 1);
        break;

      case AUTOCLICK:
        if (record->event.pressed) {
          if (autoclick_token == INVALID_DEFERRED_TOKEN){
            rgblight_sethsv_at(HSV_GREEN, 2);
            start_autoclicker();
          } else{
            stop_autoclicker();
            rgblight_sethsv_at(HSV_BLACK, 2);
          }
        }
        break;

      case INC_CLICK_SPEED:
        increment_click_speed();
        break;

      case DEC_CLICK_SPEED:
        decrement_click_speed();
        break;
     }

     if (jiggle_token) {
       // If jiggler is currently running, stop when any key is pressed.
       cancel_deferred_exec(jiggle_token);
       jiggle_token = INVALID_DEFERRED_TOKEN;
       report = (report_mouse_t){};  // Clear the mouse.
       host_mouse_send(&report);
       rgblight_sethsv_at(HSV_BLACK, 0);
     } else if (keycode == JIGGLE) {
       rgblight_sethsv_at(HSV_GREEN, 0);
       jiggle_token = defer_exec(1, jiggler_callback, NULL);  // Schedule callback.
     }

  }

  return true;
}


