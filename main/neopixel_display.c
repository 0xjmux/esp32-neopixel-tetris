/**
 * Display driver for running tetris on a neopixel matrix
*/


#include "neopixel_display.h"

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

/**
 * for a given row,col compute the position of that LED on the LED matrix
*/
inline uint16_t led_number_from_tetris_loc(uint8_t row, uint8_t col) {

}

void tetrisBoard_to_neopixel_matrix(TetrisBoard *tb) {

}

// void init_neopixel_display_colors

void neopixel_display_setup(void) {
    tNeopixelContext neopixels = neopixel_Init(PIXEL_COUNT, NEOPIXEL_PIN);

}



/**
 * Match tetris's `piece_colors` enum to 32bit neopixel color values
*/
const neopixel_tetromino_color_pair color_pairs[NUM_TETRIS_COLORS] = {
    // these colors are best guesses atm
    {S_CELL_COLOR, NP_RGB(0, 50, 0)},        // Green
    {Z_CELL_COLOR, NP_RGB(50, 0, 0)},        // Red
    {T_CELL_COLOR, NP_RGB(50, 0, 50)},        // Magenta
    {L_CELL_COLOR, NP_RGB(50, 25, 0)},        // Orange
    {J_CELL_COLOR, NP_RGB(0, 0, 50)},        // Blue
    {SQ_CELL_COLOR, NP_RGB(50, 50, 0)},       // Yellow
    {I_CELL_COLOR, NP_RGB(0, 50, 50)},        // light blue
    {BG_COLOR, NP_RGB(0, 0, 0)}             // background - off
};