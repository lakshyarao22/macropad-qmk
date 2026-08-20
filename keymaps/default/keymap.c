#include QMK_KEYBOARD_H
#include <eeprom.h>
#include "raw_hid.h"
#include <stdint.h>

#if __has_include("keymap.h")
#    include "keymap.h"
#endif


/* =========================================================
 * CUSTOM KEYCODES
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
 * PASSWORD STORAGE
 *
 * 7 password slots.
 *
 * Each slot:
 *
 *   byte 0      = password length
 *   bytes 1-31  = password
 *
 * Slot size = 32 bytes.
 *
 * Slot 1 = address 100
 * Slot 2 = address 132
 * Slot 3 = address 164
 * Slot 4 = address 196
 * Slot 5 = address 228
 * Slot 6 = address 260
 * Slot 7 = address 292
 *
 * EEPROM is NOT encrypted.
 * ========================================================= */

#define PASS_SLOT_COUNT 7
#define PASS_MAX_LEN    31

#define PASS_EEPROM_BASE 100
#define PASS_SLOT_SIZE   32


static char pass_buf[PASS_MAX_LEN + 1];
static uint8_t pass_len = 0;


/* =========================================================
 * LOAD PASSWORD FROM EEPROM
 * ========================================================= */

static void pass_load_from_eeprom(uint8_t slot) {

    if (slot >= PASS_SLOT_COUNT) {
        pass_len = 0;
        pass_buf[0] = '\0';
        return;
    }

    /*
     * QMK's EEPROM API expects a pointer.
     *
     * Do NOT use eeprom_address_t here.
     */

    uint16_t address =
        PASS_EEPROM_BASE + (slot * PASS_SLOT_SIZE);

    pass_len = eeprom_read_byte(
        (uint8_t *)address
    );

    /*
     * EEPROM is normally 0xFF when unused.
     * Reject anything larger than our maximum.
     */

    if (pass_len > PASS_MAX_LEN) {
        pass_len = 0;
    }

    if (pass_len > 0) {

        eeprom_read_block(
            pass_buf,
            (const void *)(address + 1),
            pass_len
        );
    }

    pass_buf[pass_len] = '\0';
}


/* =========================================================
 * SAVE PASSWORD TO EEPROM
 * ========================================================= */

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
        PASS_EEPROM_BASE + (slot * PASS_SLOT_SIZE);

    /*
     * Store password length.
     */

    eeprom_update_byte(
        (uint8_t *)address,
        len
    );

    /*
     * Store password bytes.
     */

    if (len > 0) {

        eeprom_update_block(
            new_pass,
            (void *)(address + 1),
            len
        );
    }

    /*
     * Clear the unused portion of the slot.
     *
     * This prevents old characters from remaining in EEPROM
     * if a new password is shorter than the old one.
     */

    if (len < PASS_MAX_LEN) {

        uint8_t zero = 0;

        for (uint8_t i = len; i < PASS_MAX_LEN; i++) {

            eeprom_update_byte(
                (uint8_t *)(address + 1 + i),
                zero
            );
        }
    }
}


/* =========================================================
 * SEND PASSWORD
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
 * RAW HID PASSWORD PROVISIONING
 *
 * Packet from Python:
 *
 *   byte 0 = command
 *   byte 1 = slot
 *   byte 2 = password length
 *   byte 3+ = password
 *
 * Response:
 *
 *   byte 0 = command
 *   byte 1 = slot
 *   byte 2 = 1
 * ========================================================= */

#ifdef RAW_ENABLE

enum raw_hid_commands {
    CMD_SET_PASSWORD = 0x01
};


void raw_hid_receive(
    uint8_t *data,
    uint8_t length
) {

    if (length < 3) {
        return;
    }

    uint8_t cmd = data[0];
    uint8_t slot = data[1];
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

    /*
     * Protect against malformed packets.
     */

    if (payload_len > (length - 3)) {
        payload_len = length - 3;
    }

    pass_save_to_eeprom(
        slot,
        (const char *)&data[3],
        payload_len
    );

    /*
     * Confirmation response.
     */

    uint8_t response[32] = {0};

    response[0] = CMD_SET_PASSWORD;
    response[1] = slot;
    response[2] = 1;

    raw_hid_send(
        response,
        sizeof(response)
    );
}

#endif


/* =========================================================
 * LAYER RGB COLORS
 * ========================================================= */

/*
 * Layer 0 = deep red / maroon
 */

#define LAYER0_HUE     0
#define LAYER0_SAT     220
#define LAYER0_MAXPCT  65

/*
 * Layer 1 = purple
 */

#define LAYER1_HUE     191
#define LAYER1_SAT     200
#define LAYER1_MAXPCT  100

/*
 * Layer 2 = emerald green
 */

#define LAYER2_HUE     100
#define LAYER2_SAT     210
#define LAYER2_MAXPCT  100

/*
 * Layer 3 = cyan
 */

#define LAYER3_HUE     128
#define LAYER3_SAT     255
#define LAYER3_MAXPCT  100


/* =========================================================
 * BREATHING SETTINGS
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
 * VOLUME / TRACK HOLD
 * ========================================================= */

#define VOLUME_HOLD_TIME 400

static uint16_t volume_down_timer = 0;
static uint16_t volume_up_timer = 0;

static bool volume_down_active = false;
static bool volume_up_active = false;

static bool volume_down_held = false;
static bool volume_up_held = false;


