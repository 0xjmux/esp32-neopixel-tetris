#include <string.h>

#define TEST_DISPLAY 1

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"  // for vTaskDelay
#include "neopixel.h"
#include "neopixel_display.h"
#include "npix_tetris_defs.h"
#include "unity.h"

/**
 * Panel information:
 * for my 8x32 panel, LED #0 is at the top right of the display, and then
 * it winds back and forth horizontally all the way down.
 * LED 255 is at the bottom right of the display
 */
#define TOP_RIGHT_LED 0
#define BOT_RIGHT_LED 255
#define TOP_LEFT_LED  7
#define BOT_LEFT_LED  248

static const int8_t example_board[32][8];
static void setRingFromOutsideToColor(TetrisBoard *tb,
                                      uint8_t rings_from_outside, int8_t color);
void printArrayContents(const uint8_t *array, const uint8_t length);

// allow iterating through cell colors
const int8_t all_cell_colors[NUM_TETRIS_COLORS] = {
    S_CELL_COLOR, Z_CELL_COLOR,  T_CELL_COLOR, L_CELL_COLOR,
    J_CELL_COLOR, SQ_CELL_COLOR, I_CELL_COLOR, BG_COLOR};
// Green        Red         Magenta         Orange          Blue        Yellow
// light blue      Off

tNeopixelContext neopixels;

// set stuff up here
void setUp(void) { neopixels = neopixel_Init(PIXEL_COUNT, NEOPIXEL_PIN); }

void tearDown(void) { neopixel_Deinit(neopixels); }

////////////////////////////////////////
// internal tests
////////////////////////////////////////
TEST_CASE("test getRGBFromCellColor", "[display]") {
  // for each cell color
  TEST_ASSERT_EQUAL(NP_RGB(0, 50, 0), getRGBFromCellColor(S_CELL_COLOR));
  TEST_ASSERT_EQUAL(NP_RGB(50, 0, 0), getRGBFromCellColor(Z_CELL_COLOR));
  TEST_ASSERT_EQUAL(NP_RGB(50, 0, 50), getRGBFromCellColor(T_CELL_COLOR));
  TEST_ASSERT_EQUAL(NP_RGB(50, 25, 0), getRGBFromCellColor(L_CELL_COLOR));
  TEST_ASSERT_EQUAL(NP_RGB(0, 0, 50), getRGBFromCellColor(J_CELL_COLOR));
  TEST_ASSERT_EQUAL(NP_RGB(50, 50, 0), getRGBFromCellColor(SQ_CELL_COLOR));
  TEST_ASSERT_EQUAL(NP_RGB(0, 50, 50), getRGBFromCellColor(I_CELL_COLOR));
  TEST_ASSERT_EQUAL(NP_RGB(0, 0, 0), getRGBFromCellColor(BG_COLOR));
}

TEST_CASE("Test getArrayOfBitsFromMask", "[internal]") {
#define MASK_WIDTH 8

  uint8_t in_mask = 0b01110001;
  uint8_t result_bits[MASK_WIDTH];
  uint8_t expected_bits[MASK_WIDTH] = {0, 1, 1, 1, 0, 0, 0, 1};
  getArrayOfBitsFromMask(in_mask, result_bits, MASK_WIDTH);

  printf("getArrayofBitsFromMask: Input (expected) was ");
  printArrayContents(expected_bits, MASK_WIDTH);
  printf(", output was: ");
  printArrayContents(result_bits, MASK_WIDTH);
  printf("\n");

  TEST_ASSERT_EQUAL_UINT8_ARRAY_MESSAGE(
      expected_bits, result_bits, MASK_WIDTH,
      "getArrayOfBitsFromMask returned incorrect result");
}

////////////////////////////////////////
/// tests requiring looking at the display
////////////////////////////////////////

/**
 * Display red in top right corner, blue in bot left
 * This will be manually checked by a human
 */
TEST_CASE("test matrix orientation", "[display]") {
  uint32_t refreshRate = neopixel_GetRefreshRate(neopixels);
  // uint32_t taskDelay = MAX(1, pdMS_TO_TICKS(1000UL / refreshRate));
  printf("refreshrate=%ld\n", refreshRate);

  // set topR to red
  tNeopixel topRightNeopix = {TOP_RIGHT_LED, getRGBFromCellColor(Z_CELL_COLOR)};
  neopixel_SetPixel(neopixels, &topRightNeopix, 1);

  // set botL to blue
  tNeopixel botLeftNeopix = {BOT_LEFT_LED, getRGBFromCellColor(J_CELL_COLOR)};
  neopixel_SetPixel(neopixels, &botLeftNeopix, 1);

  vTaskDelay(pdMS_TO_TICKS(1000));
}

TEST_CASE("test clear display", "[display]") {
  clear_display(neopixels);
  vTaskDelay(pdMS_TO_TICKS(1000));
}

/**
 * Show rainbow horizontal stripes down display (each row different color)
 */
TEST_CASE("test Display Colors", "[display]") {
  tNeopixel pixelArr[DISPLAY_COLS];

  // for each row, go through the colors of the pieces and make a rainbow
  for (int row = 0; row < DISPLAY_ROWS; row++) {
    uint32_t cell_color =
        getRGBFromCellColor(all_cell_colors[row % (NUM_TETRIS_COLORS - 1)]);
    for (int col = 0; col < DISPLAY_COLS; col++) {
      pixelArr[col] = (tNeopixel){rowcol_to_LEDNum_LUT[row][col], cell_color};
    }
    neopixel_SetPixel(neopixels, pixelArr, DISPLAY_COLS);
  }
  vTaskDelay(pdMS_TO_TICKS(1000));
}

