
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_now.h"
#include "esp_wifi.h"
#include "espnow_remote.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"
#include "neopixel.h"          // fast neopixel library
#include "neopixel_display.h"  // my neopixel array driver
#include "npix_tetris_defs.h"
#include "nvs_flash.h"
#include "tetris.h"

static SemaphoreHandle_t mutex;

/**
 * Game loop task - handles running tetris game and updating display
 */
void tetris_game_loop_task(void *pvParameter) {
  (void)pvParameter;

  remote_button_info buttons_state;
  char button_name_str[SHORT_STR_LEN];
  bool game_paused = false;

  tNeopixelContext *neopixels = neopixel_Init(PIXEL_COUNT, NEOPIXEL_PIN);
  if (neopixels == NULL) {
    ESP_LOGE(TAG, "Failed to allocate tNeopixelContext!!\n");
    assert(0 && "failed to allocate tNeoPixelContext!");
  }

  TetrisGame *tg;
  tg                    = create_game();
  enum player_move move = T_NONE;
  create_rand_piece(tg);  // create first piece

  clear_display(neopixels);
  display_board(neopixels, &tg->active_board);
  ESP_LOGD(TAG, "Beginning main game loop\n");

  while (!tg->game_over && move != T_QUIT) {
    // if the game is currently paused
    if (game_paused) {
      // check if we've unpaused by now
      if (get_buttons_state().button_val == WIZMOTE_BUTTON_NIGHT) {
        set_stat_led_state(0);
        reset_internal_buttons_state();
        display_board(neopixels, &tg->active_board);
        ESP_LOGI(TAG, "GAME UNPAUSED");
        game_paused = false;
        continue;
      }
      // if not, wait around and then check again
      else {
        vTaskDelay(100 / portTICK_PERIOD_MS);
        continue;
      }
    }

    // this function handles basically everything for the internal tetris game
    // state
    tg_tick(tg, move);

    // prevent trying to update display multiple times at once
    // check if we can take the mutex with a wait time of 10 ticks
    if (xSemaphoreTake(mutex, (TickType_t)10) == pdTRUE) {
      // if we can take it, update the display.
      display_board(neopixels, &tg->active_board);
      xSemaphoreGive(mutex);
      // if we couldn't take it, we just don't update the display this iteration
    }

    switch (buttons_state.button_val) {
      case (WIZMOTE_BUTTON_ON):  //
        strcpy(button_name_str, "ON");
        display_board(neopixels, &tg->active_board);
        break;
      case (WIZMOTE_BUTTON_OFF):  // QUIT
        strcpy(button_name_str, "QUIT (off)");
        ESP_LOGE(TAG, "QUITTING GAME!");
        move = T_QUIT;
        break;
      case (WIZMOTE_BUTTON_NIGHT):  // PAUSE
        strcpy(button_name_str, "PAUSE (night)");
        set_stat_led_state(1);
        game_paused = true;
        ESP_LOGI(TAG, "GAME PAUSED!");
        move = T_NONE;
        display_pause_icon(neopixels);
        break;

      case (WIZMOTE_BUTTON_ONE):  // LEFT
        strcpy(button_name_str, "LEFT (1)");
        move = T_LEFT;
        break;
      case (WIZMOTE_BUTTON_TWO):  // UP
        strcpy(button_name_str, "UP (2)");
        move = T_UP;
        break;
      case (WIZMOTE_BUTTON_THREE):  // DOWN
        strcpy(button_name_str, "DOWN (3)");
        move = T_DOWN;
        break;
      case (WIZMOTE_BUTTON_FOUR):  // RIGHT
        strcpy(button_name_str, "RIGHT (4)");
        move = T_RIGHT;
        break;
      case (WIZMOTE_BUTTON_BRIGHT_UP):  //
        strcpy(button_name_str, "BRIGHT_UP");
        break;
      case (WIZMOTE_BUTTON_BRIGHT_DOWN):  //
        strcpy(button_name_str, "BRIGHT_DOWN");
        break;
      default:
        move = T_NONE;
    }
    buttons_state = get_buttons_state();
    reset_internal_buttons_state();

    // if we received a move
    // if (move != T_NONE) {
    //     ESP_LOGI(TAG, "Game loop received button/move %s", button_name_str);
    // }

    // delay
    vTaskDelay(pdMS_TO_TICKS(15));
  }

  ESP_LOGI(TAG, "Game over! Level=%ld, Score=%ld\n", tg->level, tg->score);
  display_board(neopixels, &tg->active_board);
  printTetrisBoardToLog(&tg->active_board);
  // wait for print to finish before aborting
  vTaskDelay(pdMS_TO_TICKS(800));

  // if we're here, game is over; dealloc tg
  end_game(tg);
  neopixel_Deinit(neopixels);
  assert(0 && "reached end of game loop!");
}

void app_main(void) {
  ESP_LOGI(TAG, "Starting main");

  // Initialize NVS
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
      ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);

  // ESP_ERROR_CHECK( heap_trace_init_standalone(trace_record, NUM_RECORDS) );

  example_wifi_init();

  // ESP_LOGI(TAG, "Starting remote: prior vals last_seq=%ld", last_msg_seq);
  espnow_remote_recv_init();

  // Create mutex before starting tasks
  mutex = xSemaphoreCreateMutex();

  // start game loop task
  TaskHandle_t tetris_task_handle = NULL;
  xTaskCreate(tetris_game_loop_task, "tetris_game_loop_task",
              TASK_STACK_DEPTH_BYTES, NULL, 4, &tetris_task_handle);
  ESP_LOGI(TAG, "Tetris task created with handle %p", tetris_task_handle);
}
