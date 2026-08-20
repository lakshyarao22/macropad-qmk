#include QMK_KEYBOARD_H
#include <eeprom.h>
#include "raw_hid.h"
#include <stdint.h>

#if __has_include("keymap.h")
#    include "keymap.h"
#endif


/* =========================================================
 * Custom keycodes
 * ========================================================= */

enum custom_keycodes {
    PASS1 = SAFE_RANGE,
    PASS2,
    PASS3,
    PASS4,
    PASS5,
    PASS6,
    PASS7,

    VOL_DOWN_TRACK,
    VOL_UP_TRACK,

    LAYER_PREV,
    LAYER_NEXT
};


/* =========================================================
 * Password storage
 *
 * Seven password slots.
 *
 * Each slot:
 *
 *   byte 0      = password length
 *   bytes 1-29  = password
 *
 * Slot size = 32 bytes.
 *
 * We start at logical EEPROM address 256 to leave the
 * beginning of the EEPROM area available to QMK.
 *
 * Slot 1 -> 256 - 287
 * Slot 2 -> 288 - 319
 * Slot 3 -> 320 - 351
 * Slot 4 -> 352 - 383
 * Slot 5 -> 384 - 415
 * Slot 6 -> 416 - 447
 * Slot 7 -> 448 - 479
 *
 * Passwords are written later using Raw HID.
 *
 * IMPORTANT:
 * EEPROM is not encrypted.
 * ========================================================= */

#define PASS_SLOT_COUNT  7
#define PASS_MAX_LEN     29

#define PASS_EEPROM_BASE 256
#define PASS_SLOT_SIZE   32


static char pass_buf[PASS_MAX_LEN + 1];
static uint8_t pass_len = 0;


/* =========================================================
 * Load password from EEPROM
 * ========================================================= */

static void pass_load_from_eeprom(uint8_t slot) {

    if (slot >= PASS_SLOT_COUNT) {
        pass_len = 0;
        pass_buf[0] = '\0';
        return;
    }

    uint16_t address =
        PASS_EEPROM_BASE +
        (slot * PASS_SLOT_SIZE);

    pass_len =
        eeprom_read_byte(
            (uint8_t *)address
        );

    if (pass_len > PASS_MAX_LEN) {
        pass_len = 0;
    }

    eeprom_read_block(
        pass_buf,
        (const void *)(address + 1),
        pass_len
    );

    pass_buf[pass_len] = '\0';
}


