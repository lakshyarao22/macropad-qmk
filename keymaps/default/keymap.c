#include QMK_KEYBOARD_H
#if __has_include("keymap.h")
#    include "keymap.h"
#endif

/* Custom keycodes */
enum custom_keycodes {
    PASS = SAFE_RANGE
};


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


/* Handle custom keycodes */
bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    switch (keycode) {
        case PASS:
            if (record->event.pressed) {
                SEND_STRING("!Hello\n");
            }
            return false;
    }
    return true;
}


#ifdef OTHER_KEYMAP_C
#    include OTHER_KEYMAP_C
#endif // OTHER_KEYMAP_C