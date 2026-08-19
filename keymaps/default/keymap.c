#include QMK_KEYBOARD_H

#if __has_include("keymap.h")
#    include "keymap.h"
#endif


/* =========================
 * Custom keycodes
 * ========================= */

enum custom_keycodes {
    PASS = SAFE_RANGE
};


/* =========================
 * RGB Lighting Layers
 * ========================= */

/* Layer 0 = RED */
const rgblight_segment_t PROGMEM layer0_rgb[] =
    RGBLIGHT_LAYER_SEGMENTS(
        {0, 3, HSV_RED}
    );

/* Layer 1 = BLUE */
const rgblight_segment_t PROGMEM layer1_rgb[] =
    RGBLIGHT_LAYER_SEGMENTS(
        {0, 3, HSV_BLUE}
    );

/* Layer 2 = GREEN */
const rgblight_segment_t PROGMEM layer2_rgb[] =
    RGBLIGHT_LAYER_SEGMENTS(
        {0, 3, HSV_GREEN}
    );

/* Layer 3 = CYAN */
const rgblight_segment_t PROGMEM layer3_rgb[] =
    RGBLIGHT_LAYER_SEGMENTS(
        {0, 3, HSV_CYAN}
    );


/* RGB lighting layer list */
const rgblight_segment_t* const PROGMEM my_rgb_layers[] =
    RGBLIGHT_LAYERS_LIST(
        layer0_rgb,
        layer1_rgb,
        layer2_rgb,
        layer3_rgb
    );


/* =========================
 * Keyboard initialization
 * ========================= */

void keyboard_post_init_user(void) {
    rgblight_layers = my_rgb_layers;
}


/* =========================
 * Layer RGB blinking
 * ========================= */

layer_state_t layer_state_set_user(layer_state_t state) {
    uint8_t layer = get_highest_layer(state);

    switch (layer) {
        case 0:
            rgblight_blink_layer_repeat(0, 200, 2);
            break;

        case 1:
            rgblight_blink_layer_repeat(1, 200, 2);
            break;

        case 2:
            rgblight_blink_layer_repeat(2, 200, 2);
            break;

        case 3:
            rgblight_blink_layer_repeat(3, 200, 2);
            break;
    }

    return state;
}


/* =========================
 * Keymaps
 * ========================= */

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {

    [0] = LAYOUT(
        KC_F13, KC_F14, KC_F15,
        KC_F16, KC_F17, KC_F18,
        TG(3), KC_F19, TG(1)
    ),

    [1] = LAYOUT(
        KC_VOLD, KC_MUTE, KC_VOLU,
        KC_MPRV, KC_MPLY, KC_MNXT,
        TG(0), KC_NO, TG(2)
    ),

    [2] = LAYOUT(
        KC_BRID, KC_NO, KC_BRIU,
        LSG(KC_S), LGUI(KC_L), PASS,
        TG(1), KC_NO, TG(3)
    ),

    [3] = LAYOUT(
        UG_TOGG, UG_NEXT, UG_HUEU,
        UG_VALD, UG_VALU, UG_HUED,
        UG_SPDD, UG_SPDU, TG(0)
    )
};


/* =========================
 * Custom keycodes
 * ========================= */

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
#endif