#include QMK_KEYBOARD_H

#if __has_include("keymap.h")
#    include "keymap.h"
#endif


/* =========================
 * Custom keycodes
 * ========================= */

enum custom_keycodes {
    PASS = SAFE_RANGE,
    LAYER_PREV,
    LAYER_NEXT
};


/* =========================
 * Keyboard initialization
 * ========================= */

void keyboard_post_init_user(void) {
    /* One continuous effect: breathing. No layer segments, no blinking. */
    rgblight_enable_noeeprom();
    rgblight_mode_noeeprom(RGBLIGHT_MODE_BREATHING);
    rgblight_sethsv_noeeprom(HSV_RED); /* starting layer (0) color */
}


/* =========================
 * Layer RGB - breathing color changes with layer
 * ========================= */

layer_state_t layer_state_set_user(layer_state_t state) {
    switch (get_highest_layer(state)) {
        case 0:
            rgblight_sethsv_noeeprom(HSV_RED);
            break;

        case 1:
            rgblight_sethsv_noeeprom(HSV_BLUE);
            break;

        case 2:
            rgblight_sethsv_noeeprom(HSV_GREEN);
            break;

        case 3:
            rgblight_sethsv_noeeprom(HSV_CYAN);
            break;
    }

    return state;
}


/* =========================
 * Keymaps
 * ========================= */

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {

    /* =====================
     * Layer 0 - F13/F21
     * ===================== */
    [0] = LAYOUT(
        KC_F13, KC_F14, KC_F15,
        KC_F16, KC_F17, KC_F18,
        LAYER_PREV, KC_F19, LAYER_NEXT
    ),

    /* =====================
     * Layer 1 - Media
     * ===================== */
    [1] = LAYOUT(
        KC_VOLD, KC_MUTE, KC_VOLU,
        KC_MPRV, KC_MPLY, KC_MNXT,
        LAYER_PREV, KC_NO, LAYER_NEXT
    ),

    /* =====================
     * Layer 2 - Shortcuts
     * ===================== */
    [2] = LAYOUT(
        KC_BRID, KC_NO, KC_BRIU,
        LSG(KC_S), LGUI(KC_L), PASS,
        LAYER_PREV, KC_NO, LAYER_NEXT
    ),

    /* =====================
     * Layer 3 - RGB Controls
     * ===================== */
    [3] = LAYOUT(
        UG_TOGG, UG_NEXT, UG_HUEU,
        UG_VALD, UG_VALU, UG_HUED,
        LAYER_PREV, UG_SPDU, LAYER_NEXT
    )
};


/* =========================
 * Custom keycodes
 * ========================= */

bool process_record_user(uint16_t keycode, keyrecord_t *record) {

    /* Only act when key is pressed */
    if (!record->event.pressed) {
        return true;
    }

    switch (keycode) {

        /* -------------------------
         * Previous layer
         * ------------------------- */
        case LAYER_PREV: {
            uint8_t current_layer = get_highest_layer(layer_state);

            if (current_layer == 0) {
                layer_move(3);
            } else {
                layer_move(current_layer - 1);
            }

            return false;
        }


        /* -------------------------
         * Next layer
         * ------------------------- */
        case LAYER_NEXT: {
            uint8_t current_layer = get_highest_layer(layer_state);

            if (current_layer >= 3) {
                layer_move(0);
            } else {
                layer_move(current_layer + 1);
            }

            return false;
        }


        /* -------------------------
         * PASS key
         * ------------------------- */
        case PASS:
            SEND_STRING("!Hello\n");
            return false;
    }

    return true;
}


#ifdef OTHER_KEYMAP_C
#    include OTHER_KEYMAP_C
#endif