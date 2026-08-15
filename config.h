#pragma once

#include "config_common.h"

/* USB Device descriptor parameter */
#define VENDOR_ID    0xFEED
#define PRODUCT_ID   0x0000
#define DEVICE_VER   0x0100
#define MANUFACTURER "Lakshya Rao"
#define PRODUCT      "macropad"

/* key matrix size */
#define MATRIX_ROWS 3
#define MATRIX_COLS 3

/* Pin assignments */
#define MATRIX_ROW_PINS { 27, 14, 6 }
#define MATRIX_COL_PINS { 15, 9, 5, 26, 8, 4 }

/* COL2ROW or ROW2COL */
#define DIODE_DIRECTION COL2ROW

/* Debounce reduces chatter - set 0 if you want to disable it */
#define DEBOUNCE 5

/* RGB Lighting Configuration */
#define RGB_DI_PIN 13
#define RGBLED_NUM 3
#define RGBLIGHT_EFFECT_BREATHING
#define RGBLIGHT_EFFECT_RAINBOW_MOOD
#define RGBLIGHT_EFFECT_RAINBOW_SWIRL
#define RGBLIGHT_EFFECT_SNAKE
#define RGBLIGHT_EFFECT_KNIGHT
#define RGBLIGHT_EFFECT_CHRISTMAS
#define RGBLIGHT_EFFECT_STATIC_GRADIENT
#define RGBLIGHT_EFFECT_RGB_TEST
#define RGBLIGHT_EFFECT_ALTERNATING
#define RGBLIGHT_EFFECT_TWINKLE

/* Tapping term for layer tap */
#define TAPPING_TERM 200
