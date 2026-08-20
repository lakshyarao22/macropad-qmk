#include QMK_KEYBOARD_H
#include <eeprom.h>
#include "raw_hid.h"
#include <stdint.h>
#include <string.h>

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

    VOL_DOWN_TRACK,
    VOL_UP_TRACK,

    LAYER_PREV,
    LAYER_NEXT,
    LAYER_BASE
};


/* =========================================================
 * Password storage
 *
 * Six password slots.
 *
 * Each slot:
 *
 *   byte 0      = password length
 *   bytes 1-31  = password
 *
 * Slot size = 32 bytes.
 *
 * EEPROM:
 *
 *   Slot 1 -> 100 - 131
 *   Slot 2 -> 132 - 163
 *   Slot 3 -> 164 - 195
 *   Slot 4 -> 196 - 227
 *   Slot 5 -> 228 - 259
 *   Slot 6 -> 260 - 291
 *
 * EEPROM is NOT encrypted.
 * ========================================================= */

#define PASS_SLOT_COUNT 6
#define PASS_MAX_LEN    31

#define PASS_EEPROM_BASE 100
#define PASS_SLOT_SIZE   32


static char pass_buf[PASS_MAX_LEN + 1];
static uint8_t pass_len = 0;


/* =========================================================
 * EEPROM address helper
 *
 * Current QMK EEPROM API expects a pointer.
 *
 * We store the logical EEPROM offset as a pointer-sized
 * value so RP2040 builds do not produce:
 *
 *   cast to pointer from integer of different size
 * ========================================================= */

static uint8_t *pass_eeprom_address(uint8_t slot) {
    return (uint8_t *)(uintptr_t)(
        PASS_EEPROM_BASE +
        ((uint16_t)slot * PASS_SLOT_SIZE)
    );
}


/* =========================================================
 * Load password from EEPROM
 * ========================================================= */

static void pass_load_from_eeprom(uint8_t slot) {

    if (slot >= PASS_SLOT_COUNT) {
        pass_len = 0;
        pass_buf[0] = '\0';
        return;
    }

    uint8_t *address =
        pass_eeprom_address(slot);


    pass_len =
        eeprom_read_byte(address);


    /*
     * Protect against invalid EEPROM contents.
     */

    if (pass_len > PASS_MAX_LEN) {
        pass_len = 0;
    }


    /*
     * Load password bytes.
     */

    if (pass_len > 0) {

        eeprom_read_block(
            pass_buf,
            address + 1,
            pass_len
        );
    }


    /*
     * Always terminate the string.
     */

    pass_buf[pass_len] = '\0';
}


/* =========================================================
 * Save password to EEPROM
 * ========================================================= */

static void pass_save_to_eeprom(
    uint8_t slot,
    const char *password,
    uint8_t length
) {

    if (slot >= PASS_SLOT_COUNT) {
        return;
    }


    /*
     * Never exceed our 31-character password limit.
     */

    if (length > PASS_MAX_LEN) {
        length = PASS_MAX_LEN;
    }


    uint8_t *address =
        pass_eeprom_address(slot);


    /*
     * Store password length.
     */

    eeprom_update_byte(
        address,
        length
    );


    /*
     * Store password itself.
     */

    if (length > 0) {

        eeprom_update_block(
            password,
            address + 1,
            length
        );
    }


    /*
     * Update our RAM buffer as well.
     */

    memcpy(
        pass_buf,
        password,
        length
    );

    pass_buf[length] = '\0';

    pass_len = length;
}


/* =========================================================
 * Send password from EEPROM slot
 * ========================================================= */

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


/* =========================================================
 * Raw HID password provisioning
 *
 * Packet received from Python:
 *
 *   data[0] = command
 *   data[1] = slot
 *   data[2] = password length
 *   data[3...] = password
 *
 * Response:
 *
 *   response[0] = command
 *   response[1] = slot
 *   response[2] = 1
 * ========================================================= */

#ifdef RAW_ENABLE

enum raw_hid_commands {
    CMD_SET_PASSWORD = 0x01,
};