static void pass_save_to_eeprom(
    uint8_t slot,
    const char *new_pass,
    uint8_t len
) {

    if (slot >= PASS_SLOT_COUNT) {
        return;
    }

    if (len > PASS_MAX_LEN) {
        len = PASS_MAX_LEN;
    }

    uint16_t address =
        PASS_EEPROM_BASE +
        (slot * PASS_SLOT_SIZE);

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


/* =========================================================
 * Send password from EEPROM slot
 * ========================================================= */

static void pass_send_slot(uint8_t slot) {

    if (slot >= PASS_SLOT_COUNT) {
        return;
    }

    pass_load_from_eeprom(slot);

    if (pass_len == 0) {
        return;
    }

    send_string(pass_buf);

    tap_code(KC_ENT);
}


/* =========================================================
 * Raw HID
 *
 * Packet:
 *
 *   data[0] = command
 *   data[1] = slot
 *   data[2] = password length
 *   data[3...] = password
 *
 * Example:
 *
 *   01 00 08 password
 *
 * means:
 *
 *   command = SET_PASSWORD
 *   slot    = 0
 *   length  = 8
 * ========================================================= */

#ifdef RAW_ENABLE

enum raw_hid_commands {
    CMD_SET_PASSWORD = 0x01,
};

void raw_hid_receive(uint8_t *data, uint8_t length) {

    if (length < 3) {
        return;
    }

    uint8_t cmd        = data[0];
    uint8_t slot       = data[1];
    uint8_t payload_len = data[2];

    if (cmd != CMD_SET_PASSWORD) {
        return;
    }

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

    /*
     * QMK Raw HID requires exactly 32 bytes.
     *
     * Response:
     *
     * byte 0 = command
     * byte 1 = slot
     * byte 2 = success
     */

    uint8_t response[RAW_EPSIZE] = {0};

    response[0] = CMD_SET_PASSWORD;
    response[1] = slot;
    response[2] = 1;

    raw_hid_send(
        response,
        RAW_EPSIZE
    );
}

#endif


/* =========================================================
 * Layer colors
 * ========================================================= */

#define LAYER0_HUE     0
#define LAYER0_SAT     220
#define LAYER0_MAXPCT  65

#define LAYER1_HUE     191
#define LAYER1_SAT     200
#define LAYER1_MAXPCT  100

#define LAYER2_HUE     100
#define LAYER2_SAT     210
#define LAYER2_MAXPCT  100

#define LAYER3_HUE     128
#define LAYER3_SAT     255
#define LAYER3_MAXPCT  100


/* =========================================================
 * Breathing RGB
 * ========================================================= */

#define BREATH_PERIOD_MS    10000
#define BREATH_MIN_PERCENT  40

#define BREATH_MIN_VAL \
    ((BREATH_MIN_PERCENT * 255) / 100)

#define PCT_TO_VAL(pct) \
    ((pct) * 255 / 100)


static uint8_t current_hue =
    LAYER0_HUE;

static uint8_t current_sat =
    LAYER0_SAT;

static uint8_t current_max_val =
    PCT_TO_VAL(LAYER0_MAXPCT);


/* =========================================================
 * Volume / track handling
 *
 * Tap:
 *   Volume Down / Volume Up
 *
 * Hold:
 *   Previous Track / Next Track
 * ========================================================= */

#define VOLUME_HOLD_TIME 400

static uint16_t volume_down_timer = 0;
static uint16_t volume_up_timer   = 0;

static bool volume_down_active = false;
static bool volume_up_active   = false;

static bool volume_down_held = false;
static bool volume_up_held   = false;


/* =========================================================
 * RGB housekeeping
 *
 * This directly controls the three WS2812 LEDs.
 *
 * We deliberately do NOT use UG_TOGG here because
 * UG_TOGG controls the same LEDs that we use for layer
 * indication.
 * ========================================================= */

void housekeeping_task_user(void) {

    static uint32_t last_update = 0;

    if (timer_elapsed32(last_update) < 20) {
        return;
    }

    last_update = timer_read32();

    uint16_t half =
        BREATH_PERIOD_MS / 2;

    uint16_t phase =
        timer_read32() % BREATH_PERIOD_MS;

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


/* =========================================================
 * Keyboard initialization
 * ========================================================= */

void keyboard_post_init_user(void) {

    /*
     * Load first password slot.
     */

    pass_load_from_eeprom(0);

    /*
     * Enable RGB.
     */

    rgblight_enable_noeeprom();

    /*
     * Static mode.
     *
     * Our housekeeping_task_user() controls brightness.
     */

    rgblight_mode_noeeprom(
        RGBLIGHT_MODE_STATIC_LIGHT
    );

    rgblight_sethsv_noeeprom(
        current_hue,
        current_sat,
        current_max_val
    );
}


/* =========================================================
 * Layer RGB
 * ========================================================= */

layer_state_t layer_state_set_user(
    layer_state_t state
) {

    switch (get_highest_layer(state)) {

        case 0:

            current_hue =
                LAYER0_HUE;

            current_sat =
                LAYER0_SAT;

            current_max_val =
                PCT_TO_VAL(LAYER0_MAXPCT);

            break;


        case 1:

            current_hue =
                LAYER1_HUE;

            current_sat =
                LAYER1_SAT;

            current_max_val =
                PCT_TO_VAL(LAYER1_MAXPCT);

            break;


        case 2:

            current_hue =
                LAYER2_HUE;

            current_sat =
                LAYER2_SAT;

            current_max_val =
                PCT_TO_VAL(LAYER2_MAXPCT);

            break;


        case 3:

            current_hue =
                LAYER3_HUE;

            current_sat =
                LAYER3_SAT;

            current_max_val =
                PCT_TO_VAL(LAYER3_MAXPCT);

            break;
    }

    return state;
}


/* =========================================================
 * Keymaps
 * ========================================================= */

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {

    /* =====================================================
     * Layer 0
     * ===================================================== */

    [0] = LAYOUT(

        KC_F13,
        KC_F14,
        KC_F15,

        KC_F16,
        KC_F17,
        KC_F18,

        LAYER_PREV,
        KC_F19,
        LAYER_NEXT
    ),


    /* =====================================================
     * Layer 1
     *
     * Tap volume keys:
     *
     *   Left  = Volume Down
     *   Center = Mute
     *   Right = Volume Up
     *
     * Hold:
     *
     *   Left  = Previous Track
     *   Right = Next Track
     *
     * Bottom middle/right:
     *
     *   Brightness Down / Brightness Up
     * ===================================================== */

    [1] = LAYOUT(

        VOL_DOWN_TRACK,
        KC_MUTE,
        VOL_UP_TRACK,

        KC_BRID,
        KC_MPLY,
        KC_BRIU,

        LAYER_PREV,
        KC_NO,
        LAYER_NEXT
    ),


    /* =====================================================
     * Layer 2
     *
     * Passwords
     * ===================================================== */

    [2] = LAYOUT(

        PASS1,
        PASS2,
        PASS3,

        PASS4,
        PASS5,
        PASS6,

        LAYER_PREV,
        PASS7,
        LAYER_NEXT
    ),


    /* =====================================================
     * Layer 3
     *
     * RGB controls
     *
     * IMPORTANT:
     *
     * UG_TOGG is intentionally removed.
     *
     * The RGB LEDs are the layer indicators, so toggling
     * RGB off would also remove the layer indication.
     * ===================================================== */

    [3] = LAYOUT(

        KC_NO,
        UG_NEXT,
        UG_HUEU,

        UG_VALD,
        UG_VALU,
        UG_HUED,

        LAYER_PREV,
        UG_SPDU,
        LAYER_NEXT
    )
};


/* =========================================================
 * Process keycodes
 * ========================================================= */

bool process_record_user(
    uint16_t keycode,
    keyrecord_t *record
) {

    switch (keycode) {

        /* =================================================
         * Password 1
         * ================================================= */

        case PASS1:

            if (record->event.pressed) {
                pass_send_slot(0);
            }

            return false;


        /* =================================================
         * Password 2
         * ================================================= */

        case PASS2:

            if (record->event.pressed) {
                pass_send_slot(1);
            }

            return false;


        /* =================================================
         * Password 3
         * ================================================= */

        case PASS3:

            if (record->event.pressed) {
                pass_send_slot(2);
            }

            return false;


        /* =================================================
         * Password 4
         * ================================================= */

        case PASS4:

            if (record->event.pressed) {
                pass_send_slot(3);
            }

            return false;


        /* =================================================
         * Password 5
         * ================================================= */

        case PASS5:

            if (record->event.pressed) {
                pass_send_slot(4);
            }

            return false;


        /* =================================================
         * Password 6
         * ================================================= */

        case PASS6:

            if (record->event.pressed) {
                pass_send_slot(5);
            }

            return false;


        /* =================================================
         * Password 7
         * ================================================= */

        case PASS7:

            if (record->event.pressed) {
                pass_send_slot(6);
            }

            return false;


        /* =================================================
         * Volume Down / Previous Track
         * ================================================= */

        case VOL_DOWN_TRACK:

            if (record->event.pressed) {

                volume_down_timer =
                    timer_read();

                volume_down_active = true;
                volume_down_held = false;

                return false;
            }

            if (volume_down_active) {

                if (!volume_down_held) {
                    tap_code(KC_VOLD);
                }
            }

            volume_down_active = false;
            volume_down_held = false;

            return false;


        /* =================================================
         * Volume Up / Next Track
         * ================================================= */

        case VOL_UP_TRACK:

            if (record->event.pressed) {

                volume_up_timer =
                    timer_read();

                volume_up_active = true;
                volume_up_held = false;

                return false;
            }

            if (volume_up_active) {

                if (!volume_up_held) {
                    tap_code(KC_VOLU);
                }
            }

            volume_up_active = false;
            volume_up_held = false;

            return false;


        /* =================================================
         * Previous Layer
         * ================================================= */

        case LAYER_PREV:

            if (record->event.pressed) {

                uint8_t current_layer =
                    get_highest_layer(layer_state);

                if (current_layer == 0) {

                    layer_move(3);

                } else {

                    layer_move(
                        current_layer - 1
                    );
                }
            }

            return false;


        /* =================================================
         * Next Layer
         * ================================================= */

        case LAYER_NEXT:

            if (record->event.pressed) {

                uint8_t current_layer =
                    get_highest_layer(layer_state);

                if (current_layer >= 3) {

                    layer_move(0);

                } else {

                    layer_move(
                        current_layer + 1
                    );
                }
            }

            return false;
    }

    return true;
}


/* =========================================================
 * Matrix scan
 *
 * Volume:
 *
 *   Tap  = Volume
 *   Hold = Track
 * ========================================================= */

void matrix_scan_user(void) {

    /* =====================================================
     * Volume Down -> Previous Track
     * ===================================================== */

    if (
        volume_down_active &&
        !volume_down_held &&
        timer_elapsed(volume_down_timer)
            >= VOLUME_HOLD_TIME
    ) {

        volume_down_held = true;

        tap_code(KC_MPRV);
    }


    /* =====================================================
     * Volume Up -> Next Track
     * ===================================================== */

    if (
        volume_up_active &&
        !volume_up_held &&
        timer_elapsed(volume_up_timer)
            >= VOLUME_HOLD_TIME
    ) {

        volume_up_held = true;

        tap_code(KC_MNXT);
    }
}


#ifdef OTHER_KEYMAP_C
#    include OTHER_KEYMAP_C
#endif