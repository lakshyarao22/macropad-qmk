// Copyright 2023 QMK
// SPDX-License-Identifier: GPL-2.0-or-later

#include QMK_KEYBOARD_H
#define WS2812_DI_PIN 13

enum layer_names {
    _LAYER0,
    _LAYER1,
    _LAYER2,
    _LAYER3
};

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    /*
     * Layer 0 - Numpad
     * ┌───┬───┬───┬
     * │ 1 │ 2 │ 3 │
     * ├───┼───┼───┼
     * │ 4 │ 5 │ 6 │
     * ├───┼───┼───┼
     * │ 7 │ 8 │ 9 │
     * └───┴───┴───┘
     *

    /*
     * Layer 0 - Media & Functions
     */
    [_LAYER0] = LAYOUT(
        KC_MUTE,    KC_VOLU,    KC_VOLD,
        KC_MPRV,    KC_MPLY,    KC_MNXT,
        KC_HOME,    KC_END,     MO(_LAYER1) ),
    }

const rgblight_segment_t PROGMEM my_capslock_layer[] = RGBLIGHT_LAYER_SEGMENTS(
    {0, RGBLED_NUM, HSV_RED}
);

// Now define the array of layers. Later layers take precedence
const rgblight_segment_t* const PROGMEM my_rgb_layers[] = RGBLIGHT_LAYERS_LIST(
    my_capslock_layer
);

void keyboard_post_init_user(void) {
    // Enable the LED layers
    rgblight_layers = my_rgb_layers;
}

bool led_update_user(led_t led_state) {
    rgblight_set_layer_state(0, led_state.caps_lock);
    return true;
}