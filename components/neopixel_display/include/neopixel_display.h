#ifndef NEOPIXEL_DISPLAY_H
#define NEOPIXEL_DISPLAY_H

#include <stdint.h>

#include "npix_tetris_defs.h"
#include "tetris.h"
#include "neopixel.h"


#define NUM_TETRIS_COLORS NUM_TETROMINOS + 1


/**
 * Panel information:
 * for my 8x32 panel, LED #0 is at the top right of the display, and then
 * it winds back and forth horizontally all the way down. 
 * LED 255 is at the bottom right of the display
*/

// LED numbers
#define TOP_RIGHT_LED 0
#define BOT_RIGHT_LED 255
#define TOP_LEFT_LED 7
#define BOT_LEFT_LED 248


#define ARRAY_SIZE(x) (sizeof(x)/sizeof(x[0]))

// extern const uint8_t rowcol_to_LEDNum_LUT[34][8];

tNeopixelContext init_neopixel_display(void);
void deinit_neopixel_display(tNeopixelContext *neopixels);

void clear_display(tNeopixelContext *neopixels);

uint32_t getRGBFromCellColor(int8_t color);

void display_board(tNeopixelContext *neopixels, const TetrisBoard *tb);

#endif