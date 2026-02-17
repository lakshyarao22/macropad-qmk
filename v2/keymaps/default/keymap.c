#include QMK_KEYBOARD_H
#if __has_include("keymap.h")
#    include "keymap.h"
#endif
#include "ws2812.h"

/* Custom keycodes */
enum custom_keycodes {
    PASS = SAFE_RANGE
};

void set_backlight_color(uint8_t r, uint8_t g, uint8_t b) {
    ws2812_led_t led = {
        .r = r,
        .g = g,
        .b = b
    };
    ws2812_set_leds(&led, 1);
}

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [0] = LAYOUT(
        KC_F13, KC_F14, KC_F15,
        KC_F16, KC_F17, KC_F18,
        LT(3,KC_F19), LT(2,KC_F20), LT(1,KC_F21)
    ),

    [1] = LAYOUT(
        KC_VOLD, KC_MUTE, KC_VOLU,
        KC_MPRV, KC_MPLY, KC_MNXT,
        KC_BRIU, KC_BRID, KC_NO
    ),

    [2] = LAYOUT(
        KC_NO, KC_NO, KC_NO,
        LSG(KC_S), LGUI(KC_L), PASS,
        KC_NO, KC_NO, KC_NO
    ),

    [3] = LAYOUT(
        UG_TOGG, UG_NEXT, UG_HUEU,
        UG_VALU, UG_VALD, UG_HUED,
        KC_NO, UG_SPDD, UG_SPDU
    )
};

layer_state_t layer_state_set_user(layer_state_t state) {
    switch (get_highest_layer(state)) {

        case 0:
            set_backlight_color(0, 0, 0);
            break;

        case 1:
            set_backlight_color(0, 0, 255);
            break;

        case 2:
            set_backlight_color(0, 255, 0);
            break;

        case 3:
            set_backlight_color(255, 0, 0);
            break;
    }

    return state;
}

/* Handle custom keycodes */
bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    switch (keycode) {
        case PASS:
            if (record->event.pressed) {
                SEND_STRING("!H@teEatingTyres\n");
            }
            return false;
    }
    return true;
}


#ifdef OTHER_KEYMAP_C
#    include OTHER_KEYMAP_C
#endif // OTHER_KEYMAP_C