/**
 * Display driver for running tetris on a neopixel matrix
 */

#include "neopixel_display.h"

#include "esp_log.h" // used for debugging info statements

inline static tNeopixel tPixelFromCellColor(unsigned int ledNum,
                                            int8_t tetris_cell_color);

/**
 * Initialize and clear neopixel display
 * @returns tNeopixelContext of display
 */
tNeopixelContext init_neopixel_display(void) {
  tNeopixelContext neopixels = neopixel_Init(PIXEL_COUNT, NEOPIXEL_PIN);
  clear_display(neopixels);
  ESP_LOGI(TAG, "initialized and cleared neopixel display");
  return neopixels;
}

void deinit_neopixel_display(tNeopixelContext *neopixels) {
  neopixel_Deinit(neopixels);
}

/**
 * Clear neopixel display
 * @param neopixels - tNeopixelContext of display
 */
void clear_display(tNeopixelContext *neopixels) {
  tNeopixel pixelArr[PIXEL_COUNT];

  for (int i = 0; i < PIXEL_COUNT; i++) {
    pixelArr[i] = (tNeopixel){i, 0};
  }
  neopixel_SetPixel(neopixels, pixelArr, PIXEL_COUNT);
  // so, it seems like if I try to address the same pixel more than once in the
  // pixelArr
  //  a lock_acquire_generic will result in an panic_abort() being called by
  //  freertos
}

void display_board(tNeopixelContext *neopixels, const TetrisBoard *tb) {
  // sanity check to make sure display is right size for board
  assert(TETRIS_COLS == DISPLAY_COLS && TETRIS_ROWS == DISPLAY_ROWS);
  assert(neopixels != NULL);
  ESP_LOGD(TAG, "Displaying board\n");

  tNeopixel pixelArr[PIXEL_COUNT] = {0};

  for (int row = 0; row < DISPLAY_ROWS; row++) {
    for (int col = 0; col < DISPLAY_COLS; col++) {
      int ledNum = rowcol_to_LEDNum_LUT[row][col];
      assert(ledNum < PIXEL_COUNT && "LED number out of bounds");
      pixelArr[ledNum] = tPixelFromCellColor(ledNum, tb->board[row][col]);
      // pixelArr[ledNum] = (tNeopixel){ledNum,
      // getRGBFromCellColor(tb->board[row][col])};
    }
  }
  neopixel_SetPixel(neopixels, pixelArr, PIXEL_COUNT);
}

inline static tNeopixel tPixelFromCellColor(unsigned int ledNum,
                                            int8_t tetris_cell_color) {
  tNeopixel temp = {0};
  temp.index = ledNum;
  temp.rgb = getRGBFromCellColor(tetris_cell_color);
  return temp;
}

/**
 * Match tetris's `piece_colors` enum to 32bit neopixel color values
 */
inline uint32_t getRGBFromCellColor(int8_t color) {
  switch (color) {
  case S_CELL_COLOR:
    return NP_RGB(0, 50, 0); // Green
  case Z_CELL_COLOR:
    return NP_RGB(50, 0, 0); // Red
  case T_CELL_COLOR:
    return NP_RGB(50, 0, 50); // Magenta
  case L_CELL_COLOR:
    return NP_RGB(50, 25, 0); // Orange
  case J_CELL_COLOR:
    return NP_RGB(0, 0, 50); // Blue
  case SQ_CELL_COLOR:
    return NP_RGB(50, 50, 0); // Yellow
  case I_CELL_COLOR:
    return NP_RGB(0, 50, 50); // light blue
  case BG_COLOR:
    return NP_RGB(0, 0, 0); // background - off
  default:
    assert(0 && "default case of getRGB should never be reached!");
  }
}

/**
 * Convert [row][col] to LED number in 32x8 matrix
 */
const uint8_t rowcol_to_LEDNum_LUT[32][8] = {
    {0, 1, 2, 3, 4, 5, 6, 7},                 // row 0
    {15, 14, 13, 12, 11, 10, 9, 8},           // row 1
    {16, 17, 18, 19, 20, 21, 22, 23},         // row 2
    {31, 30, 29, 28, 27, 26, 25, 24},         // row 3
    {32, 33, 34, 35, 36, 37, 38, 39},         // row 4
    {47, 46, 45, 44, 43, 42, 41, 40},         // row 5
    {48, 49, 50, 51, 52, 53, 54, 55},         // row 6
    {63, 62, 61, 60, 59, 58, 57, 56},         // row 7
    {64, 65, 66, 67, 68, 69, 70, 71},         // row 8
    {79, 78, 77, 76, 75, 74, 73, 72},         // row 9
    {80, 81, 82, 83, 84, 85, 86, 87},         // row 10
    {95, 94, 93, 92, 91, 90, 89, 88},         // row 11
    {96, 97, 98, 99, 100, 101, 102, 103},     // row 12
    {111, 110, 109, 108, 107, 106, 105, 104}, // row 13
    {112, 113, 114, 115, 116, 117, 118, 119}, // row 14
    {127, 126, 125, 124, 123, 122, 121, 120}, // row 15
    {128, 129, 130, 131, 132, 133, 134, 135}, // row 16
    {143, 142, 141, 140, 139, 138, 137, 136}, // row 17
    {144, 145, 146, 147, 148, 149, 150, 151}, // row 18
    {159, 158, 157, 156, 155, 154, 153, 152}, // row 19
    {160, 161, 162, 163, 164, 165, 166, 167}, // row 20
    {175, 174, 173, 172, 171, 170, 169, 168}, // row 21
    {176, 177, 178, 179, 180, 181, 182, 183}, // row 22
    {191, 190, 189, 188, 187, 186, 185, 184}, // row 23
    {192, 193, 194, 195, 196, 197, 198, 199}, // row 24
    {207, 206, 205, 204, 203, 202, 201, 200}, // row 25
    {208, 209, 210, 211, 212, 213, 214, 215}, // row 26
    {223, 222, 221, 220, 219, 218, 217, 216}, // row 27
    {224, 225, 226, 227, 228, 229, 230, 231}, // row 28
    {239, 238, 237, 236, 235, 234, 233, 232}, // row 29
    {240, 241, 242, 243, 244, 245, 246, 247}, // row 30
    {255, 254, 253, 252, 251, 250, 249, 248}, // row 31
};