void raw_hid_receive(
    uint8_t *data,
    uint8_t length
) {

    /*
     * Need at least:
     *
     * command + slot + length
     */

    if (length < 3) {
        return;
    }


    uint8_t cmd =
        data[0];

    uint8_t slot =
        data[1];

    uint8_t payload_len =
        data[2];


    /*
     * Check command.
     */

    if (cmd != CMD_SET_PASSWORD) {
        return;
    }


    /*
     * Check slot.
     */

    if (slot >= PASS_SLOT_COUNT) {
        return;
    }


    /*
     * Never allow more than our EEPROM slot
     * can hold.
     */

    if (payload_len > PASS_MAX_LEN) {
        payload_len = PASS_MAX_LEN;
    }


    /*
     * Never read beyond the received HID packet.
     */

    if (payload_len > (length - 3)) {
        payload_len = length - 3;
    }


    /*
     * Save password.
     */

    pass_save_to_eeprom(
        slot,
        (const char *)&data[3],
        payload_len
    );


    /*
     * Send acknowledgement.
     *
     * QMK Raw HID packets are 32 bytes.
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
 * Layer colors
 * ========================================================= */

/*
 * Layer 0 = Cyberpunk Cyan
 * Layer 1 = Vaporwave Magenta
 * Layer 2 = Amber Gold
 * Layer 3 = Crimson Red
 * Layer 4 = Acid Green
 * Layer 5 = Deep Violet
 * Layer 6 = Warm White
 * Layer 7 = Ice Blue
 *
 * Maximum brightness = 100%.
 *
 * Breathing minimum = 40%.
 */

#define LAYER0_R       0
#define LAYER0_G       255
#define LAYER0_B       200

#define LAYER1_R       255
#define LAYER1_G       0
#define LAYER1_B       150

#define LAYER2_R       255
#define LAYER2_G       110
#define LAYER2_B       0

#define LAYER3_R       255
#define LAYER3_G       0
#define LAYER3_B       20

#define LAYER4_R       50
#define LAYER4_G       255
#define LAYER4_B       20

#define LAYER5_R       120
#define LAYER5_G       0
#define LAYER5_B       255

#define LAYER6_R       255
#define LAYER6_G       180
#define LAYER6_B       100

#define LAYER7_R       40
#define LAYER7_G       180
#define LAYER7_B       255


/* =========================================================
 * Breathing RGB effect
 * ========================================================= */

/*
 * Full breathing cycle:
 *
 *   40% -> 100% -> 40%
 *
 * 10 seconds total.
 */

#define BREATH_PERIOD_MS    10000
#define BREATH_MIN_PERCENT  40

#define BREATH_MIN_VAL \
    ((BREATH_MIN_PERCENT * 255) / 100)

#define PCT_TO_VAL(pct) \
    ((pct) * 255 / 100)


static uint8_t current_red =
    LAYER0_R;

static uint8_t current_green =
    LAYER0_G;

static uint8_t current_blue =
    LAYER0_B;

static uint8_t current_max_val =
    255;


/* =========================================================
 * Volume / track hold handling
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
 * ========================================================= */

void housekeeping_task_user(void) {

    static uint32_t last_update = 0;


    /*
     * Update RGB every 20 ms.
     */

    if (timer_elapsed32(last_update) < 20) {
        return;
    }


    last_update = timer_read32();


    uint16_t half =
        BREATH_PERIOD_MS / 2;


    uint16_t phase =
        timer_read32() % BREATH_PERIOD_MS;


    /*
     * Triangle-wave breathing:
     *
     * 40% -> 100% -> 40%
     */

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


    rgblight_setrgb(
        ((uint16_t)current_red * val) / 255,
        ((uint16_t)current_green * val) / 255,
        ((uint16_t)current_blue * val) / 255
    );
}


/* =========================================================
 * Keyboard initialization
 * ========================================================= */

void keyboard_post_init_user(void) {

    /*
     * Load slot 0 initially.
     * Individual slots are loaded when their
     * corresponding key is pressed.
     */

    pass_load_from_eeprom(0);


    /*
     * Start RGB lighting.
     */

    rgblight_enable_noeeprom();


    rgblight_mode_noeeprom(
        RGBLIGHT_MODE_STATIC_LIGHT
    );


    rgblight_setrgb(
        current_red,
        current_green,
        current_blue
    );
}


/* =========================================================
 * Layer RGB
 * ========================================================= */

layer_state_t layer_state_set_user(
    layer_state_t state
) {

    switch (get_highest_layer(state)) {

        /* =================================================
         * Layer 0 - Cyan
         * ================================================= */

        case 0:

            current_red =
                LAYER0_R;

            current_green =
                LAYER0_G;

            current_blue =
                LAYER0_B;

            current_max_val = 255;

            break;


        /* =================================================
         * Layer 1 - Blue
         * ================================================= */

        case 1:

            current_red =
                LAYER1_R;

            current_green =
                LAYER1_G;

            current_blue =
                LAYER1_B;

            current_max_val = 255;

            break;


        /* =================================================
         * Layer 2 - Green
         * ================================================= */

        case 2:

            current_red =
                LAYER2_R;

            current_green =
                LAYER2_G;

            current_blue =
                LAYER2_B;

            current_max_val = 255;

            break;


        case 3:

            current_red = LAYER3_R;
            current_green = LAYER3_G;
            current_blue = LAYER3_B;
            current_max_val = 255;
            break;


        case 4:

            current_red = LAYER4_R;
            current_green = LAYER4_G;
            current_blue = LAYER4_B;
            current_max_val = 255;
            break;


        case 5:

            current_red = LAYER5_R;
            current_green = LAYER5_G;
            current_blue = LAYER5_B;
            current_max_val = 255;
            break;


        case 6:

            current_red = LAYER6_R;
            current_green = LAYER6_G;
            current_blue = LAYER6_B;
            current_max_val = 255;
            break;


        case 7:

            current_red = LAYER7_R;
            current_green = LAYER7_G;
            current_blue = LAYER7_B;
            current_max_val = 255;
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
     *
      * Meh 1 - Meh 6
     *
    * Color: Cyberpunk Cyan
     * ===================================================== */

    [0] = LAYOUT(

        MEH(KC_1),
        MEH(KC_2),
        MEH(KC_3),

        MEH(KC_4),
        MEH(KC_5),
        MEH(KC_6),

        LAYER_PREV,
        LAYER_BASE,
        LAYER_NEXT
    ),


    /* =====================================================
     * Layer 1
     *
     * Volume / Media / Brightness
     *
     * VOL DOWN:
     *   tap  = Volume Down
     *   hold = Previous Track
     *
     * VOL UP:
     *   tap  = Volume Up
     *   hold = Next Track
     *
        * Color: Vaporwave Magenta
     * ===================================================== */

    [1] = LAYOUT(

        VOL_DOWN_TRACK,
        KC_MUTE,
        VOL_UP_TRACK,

        KC_BRID,
        KC_MPLY,
        KC_BRIU,

        LAYER_PREV,
        LAYER_BASE,
        LAYER_NEXT
    ),


    /* =====================================================
     * Layer 2
     *
     * F13 - F18
     *
     * Color: Amber Gold
     * ===================================================== */

    [2] = LAYOUT(

        KC_F13,
        KC_F14,
        KC_F15,

        KC_F16,
        KC_F17,
        KC_F18,

        LAYER_PREV,
        LAYER_BASE,
        LAYER_NEXT
    ),


    /* =====================================================
     * Layer 3
     *
     * Six password slots
     *
    * Color: Crimson Red
     * ===================================================== */

    [3] = LAYOUT(

        PASS1,
        PASS2,
        PASS3,

        PASS4,
        PASS5,
        PASS6,

        LAYER_PREV,
        LAYER_BASE,
        LAYER_NEXT
    ),


    /* =====================================================
     * Layer 4 - Acid Green
     * ===================================================== */

    [4] = LAYOUT(

        KC_NO,
        KC_NO,
        KC_NO,

        KC_NO,
        KC_NO,
        KC_NO,

        LAYER_PREV,
        LAYER_BASE,
        LAYER_NEXT
    ),


    /* =====================================================
     * Layer 5 - Deep Violet
     * ===================================================== */

    [5] = LAYOUT(

        KC_NO,
        KC_NO,
        KC_NO,

        KC_NO,
        KC_NO,
        KC_NO,

        LAYER_PREV,
        LAYER_BASE,
        LAYER_NEXT
    ),


    /* =====================================================
     * Layer 6 - Warm White
     * ===================================================== */

    [6] = LAYOUT(

        KC_NO,
        KC_NO,
        KC_NO,

        KC_NO,
        KC_NO,
        KC_NO,

        LAYER_PREV,
        LAYER_BASE,
        LAYER_NEXT
    ),


    /* =====================================================
    * Layer 7 - Ice Blue
     * ===================================================== */

    [7] = LAYOUT(

        KC_NO,
        KC_NO,
        KC_NO,

        KC_NO,
        KC_NO,
        KC_NO,

        LAYER_PREV,
        LAYER_BASE,
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
         * Base layer
         * ================================================= */

        case LAYER_BASE:

            if (record->event.pressed) {
                layer_move(0);
            }

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
         *
         * 0 -> 7
         * 1 -> 0
         * 2 -> 1
         * ...
         * 7 -> 6
         * ================================================= */

        case LAYER_PREV:

            if (record->event.pressed) {

                uint8_t current_layer =
                    get_highest_layer(layer_state);


                if (current_layer == 0) {

                    layer_move(7);

                } else {

                    layer_move(
                        current_layer - 1
                    );
                }
            }

            return false;


        /* =================================================
         * Next Layer
         *
         * 0 -> 1
         * 1 -> 2
         * ...
         * 6 -> 7
         * 7 -> 0
         * ================================================= */

        case LAYER_NEXT:

            if (record->event.pressed) {

                uint8_t current_layer =
                    get_highest_layer(layer_state);


                if (current_layer >= 7) {

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
 * Detects long presses on the volume keys.
 *
 * Volume Down:
 *   hold 400 ms -> Previous Track
 *
 * Volume Up:
 *   hold 400 ms -> Next Track
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