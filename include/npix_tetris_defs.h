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

#define PIXEL_COUNT DISPLAY_ROWS*DISPLAY_COLS



////////////////////////////////////////
// TEMPORARY DEFS
// real impl needs to be able to find and accept any local remote, not just mine
#define MY_REMOTE_MAC 0x444f8ebf15

#define TASK_STACK_DEPTH_BYTES 4096




#endif