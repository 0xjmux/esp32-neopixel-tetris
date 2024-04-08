#ifndef NEOPIXEL_DISPLAY_H
#define NEOPIXEL_DISPLAY_H

#include <stdint.h>

#include "defs.h"
#include "tetris.h"
#include "neopixel.h"

#define NUM_TETRIS_COLORS NUM_TETROMINOS + 1

/**
 * match tetris's `piece_colors` enum to a 32bit neopixel color value
 * @param piece_colors piece_color
 * @param uint32_t neopix_rgb
*/
typedef struct neopixel_tetromino_color_pair {
    enum piece_colors piece_color;
    uint32_t neopix_rgb;
} neopixel_tetromino_color_pair;

extern const neopixel_tetromino_color_pair color_pairs[NUM_TETRIS_COLORS];


#endif