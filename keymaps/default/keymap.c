#include QMK_KEYBOARD_H
#include <eeprom.h>
#include "raw_hid.h"

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
 * Password storage (EEPROM, set via Raw HID after flashing)
 * The firmware source never contains the actual password - it
 * lives only in the keyboard's on-chip EEPROM, written later by
 * a small local script (see set_password.py) over Raw HID.
 * NOTE: EEPROM here is NOT encrypted - this keeps the secret out
 * of the git repo, it isn't vault-grade protection against someone
 * with physical/debugger access to the flashed board.
 * ========================= */

#define PASS_MAX_LEN     32
#define PASS_EEPROM_ADDR 100  /* pick an address unused by other EEPROM users (e.g. VIA/dynamic keymaps) */

static char    pass_buf[PASS_MAX_LEN + 1]; /* +1 for null terminator */
static uint8_t pass_len = 0;

static void pass_load_from_eeprom(void) {
    pass_len = eeprom_read_byte((uint8_t *)PASS_EEPROM_ADDR);
    if (pass_len > PASS_MAX_LEN) {
        pass_len = 0; /* guard against uninitialized/garbage EEPROM (0xFF on first boot) */
    }
    eeprom_read_block(pass_buf, (void *)(PASS_EEPROM_ADDR + 1), pass_len);
    pass_buf[pass_len] = '\0';
}

static void pass_save_to_eeprom(const char *new_pass, uint8_t len) {
    if (len > PASS_MAX_LEN) {
        len = PASS_MAX_LEN;
    }
    eeprom_update_byte((uint8_t *)PASS_EEPROM_ADDR, len);
    eeprom_update_block(new_pass, (void *)(PASS_EEPROM_ADDR + 1), len);
    pass_load_from_eeprom(); /* refresh the RAM copy */
}


/* =========================
 * Raw HID - receives the "set password" command from
 * set_password.py after the firmware is already flashed.
 * Report layout: [cmd][len][...password bytes...]
 * ========================= */

#ifdef RAW_ENABLE

enum raw_hid_commands {
    CMD_SET_PASSWORD = 0x01,
};

void raw_hid_receive(uint8_t *data, uint8_t length) {
    if (length < 2) {
        return;
    }

    uint8_t cmd        = data[0];
    uint8_t payload_len = data[1];

    if (cmd == CMD_SET_PASSWORD) {
        if (payload_len > (length - 2)) {
            payload_len = length - 2; /* don't read past what was actually sent */
        }
        pass_save_to_eeprom((const char *)&data[2], payload_len);

        uint8_t response[32] = {0};
        response[0] = CMD_SET_PASSWORD;
        response[1] = 1; /* ack */
        raw_hid_send(response, sizeof(response));
    }
}

#endif


/* =========================
 * Layer colors (Hue, Saturation, Max brightness %)
 * Softer, non-primary hues instead of harsh pure R/G/B, spread
 * further apart on the hue wheel so adjacent layers don't blur
 * together. Peak brightness (Max %) is per-layer because a color
 * like maroon is defined by being dark - at 100% it just becomes
 * plain red, so its ceiling is capped lower than the others.
 * ========================= */

#define LAYER0_HUE    0    /* Maroon  - deep red                        */
#define LAYER0_SAT    220
#define LAYER0_MAXPCT 65   /* capped so it stays dark even at peak      */

#define LAYER1_HUE    191  /* Purple                                    */
#define LAYER1_SAT    200
#define LAYER1_MAXPCT 100

#define LAYER2_HUE    100  /* Emerald - true green, well clear of cyan  */
#define LAYER2_SAT    210
#define LAYER2_MAXPCT 100

#define LAYER3_HUE    128  /* Cyan                                      */
#define LAYER3_SAT    255
#define LAYER3_MAXPCT 100


/* =========================
 * Manual breathing effect
 * QMK's built-in RGBLIGHT_MODE_BREATHING dims all the way to 0%
 * at the bottom of its curve. This drives brightness ourselves
 * with a triangle wave clamped between BREATH_MIN_PERCENT and
 * each layer's own max, so it never goes fully dark and layers
 * like maroon never blow out to full brightness either.
 * ========================= */

#define BREATH_PERIOD_MS   10000  /* full cycle length (up+down), ms - lower = faster */
#define BREATH_MIN_PERCENT 40    /* floor - never dims below this, all layers */

#define BREATH_MIN_VAL ((BREATH_MIN_PERCENT * 255) / 100)
#define PCT_TO_VAL(pct) ((pct) * 255 / 100)

static uint8_t current_hue     = LAYER0_HUE;
static uint8_t current_sat     = LAYER0_SAT;
static uint8_t current_max_val = PCT_TO_VAL(LAYER0_MAXPCT);

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
                  (uint8_t)((uint32_t)(current_max_val - BREATH_MIN_VAL) * position / half);

    rgblight_sethsv_noeeprom(current_hue, current_sat, val);
}


/* =========================
 * Keyboard initialization
 * ========================= */

void keyboard_post_init_user(void) {
    pass_load_from_eeprom();

    rgblight_enable_noeeprom();
    rgblight_mode_noeeprom(RGBLIGHT_MODE_STATIC_LIGHT); /* base mode - brightness driven by housekeeping_task_user */
    rgblight_sethsv_noeeprom(current_hue, current_sat, current_max_val);
}


/* =========================
 * Layer RGB - breathing color changes with layer
 * ========================= */

layer_state_t layer_state_set_user(layer_state_t state) {
    switch (get_highest_layer(state)) {
        case 0:
            current_hue     = LAYER0_HUE;
            current_sat     = LAYER0_SAT;
            current_max_val = PCT_TO_VAL(LAYER0_MAXPCT);
            break;

        case 1:
            current_hue     = LAYER1_HUE;
            current_sat     = LAYER1_SAT;
            current_max_val = PCT_TO_VAL(LAYER1_MAXPCT);
            break;

        case 2:
            current_hue     = LAYER2_HUE;
            current_sat     = LAYER2_SAT;
            current_max_val = PCT_TO_VAL(LAYER2_MAXPCT);
            break;

        case 3:
            current_hue     = LAYER3_HUE;
            current_sat     = LAYER3_SAT;
            current_max_val = PCT_TO_VAL(LAYER3_MAXPCT);
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
         * PASS key - sends whatever was stored via Raw HID,
         * followed by Enter. If nothing has been set yet,
         * pass_len is 0 and this no-ops entirely.
         * ------------------------- */
        case PASS:
            if (pass_len > 0) {
                send_string(pass_buf);
                tap_code(KC_ENT);
            }
            return false;
    }

    return true;
}


#ifdef OTHER_KEYMAP_C
#    include OTHER_KEYMAP_C
#endif