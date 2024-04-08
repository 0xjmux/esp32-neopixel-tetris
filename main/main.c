
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <assert.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_now.h"


#include "defs.h"
#include "espnow_remote.h"
#include "tetris.h"
#include "neopixel.h"       // fast neopixel library


/**
 * Game loop task - handles running tetris game and updating display
*/
void tetris_game_loop_task(void *pvParameter) {
    TetrisGame *tg;
    remote_button_info buttons_state;
    char button_name_str[SHORT_STR_LEN];

    bool game_paused = false;


    tg = create_game();
    enum player_move move = T_NONE;
    create_rand_piece(tg);      // create first piece



    while (!tg->game_over && move != T_QUIT) {

        // if the game is currently paused
        if(game_paused) {
            // check if we've unpaused by now
            if(get_buttons_state().button_val == WIZMOTE_BUTTON_NIGHT) {
                panic_toggle_stat_led(0);
                reset_internal_buttons_state();
                ESP_LOGI(TAG, "GAME UNPAUSED");
                game_paused= false;
                continue;
            }
            // if not, wait around and then check again
            else {
                vTaskDelay(100 / portTICK_PERIOD_MS);
                continue;
            }
        }

        // this function handles basically everything about the internal game state. 
        // all the driver really has to do is pass `move` along to it and then print out
        //  the tg->active_board array in whatever format is desired
        tg_tick(tg, move);

        switch(buttons_state.button_val) {
            case (WIZMOTE_BUTTON_ON):               // 
                strcpy(button_name_str, "ON");
                break;
            case (WIZMOTE_BUTTON_OFF):              // QUIT
                strcpy(button_name_str, "QUIT (off)");
                ESP_LOGE(TAG, "QUITTING GAME!");
                move = T_QUIT;
                break;
            case (WIZMOTE_BUTTON_NIGHT):            // PAUSE
                strcpy(button_name_str, "PAUSE (night)");
                game_paused = true;
                ESP_LOGI(TAG, "GAME PAUSED!");
                panic_toggle_stat_led(1);

                move = T_NONE;
                break;

            case (WIZMOTE_BUTTON_ONE):              // LEFT
                strcpy(button_name_str, "LEFT (1)");
                move = T_LEFT;
                break;
            case (WIZMOTE_BUTTON_TWO):              // UP
                strcpy(button_name_str, "UP (2)");
                move = T_UP;
                break;
            case (WIZMOTE_BUTTON_THREE):            // DOWN
                strcpy(button_name_str, "DOWN (3)");
                move = T_DOWN;
                break;
            case (WIZMOTE_BUTTON_FOUR):             // RIGHT
                strcpy(button_name_str, "RIGHT (4)");
                move = T_RIGHT;
                break;
            case (WIZMOTE_BUTTON_BRIGHT_UP):        // 
                strcpy(button_name_str, "BRIGHT_UP");
                break;
            case (WIZMOTE_BUTTON_BRIGHT_DOWN):      // 
                strcpy(button_name_str, "BRIGHT_DOWN");
                break;
            default:
                move = T_NONE;
        }
        buttons_state = get_buttons_state();
        reset_internal_buttons_state();

        // if we received a move
        if (move != T_NONE) {
            ESP_LOGI(TAG, "Game loop received button/move %s", button_name_str);
        }

        // delay for 10ms
        vTaskDelay(10 / portTICK_PERIOD_MS);

    }

    // if we're here, game is over; dealloc tg
    end_game(tg);
    ESP_LOGI(TAG, "Game over! Level=%ld, Score=%ld\n", tg->level, tg->score);
    assert(0 && "reached end of game loop!");
}




void app_main(void)
{
    ESP_LOGI(TAG, "Starting main");



    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK( nvs_flash_erase() );
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );

    // ESP_ERROR_CHECK( heap_trace_init_standalone(trace_record, NUM_RECORDS) );

    example_wifi_init();
    // set up esp-now functionality

    // ESP_LOGI(TAG, "Starting remote: prior vals last_seq=%ld", last_msg_seq);
    espnow_remote_recv_init();

    // start game loop task
    // TaskHandle_t tetris_task_handle = NULL;
    // xTaskCreate(tetris_game_loop_task, "tetris_game_loop_task", TASK_STACK_DEPTH_BYTES, NULL, 4, &tetris_task_handle);
    // ESP_LOGI(TAG, "Tetris task created with handle %p", tetris_task_handle);
}