/* =========================================================
 * RGB HOUSEKEEPING
 *
 * The layer color is continuously maintained.
 *
 * This intentionally does NOT use RGB breathing mode.
 * We generate the breathing effect ourselves.
 * ========================================================= */

void housekeeping_task_user(void) {

    static uint32_t last_update = 0;

    /*
     * Update approximately every 30 ms.
     */

    if (timer_elapsed32(last_update) < 30) {
        return;
    }

    last_update = timer_read32();

    uint16_t half =
        BREATH_PERIOD_MS / 2;

    uint16_t phase =
        timer_read32() % BREATH_PERIOD_MS;

    uint16_t position;

    if (phase < half) {

        position = phase;

    } else {

        position =
            BREATH_PERIOD_MS - phase;
    }

    uint8_t val =
        BREATH_MIN_VAL +
        (uint8_t)(
            (uint32_t)(
                current_max_val -
                BREATH_MIN_VAL
            ) * position / half
        );

    /*
     * Keep the layer indicator alive.
     */

    rgblight_sethsv_noeeprom(
        current_hue,
        current_sat,
        val
    );
}


/* =========================================================
 * KEYBOARD INITIALIZATION
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
     * Brightness is controlled manually above.
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
 * LAYER RGB
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
 * KEYMAPS
 * ========================================================= */

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {

    /*
     * =====================================================
     * LAYER 0
     *
     * F13 - F21
     * =====================================================
     */

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


    /*
     * =====================================================
     * LAYER 1
     *
     * Top:
     *
     * VOL DOWN | MUTE | VOL UP
     *
     * Middle:
     *
     * BRIGHT DN | PLAY/PAUSE | BRIGHT UP
     *
     * Bottom:
     *
     * PREV LAYER | unused | NEXT LAYER
     * =====================================================
     */

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


    /*
     * =====================================================
     * LAYER 2
     *
     * Seven password slots.
     *
     * PASS1 | PASS2 | PASS3
     * PASS4 | PASS5 | PASS6
     * PREV  | PASS7 | NEXT
     * =====================================================
     */

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


    /*
     * =====================================================
     * LAYER 3
     *
     * RGB controls.
     *
     * NOTE:
     * UG_TOGG is intercepted below so that the layer
     * indicator cannot be permanently disabled by it.
     * =====================================================
     */

    [3] = LAYOUT(

        UG_TOGG,
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
 * PROCESS KEYCODES
 * ========================================================= */

bool process_record_user(
    uint16_t keycode,
    keyrecord_t *record
) {

    switch (keycode) {

        /* =================================================
         * PASSWORD 1
         * ================================================= */

        case PASS1:

            if (record->event.pressed) {
                pass_send_slot(0);
            }

            return false;


        /* =================================================
         * PASSWORD 2
         * ================================================= */

        case PASS2:

            if (record->event.pressed) {
                pass_send_slot(1);
            }

            return false;


        /* =================================================
         * PASSWORD 3
         * ================================================= */

        case PASS3:

            if (record->event.pressed) {
                pass_send_slot(2);
            }

            return false;


        /* =================================================
         * PASSWORD 4
         * ================================================= */

        case PASS4:

            if (record->event.pressed) {
                pass_send_slot(3);
            }

            return false;


        /* =================================================
         * PASSWORD 5
         * ================================================= */

        case PASS5:

            if (record->event.pressed) {
                pass_send_slot(4);
            }

            return false;


        /* =================================================
         * PASSWORD 6
         * ================================================= */

        case PASS6:

            if (record->event.pressed) {
                pass_send_slot(5);
            }

            return false;


        /* =================================================
         * PASSWORD 7
         * ================================================= */

        case PASS7:

            if (record->event.pressed) {
                pass_send_slot(6);
            }

            return false;


        /* =================================================
         * VOLUME DOWN / PREVIOUS TRACK
         *
         * Tap  = Volume Down
         * Hold = Previous Track
         * =================================================
         */

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
         * VOLUME UP / NEXT TRACK
         *
         * Tap  = Volume Up
         * Hold = Next Track
         * =================================================
         */

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
         * PREVIOUS LAYER
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
         * NEXT LAYER
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


        /* =================================================
         * RGB TOGGLE
         *
         * We intentionally do not allow UG_TOGG to shut
         * down the layer indicator because our layer color
         * system continuously controls these same LEDs.
         * =================================================
         */

        case UG_TOGG:

            if (record->event.pressed) {

                /*
                 * Restore the current layer color.
                 */

                rgblight_enable_noeeprom();

                rgblight_sethsv_noeeprom(
                    current_hue,
                    current_sat,
                    current_max_val
                );
            }

            return false;
    }

    return true;
}


/* =========================================================
 * MATRIX SCAN
 *
 * Volume keys:
 *
 * Tap:
 *   Volume Up / Down
 *
 * Hold >= 400 ms:
 *   Next / Previous Track
 * ========================================================= */

void matrix_scan_user(void) {

    /*
     * =====================================================
     * Volume Down -> Previous Track
     * =====================================================
     */

    if (
        volume_down_active &&
        !volume_down_held &&
        timer_elapsed(volume_down_timer)
            >= VOLUME_HOLD_TIME
    ) {

        volume_down_held = true;

        tap_code(KC_MPRV);
    }


    /*
     * =====================================================
     * Volume Up -> Next Track
     * =====================================================
     */

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