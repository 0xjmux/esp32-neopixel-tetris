#include "unity.h"
#include "esp_log.h"
#include <string.h>

#include "neopixel.h"
#include "neopixel_display.h"
#include "npix_tetris_defs.h"


#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"        // for vTaskDelay

#define DELAY_1000MS (1000 / portTICK_PERIOD_MS)

void setRingFromOutsideToColor(TetrisBoard *tb, uint8_t rings_from_outside, int8_t color);

// allow iterating through cell colors
const int8_t all_cell_colors[NUM_TETRIS_COLORS] = 
{ S_CELL_COLOR, Z_CELL_COLOR, T_CELL_COLOR, L_CELL_COLOR, J_CELL_COLOR, SQ_CELL_COLOR, I_CELL_COLOR, BG_COLOR };
// Green        Red         Magenta         Orange          Blue        Yellow          light blue      Off

tNeopixelContext neopixels;


// set stuff up here
void setUp(void) {
    neopixels = neopixel_Init(PIXEL_COUNT, NEOPIXEL_PIN);
    // clear_display(neopixels);
}

void tearDown(void) {
    vTaskDelay(DELAY_1000MS);
    neopixel_Deinit(neopixels);
}

/**
 * Display red in top right corner, blue in bot left
 * This will be manually checked by a human
*/
TEST_CASE("test matrix orientation", "[display]")
{
    uint32_t refreshRate = neopixel_GetRefreshRate(neopixels);
    // uint32_t taskDelay = MAX(1, pdMS_TO_TICKS(1000UL / refreshRate));
    printf("refreshrate=%ld\n", refreshRate);

    // set topR to red
    tNeopixel topRightNeopix = {TOP_RIGHT_LED, getRGBFromCellColor(S_CELL_COLOR)};
    neopixel_SetPixel(neopixels, &topRightNeopix, 1);

    // set botL to blue
    tNeopixel botLeftNeopix = {BOT_LEFT_LED, getRGBFromCellColor(J_CELL_COLOR)};
    neopixel_SetPixel(neopixels, &botLeftNeopix, 1);

}


TEST_CASE("test clear display", "[display]") {
    clear_display(neopixels);
}

/**
 * Show rainbow horizontal stripes down display (each row different color)
*/
TEST_CASE("test Display Colors", "[display]") {
    tNeopixel pixelArr[DISPLAY_COLS];

    // for each row, go through the colors of the pieces and make a rainbow
    for (int row = 0; row < DISPLAY_ROWS; row++) {
        uint32_t cell_color = getRGBFromCellColor(all_cell_colors[row % (NUM_TETRIS_COLORS - 1)]);
        for (int col = 0; col < DISPLAY_COLS; col++) {
            pixelArr[col] = (tNeopixel){ rowcol_to_LEDNum_LUT[row][col], cell_color};
        }
        neopixel_SetPixel(neopixels, pixelArr, DISPLAY_COLS);
    }
}


TEST_CASE("test getRGBFromCellColor", "[display]") {
    // for each cell color
    TEST_ASSERT_EQUAL(NP_RGB(0,50,0), getRGBFromCellColor(S_CELL_COLOR));
    TEST_ASSERT_EQUAL(NP_RGB(50,0,0), getRGBFromCellColor(Z_CELL_COLOR));
    TEST_ASSERT_EQUAL(NP_RGB(50,0,50), getRGBFromCellColor(T_CELL_COLOR));
    TEST_ASSERT_EQUAL(NP_RGB(50,25,0), getRGBFromCellColor(L_CELL_COLOR));
    TEST_ASSERT_EQUAL(NP_RGB(0,0,50), getRGBFromCellColor(J_CELL_COLOR));
    TEST_ASSERT_EQUAL(NP_RGB(50,50,0), getRGBFromCellColor(SQ_CELL_COLOR));
    TEST_ASSERT_EQUAL(NP_RGB(0,50,50), getRGBFromCellColor(I_CELL_COLOR));
    TEST_ASSERT_EQUAL(NP_RGB(0,0,0), getRGBFromCellColor(BG_COLOR));
}



TEST_CASE("display empty TetrisBoard", "[display]") {
    TetrisBoard tb = init_board();
    display_board(neopixels, &tb);
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




////////////////////////////////////////
// helper functions
////////////////////////////////////////

void setRingFromOutsideToColor(TetrisBoard *tb, uint8_t rings_from_outside, int8_t color) {
    assert(rings_from_outside < TETRIS_COLS / 2 && "rings_from_outside out of bounds!");

    // verticals
    for (int row = rings_from_outside; row < TETRIS_ROWS - rings_from_outside; row++) {
        tb->board[row][0+rings_from_outside] = color;
        tb->board[row][TETRIS_COLS-1-rings_from_outside] = color;
    }

    // horizontals
    for (int col = rings_from_outside; col < TETRIS_COLS - rings_from_outside; col++) {
        tb->board[0+rings_from_outside][col] = color;
        tb->board[TETRIS_ROWS-1-rings_from_outside][col]= color;
    }

}