#ifndef DEFS_H
#define DEFS_H
/**
 * Base definitions for ESP32 Neopixel Tetris
 *
 */

#define TAG "esp32-neopixel-tetris"

#define STAT_LED_PIN 2
#define NEOPIXEL_PIN 21

#define DISPLAY_ROWS 32
#define DISPLAY_COLS 8

#define PIXEL_COUNT DISPLAY_ROWS* DISPLAY_COLS

// if this value is too low, FreeRTOS stack overflow protection will detect
// corruption
#define TASK_STACK_DEPTH_BYTES 4096

#endif
