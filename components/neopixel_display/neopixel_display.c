/**
 * Display driver for running tetris on a neopixel matrix
 */

#include "neopixel_display.h"

#include "esp_log.h"  // used for debugging info statements

// local functions
static void display_mask_over_board(tNeopixelContext *neopixels,
                                    const uint8_t top_row,
                                    const uint8_t *mask_2D,
                                    const uint8_t mask_height,
                                    const uint8_t leftmost_col);
inline static tNeopixel tPixelFromCellColor(unsigned int ledNum,
                                            int8_t tetris_cell_color);

static const uint8_t play_again_mask_height;
static const uint8_t play_again_icon_mask[5];

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
      // pixelArr[ledNum] = tPixelFromCellColor(ledNum, tb->board[row][col]);
      pixelArr[ledNum] =
          (tNeopixel){ledNum, getRGBFromCellColor(tb->board[row][col])};
    }
  }
  neopixel_SetPixel(neopixels, pixelArr, PIXEL_COUNT);
}

inline static tNeopixel tPixelFromCellColor(unsigned int ledNum,
                                            int8_t tetris_cell_color) {
  tNeopixel temp = {0};
  temp.index     = ledNum;
  temp.rgb       = getRGBFromCellColor(tetris_cell_color);
  return temp;
}

void display_play_again_icon(tNeopixelContext *neopixels) {
  display_mask_over_board(neopixels, 2, play_again_icon_mask,
                          play_again_mask_height, 0);
}

/**
 * Draw pause icon on display when game is paused
 */
void display_pause_icon(tNeopixelContext *neopixels) {
  const uint8_t pause_icon_starting_height = 3;
  const uint8_t pause_icon_height          = 4;
  tNeopixel Lpixel                         = {0};
  tNeopixel Rpixel                         = {0};

  for (int i = pause_icon_starting_height;
       i < pause_icon_height + pause_icon_starting_height; i++) {
    Lpixel = (tNeopixel){rowcol_to_LEDNum_LUT[i][3], NP_RGB(50, 50, 50)};
    Rpixel = (tNeopixel){rowcol_to_LEDNum_LUT[i][5], NP_RGB(50, 50, 50)};
    neopixel_SetPixel(neopixels, &Lpixel, 1);
    neopixel_SetPixel(neopixels, &Rpixel, 1);
  }
}

/**
 * Overlay symbols represented by bitmask over board contents (eg pause symbol)
 *
 * @param *neopixels tNeopixelContext   - neopixel context (display pixels)
 * @param top_row uint8_t               - row to start drawing at
 * @param mask_2D uint8_t                  - mask to overlay. A ptr to an array
 * of 8 bit uints, which are used bitwise as the mask
 * @param mask_height uint8_t           - number of rows in mask
 * @param leftmost_col uint8_t          - leftmost column of mask (for displays
 * wider than 8 bits, otherwise just 0)
 */
static void display_mask_over_board(tNeopixelContext *neopixels,
                                    const uint8_t top_row,
                                    const uint8_t *mask_2D,
                                    const uint8_t mask_height,
                                    const uint8_t leftmost_col) {
  uint8_t curr_row = top_row;
  uint8_t bits[DISPLAY_MASK_WIDTH];

  // largest possible pixelArr needed
  tNeopixel pixelArr[DISPLAY_COLS * mask_height];
  uint16_t num_bits_set = 0;

  assert(mask_height < DISPLAY_ROWS);
  // for each row in mask
  // for(int row = top_row; row < top_row + mask_height; row++) {
  for (int i = 0; i < mask_height; i++) {
    curr_row = top_row + i;

    // get bitmask for this row
    getArrayOfBitsFromMask(mask_2D[i], bits, DISPLAY_MASK_WIDTH);
    // for each bit in mask
    for (int col = leftmost_col; col < leftmost_col + DISPLAY_MASK_WIDTH;
         col++) {
      // if bit is present, add to pixelArr
      if (bits[col]) {
        // set current end of pixelArr accordingly
        pixelArr[num_bits_set] = (tNeopixel){
            rowcol_to_LEDNum_LUT[curr_row][col], NP_RGB(100, 100, 100)};
        num_bits_set++;
      }
    }
  }

  neopixel_SetPixel(neopixels, pixelArr, num_bits_set);
}

/**
 * Converts bitwise contents of 1D `mask` to an array *bits
 * @param in_mask const uint8_t - single uint8_t holding 8 columns
 * @param *bits - bit array result stored in
 * @param mask_width - width of mask row
 *
 */
inline void getArrayOfBitsFromMask(const uint8_t in_mask, uint8_t *bits,
                                   const uint8_t mask_width) {
  uint8_t mask = in_mask;

  for (int8_t bit = mask_width - 1; bit >= 0; bit--) {
    bits[bit] = mask & 1;
    mask >>= 1;
  }
}

