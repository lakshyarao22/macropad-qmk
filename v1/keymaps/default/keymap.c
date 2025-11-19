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