/**
 * create a outer red border around the display using a TetrisBoard
 * and then display it
 */
TEST_CASE("Display border", "[display]") {
  // create tetrisboard and fill outer pixels of board with color
  TetrisBoard tb = init_board();
  setRingFromOutsideToColor(&tb, 0, Z_CELL_COLOR);
  display_board(neopixels, &tb);
  vTaskDelay(pdMS_TO_TICKS(500));

  setRingFromOutsideToColor(&tb, 1, S_CELL_COLOR);
  display_board(neopixels, &tb);
  vTaskDelay(pdMS_TO_TICKS(500));

  setRingFromOutsideToColor(&tb, 2, L_CELL_COLOR);
  display_board(neopixels, &tb);
  vTaskDelay(pdMS_TO_TICKS(500));

  setRingFromOutsideToColor(&tb, 3, I_CELL_COLOR);
  display_board(neopixels, &tb);
  vTaskDelay(pdMS_TO_TICKS(500));
}

TEST_CASE("display empty TetrisBoard", "[display]") {
  TetrisBoard tb = init_board();
  display_board(neopixels, &tb);
}

TEST_CASE("Display example_board", "[display]") {
  TetrisBoard tb = init_board();
  memcpy(&tb.board, example_board, sizeof(example_board));
  display_board(neopixels, &tb);

  vTaskDelay(pdMS_TO_TICKS(2000));
}

TEST_CASE("Test Display play again mask over board", "[display]") {
  display_play_again_icon(neopixels);
  // display_mask_over_board(neopixels, 2, play_again_icon_mask, 5, 0);

  vTaskDelay(pdMS_TO_TICKS(2000));
}

////////////////////////////////////////
// helper functions
////////////////////////////////////////

static void setRingFromOutsideToColor(TetrisBoard *tb,
                                      uint8_t rings_from_outside,
                                      int8_t color) {
  assert(rings_from_outside < TETRIS_COLS / 2 &&
         "rings_from_outside out of bounds!");

  // verticals
  for (int row = rings_from_outside; row < TETRIS_ROWS - rings_from_outside;
       row++) {
    tb->board[row][0 + rings_from_outside]               = color;
    tb->board[row][TETRIS_COLS - 1 - rings_from_outside] = color;
  }

  // horizontals
  for (int col = rings_from_outside; col < TETRIS_COLS - rings_from_outside;
       col++) {
    tb->board[0 + rings_from_outside][col]               = color;
    tb->board[TETRIS_ROWS - 1 - rings_from_outside][col] = color;
  }
}

/**
 * Prints array contents without newline
 * @param *array unsigned int
 * @param length unsigned int
 */
void printArrayContents(const uint8_t *array, const uint8_t length) {
  printf("{ ");
  for (int i = 0; i < length - 1; i++) {
    printf("%d, ", array[i]);
  }
  printf("%d }", array[length - 1]);
}

// clang-format off
/**
 * Example board created from ini print in tetris game driver
*/
static const int8_t example_board[32][8] = {
    {-1,-1,-1,-1,-1,-1,-1,-1},
    {-1,-1,-1,-1,-1,-1,-1,-1},
    {-1,-1,-1,-1,-1,-1,-1,-1},
    {-1,-1,-1,-1,-1,-1,-1,-1},
    {-1,-1,-1,-1,-1,-1,-1,-1},
    {-1,-1,-1,-1,-1,-1,-1,-1},
    {-1,-1,-1,-1,-1,-1,-1,-1},
    {-1,-1,-1,-1, 5, 5,-1,-1},
    {-1,-1,-1,-1, 5, 5,-1,-1},
    {-1,-1,-1,-1,-1,-1,-1,-1},
    {-1,-1,-1,-1,-1,-1,-1,-1},
    {-1,-1,-1,-1,-1,-1,-1,-1},
    {-1,-1,-1,-1,-1,-1,-1,-1},
    {-1,-1,-1,-1,-1,-1,-1,-1},
    {-1,-1,-1,-1,-1,-1,-1,-1},
    {-1,-1,-1,-1,-1,-1,-1,-1},
    {-1,-1,-1,-1,-1,-1,-1,-1},
    {-1,-1,-1,-1,-1,-1,-1,-1},
    {-1,-1,-1,-1,-1,-1,-1,-1},
    {-1,-1,-1,-1,-1,-1,-1,-1},
    {-1,-1,-1, 5, 5,-1,-1,-1},
    { 5, 5,-1, 5, 5,-1,-1,-1},
    { 5, 5,-1, 2, 2, 2,-1,-1},
    {-1, 0,-1, 3, 2, 2, 2, 2},
    {-1, 0, 0, 3,-1,-1, 2,-1},
    {-1,-1, 0, 3, 3, 2, 2, 2},
    {-1, 2, 2, 2,-1, 1, 2,-1},
    {-1, 6, 2,-1, 1, 1,-1, 3},
    {-1, 6, 0,-1, 1, 3, 3, 3},
    { 6, 6, 1, 0, 4,-1, 5, 5},
    { 3, 1, 4, 1, 1,-1, 3, 3},
    { 1, 1, 4, 1, 2,-1, 5, 5}
};
// clang-format on
