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
 * Layer colors (Hue, Saturation)
 * Softer, non-primary hues instead of harsh pure R/G/B.
 * Brightness (V) is NOT set here - it's driven separately
 * by the manual breathing task below, so it can be floored.
 * ========================= */

#define LAYER0_HUE 11   /* Coral     - warm, not alarm-red   */
#define LAYER0_SAT 200

#define LAYER1_HUE 191  /* Purple                             */
#define LAYER1_SAT 200

#define LAYER2_HUE 123  /* Turquoise - green-adjacent, soft  */
#define LAYER2_SAT 200

#define LAYER3_HUE 128  /* Cyan                               */
#define LAYER3_SAT 255


/* =========================
 * Manual breathing effect
 * QMK's built-in RGBLIGHT_MODE_BREATHING dims all the way to 0%
 * at the bottom of its curve. This drives brightness ourselves
 * with a triangle wave clamped between BREATH_MIN_PERCENT and
 * BREATH_MAX_PERCENT so it never goes fully dark.
 * ========================= */

#define BREATH_PERIOD_MS   3000  /* full cycle length (up+down), ms - lower = faster */
#define BREATH_MIN_PERCENT 20    /* floor - never dims below this   */
#define BREATH_MAX_PERCENT 100

#define BREATH_MIN_VAL ((BREATH_MIN_PERCENT * 255) / 100)
#define BREATH_MAX_VAL ((BREATH_MAX_PERCENT * 255) / 100)

static uint8_t current_hue = LAYER0_HUE;
static uint8_t current_sat = LAYER0_SAT;

void housekeeping_task_user(void) {
    static uint32_t last_update = 0;

    if (timer_elapsed32(last_update) < 20) {
        return; /* ~50 updates/sec - smooth without hammering the LED driver */
    }
    last_update = timer_read32();

    uint16_t half     = BREATH_PERIOD_MS / 2;
    uint16_t phase    = timer_read32() % BREATH_PERIOD_MS;
    uint16_t position  = (phase < half) ? phase : (BREATH_PERIOD_MS - phase); /* 0..half..0 */

    uint8_t val = BREATH_MIN_VAL +
                  (uint8_t)((uint32_t)(BREATH_MAX_VAL - BREATH_MIN_VAL) * position / half);

    rgblight_sethsv_noeeprom(current_hue, current_sat, val);
}


/* =========================
 * Keyboard initialization
 * ========================= */

void keyboard_post_init_user(void) {
    rgblight_enable_noeeprom();
    rgblight_mode_noeeprom(RGBLIGHT_MODE_STATIC_LIGHT); /* base mode - brightness driven by housekeeping_task_user */
    rgblight_sethsv_noeeprom(current_hue, current_sat, BREATH_MAX_VAL);
}


/* =========================
 * Layer RGB - breathing color changes with layer
 * ========================= */

layer_state_t layer_state_set_user(layer_state_t state) {
    switch (get_highest_layer(state)) {
        case 0:
            current_hue = LAYER0_HUE;
            current_sat = LAYER0_SAT;
            break;

        case 1:
            current_hue = LAYER1_HUE;
            current_sat = LAYER1_SAT;
            break;

        case 2:
            current_hue = LAYER2_HUE;
            current_sat = LAYER2_SAT;
            break;

        case 3:
            current_hue = LAYER3_HUE;
            current_sat = LAYER3_SAT;
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