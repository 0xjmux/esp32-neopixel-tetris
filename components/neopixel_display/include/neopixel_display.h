#ifndef NEOPIXEL_DISPLAY_H
#define NEOPIXEL_DISPLAY_H

#include <stdint.h>

#include "neopixel.h"
#include "npix_tetris_defs.h"
#include "tetris.h"

#define NUM_TETRIS_COLORS NUM_TETROMINOS + 1

// all display masks use uint8_t, so without a refactor the widest mask that can
//  be used is 8 cells wide
#define DISPLAY_MASK_WIDTH 8
// this defines the leftmost cell for masks. On implementations with larger
// widths,
//  this would need to be changed; for DISPLAY_COLS < 10 0 is ideal.
#define DISPLAY_MASK_LEFTMOST_COL 0

// Lookup table for converting [row][col] of TetrisBoard to LEDs in the matrix.
//  Tables of arbitrary size forcan be generated using `gen_Matrix_LUT.py`
extern const uint8_t rowcol_to_LEDNum_LUT[32][8];

tNeopixelContext init_neopixel_display(void);
void deinit_neopixel_display(tNeopixelContext *neopixels);

void display_board(tNeopixelContext *neopixels, const TetrisBoard *tb);
void clear_display(tNeopixelContext *neopixels);

uint32_t getRGBFromCellColor(int8_t color);

void display_play_again_icon(tNeopixelContext *neopixels);
void display_pause_icon(tNeopixelContext *neopixels);

void printTetrisBoardToLog(TetrisBoard *tb);

void getArrayOfBitsFromMask(const uint8_t in_mask, uint8_t *bits,
                            const uint8_t mask_width);

#endif