/**
 * Match tetris's `piece_colors` enum to 32bit neopixel color values
 */
inline uint32_t getRGBFromCellColor(int8_t color) {
  switch (color) {
    case S_CELL_COLOR:
      return NP_RGB(0, 50, 0);  // Green
    case Z_CELL_COLOR:
      return NP_RGB(50, 0, 0);  // Red
    case T_CELL_COLOR:
      return NP_RGB(50, 0, 50);  // Magenta
    case L_CELL_COLOR:
      return NP_RGB(50, 25, 0);  // Orange
    case J_CELL_COLOR:
      return NP_RGB(0, 0, 50);  // Blue
    case SQ_CELL_COLOR:
      return NP_RGB(50, 50, 0);  // Yellow
    case I_CELL_COLOR:
      return NP_RGB(0, 50, 50);  // light blue
    case BG_COLOR:
      return NP_RGB(0, 0, 0);  // background - off
    default:
      assert(0 && "default case of getRGB should never be reached!");
  }
}

// these LUTs and bitmasks are only valid for 8bit wide matrices.
//      Matrices of other sizes would require different constants.
#if DISPLAY_COLS == 8

/**
 * Convert [row][col] to LED number in 32x8 matrix
 */
const uint8_t rowcol_to_LEDNum_LUT[32][8] = {
    {7, 6, 5, 4, 3, 2, 1, 0},                  // row 0
    {8, 9, 10, 11, 12, 13, 14, 15},            // row 1
    {23, 22, 21, 20, 19, 18, 17, 16},          // row 2
    {24, 25, 26, 27, 28, 29, 30, 31},          // row 3
    {39, 38, 37, 36, 35, 34, 33, 32},          // row 4
    {40, 41, 42, 43, 44, 45, 46, 47},          // row 5
    {55, 54, 53, 52, 51, 50, 49, 48},          // row 6
    {56, 57, 58, 59, 60, 61, 62, 63},          // row 7
    {71, 70, 69, 68, 67, 66, 65, 64},          // row 8
    {72, 73, 74, 75, 76, 77, 78, 79},          // row 9
    {87, 86, 85, 84, 83, 82, 81, 80},          // row 10
    {88, 89, 90, 91, 92, 93, 94, 95},          // row 11
    {103, 102, 101, 100, 99, 98, 97, 96},      // row 12
    {104, 105, 106, 107, 108, 109, 110, 111},  // row 13
    {119, 118, 117, 116, 115, 114, 113, 112},  // row 14
    {120, 121, 122, 123, 124, 125, 126, 127},  // row 15
    {135, 134, 133, 132, 131, 130, 129, 128},  // row 16
    {136, 137, 138, 139, 140, 141, 142, 143},  // row 17
    {151, 150, 149, 148, 147, 146, 145, 144},  // row 18
    {152, 153, 154, 155, 156, 157, 158, 159},  // row 19
    {167, 166, 165, 164, 163, 162, 161, 160},  // row 20
    {168, 169, 170, 171, 172, 173, 174, 175},  // row 21
    {183, 182, 181, 180, 179, 178, 177, 176},  // row 22
    {184, 185, 186, 187, 188, 189, 190, 191},  // row 23
    {199, 198, 197, 196, 195, 194, 193, 192},  // row 24
    {200, 201, 202, 203, 204, 205, 206, 207},  // row 25
    {215, 214, 213, 212, 211, 210, 209, 208},  // row 26
    {216, 217, 218, 219, 220, 221, 222, 223},  // row 27
    {231, 230, 229, 228, 227, 226, 225, 224},  // row 28
    {232, 233, 234, 235, 236, 237, 238, 239},  // row 29
    {247, 246, 245, 244, 243, 242, 241, 240},  // row 30
    {248, 249, 250, 251, 252, 253, 254, 255},  // row 31
};

static const uint8_t play_again_mask_height  = 5;
static const uint8_t play_again_icon_mask[5] = {
    0b01000010, 0b01100101, 0b01110001, 0b01100010, 0b01000010};
#endif

/**
 * Print board state to ESP_LOGI
 */
void printTetrisBoardToLog(TetrisBoard *tb) {
  // draw existing pieces on board
  printf("Highest occupied cell: %d\n", tb->highest_occupied_cell);

  // print col numbers
  printf("  ");
  for (int i = 0; i < TETRIS_COLS; i++) printf("%-2d  ", i);
  printf("\n");

  // print separator
  printf("   ");
  for (int i = 0; i < TETRIS_COLS; i++) printf("----");
  printf("----\n");

  // print board itself
  for (int i = 0; i < TETRIS_ROWS; i++) {
    // print row number
    printf("%-3d| ", i);
    for (int j = 0; j < TETRIS_COLS; j++) {
      if (tb->board[i][j] >= 0) {
        printf("%-3d ", tb->board[i][j]);
      } else {
        printf("    ");
      }
    }
    printf("|\n");
  }
}
