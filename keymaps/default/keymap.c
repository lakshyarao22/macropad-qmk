```c
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
    PASS1 = SAFE_RANGE,
    PASS2,
    PASS3,
    PASS4,
    PASS5,
    PASS6,
    PASS7,
    LAYER_PREV,
    LAYER_NEXT
};


/* =========================
 * Password storage
 * =========================
 *
 * Seven independent password slots.
 *
 * Each slot:
 *   byte 0      = password length
 *   bytes 1-31  = password data
 *
 * Slot size = 32 bytes.
 *
 * EEPROM layout:
 *
 *   Slot 1 -> 100 - 131
 *   Slot 2 -> 132 - 163
 *   Slot 3 -> 164 - 195
 *   Slot 4 -> 196 - 227
 *   Slot 5 -> 228 - 259
 *   Slot 6 -> 260 - 291
 *   Slot 7 -> 292 - 323
 *
 * The actual passwords are NEVER stored in the firmware source.
 * They are written later through Raw HID.
 *
 * NOTE:
 * EEPROM is NOT encrypted. This prevents passwords from being
 * stored in the Git repository, but does not protect against
 * physical/debugger access to the RP2040.
 * ========================= */

#define PASS_SLOT_COUNT 7
#define PASS_MAX_LEN    31

#define PASS_EEPROM_BASE 100
#define PASS_SLOT_SIZE   32


static char pass_buf[PASS_MAX_LEN + 1];
static uint8_t pass_len = 0;


/* =========================
 * Load password from EEPROM
 * ========================= */

static void pass_load_from_eeprom(uint8_t slot) {

    if (slot >= PASS_SLOT_COUNT) {
        pass_len = 0;
        pass_buf[0] = '\0';
        return;
    }

    uint16_t address = PASS_EEPROM_BASE + (slot * PASS_SLOT_SIZE);

    pass_len = eeprom_read_byte((uint8_t *)address);

    if (pass_len > PASS_MAX_LEN) {
        pass_len = 0;
    }

    eeprom_read_block(
        pass_buf,
        (void *)(address + 1),
        pass_len
    );

    pass_buf[pass_len] = '\0';
}


/* =========================
 * Save password to EEPROM
 * ========================= */

static void pass_save_to_eeprom(uint8_t slot, const char *new_pass, uint8_t len) {

    if (slot >= PASS_SLOT_COUNT) {
        return;
    }

    if (len > PASS_MAX_LEN) {
        len = PASS_MAX_LEN;
    }

    uint16_t address = PASS_EEPROM_BASE + (slot * PASS_SLOT_SIZE);

    eeprom_update_byte(
        (uint8_t *)address,
        len
    );

    eeprom_update_block(
        new_pass,
        (void *)(address + 1),
        len
    );
}


/* =========================
 * Send password from slot
 * ========================= */

static void pass_send_slot(uint8_t slot) {

    if (slot >= PASS_SLOT_COUNT) {
        return;
    }

    pass_load_from_eeprom(slot);

    if (pass_len > 0) {
        send_string(pass_buf);
        tap_code(KC_ENT);
    }
}


/* =========================
 * Raw HID
 *
 * Report:
 *
 * [0] = command
 * [1] = slot
 * [2] = password length
 * [3...] = password
 *
 * The USB report itself is 32 bytes.
 * ========================= */

#ifdef RAW_ENABLE

enum raw_hid_commands {
    CMD_SET_PASSWORD = 0x01,
};


void raw_hid_receive(uint8_t *data, uint8_t length) {

    if (length < 3) {
        return;
    }

    uint8_t cmd         = data[0];
    uint8_t slot        = data[1];
    uint8_t payload_len = data[2];


    if (cmd == CMD_SET_PASSWORD) {

        if (slot >= PASS_SLOT_COUNT) {
            return;
        }

        if (payload_len > PASS_MAX_LEN) {
            payload_len = PASS_MAX_LEN;
        }

        if (payload_len > (length - 3)) {
            payload_len = length - 3;
        }

        pass_save_to_eeprom(
            slot,
            (const char *)&data[3],
            payload_len
        );


        /* Send acknowledgement */

        uint8_t response[32] = {0};

        response[0] = CMD_SET_PASSWORD;
        response[1] = slot;
        response[2] = 1;

        raw_hid_send(
            response,
            sizeof(response)
        );
    }
}

#endif


/* =========================
 * Layer colors
 * ========================= */

#define LAYER0_HUE    0
#define LAYER0_SAT    220
#define LAYER0_MAXPCT 65

#define LAYER1_HUE    191
#define LAYER1_SAT    200
#define LAYER1_MAXPCT 100

#define LAYER2_HUE    100
#define LAYER2_SAT    210
#define LAYER2_MAXPCT 100

#define LAYER3_HUE    128
#define LAYER3_SAT    255
#define LAYER3_MAXPCT 100


/* =========================
 * Breathing effect
 * ========================= */

#define BREATH_PERIOD_MS   10000
#define BREATH_MIN_PERCENT 40

#define BREATH_MIN_VAL ((BREATH_MIN_PERCENT * 255) / 100)
#define PCT_TO_VAL(pct) ((pct) * 255 / 100)


static uint8_t current_hue     = LAYER0_HUE;
static uint8_t current_sat     = LAYER0_SAT;
static uint8_t current_max_val = PCT_TO_VAL(LAYER0_MAXPCT);


void housekeeping_task_user(void) {

    static uint32_t last_update = 0;

    if (timer_elapsed32(last_update) < 20) {
        return;
    }

    last_update = timer_read32();


    uint16_t half  = BREATH_PERIOD_MS / 2;
    uint16_t phase = timer_read32() % BREATH_PERIOD_MS;

    uint16_t position =
        (phase < half)
            ? phase
            : (BREATH_PERIOD_MS - phase);


    uint8_t val =
        BREATH_MIN_VAL +
        (uint8_t)(
            (uint32_t)(
                current_max_val -
                BREATH_MIN_VAL
            ) * position / half
        );


    rgblight_sethsv_noeeprom(
        current_hue,
        current_sat,
        val
    );
}


/* =========================
 * Keyboard initialization
 * ========================= */

void keyboard_post_init_user(void) {

    pass_load_from_eeprom(0);

    rgblight_enable_noeeprom();

    rgblight_mode_noeeprom(
        RGBLIGHT_MODE_STATIC_LIGHT
    );

    rgblight_sethsv_noeeprom(
        current_hue,
        current_sat,
        current_max_val
    );
}


/* =========================
 * Layer RGB
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
     * Layer 2 - Passwords
     * ===================== */

    [2] = LAYOUT(
        PASS1, PASS2, PASS3,
        PASS4, PASS5, PASS6,
        LAYER_PREV, PASS7, LAYER_NEXT
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

bool process_record_user(
    uint16_t keycode,
    keyrecord_t *record
) {

    if (!record->event.pressed) {
        return true;
    }


    switch (keycode) {

        /* -------------------------
         * Password 1
         * ------------------------- */

        case PASS1:
            pass_send_slot(0);
            return false;


        /* -------------------------
         * Password 2
         * ------------------------- */

        case PASS2:
            pass_send_slot(1);
            return false;


        /* -------------------------
         * Password 3
         * ------------------------- */

        case PASS3:
            pass_send_slot(2);
            return false;


        /* -------------------------
         * Password 4
         * ------------------------- */

        case PASS4:
            pass_send_slot(3);
            return false;


        /* -------------------------
         * Password 5
         * ------------------------- */

        case PASS5:
            pass_send_slot(4);
            return false;


        /* -------------------------
         * Password 6
         * ------------------------- */

        case PASS6:
            pass_send_slot(5);
            return false;


        /* -------------------------
         * Password 7
         * ------------------------- */

        case PASS7:
            pass_send_slot(6);
            return false;


        /* -------------------------
         * Previous layer
         * ------------------------- */

        case LAYER_PREV: {

            uint8_t current_layer =
                get_highest_layer(layer_state);


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

            uint8_t current_layer =
                get_highest_layer(layer_state);


            if (current_layer >= 3) {
                layer_move(0);
            } else {
                layer_move(current_layer + 1);
            }

            return false;
        }
    }


    return true;
}


#ifdef OTHER_KEYMAP_C
#    include OTHER_KEYMAP_C
#endif
```
