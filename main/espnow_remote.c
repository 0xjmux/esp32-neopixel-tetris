/**
 * Test code for ingesting messages from the Wizmote ESP-NOW remote
 * @file espnow_remote_test.c
*/

#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <assert.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"
// #include "nvs_flash.h"
#include "esp_random.h"
// #include "esp_event.h"
// #include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_now.h"
// #include "esp_crc.h"


#include "driver/gpio.h"     // for LED panic function

#include "espnow_remote.h"

// global struct for last buttons pressed on remote
static remote_button_info button_info;

// HEAP TRACING
// #include "esp_heap_trace.h"
// #include "esp_sysview_trace.h"

static uint32_t last_msg_seq = 0;      // seq number of last message
// static espnow_msg_structure incoming;           // holds incoming message data

static QueueHandle_t s_example_espnow_queue;    // semaphore for espnow handling
static uint8_t s_example_broadcast_mac[ESP_NOW_ETH_ALEN] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

/* WiFi should start before using ESPNOW */
void example_wifi_init(void)
// static void example_wifi_init(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
    ESP_ERROR_CHECK( esp_wifi_set_mode(ESPNOW_WIFI_MODE) );
    ESP_ERROR_CHECK( esp_wifi_start());
    ESP_ERROR_CHECK( esp_wifi_set_channel(CONFIG_ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE));

#if CONFIG_ESPNOW_ENABLE_LONG_RANGE
    ESP_ERROR_CHECK( esp_wifi_set_protocol(ESPNOW_WIFI_IF, WIFI_PROTOCOL_11B|WIFI_PROTOCOL_11G|WIFI_PROTOCOL_11N|WIFI_PROTOCOL_LR) );
#endif
}

// turn the STAT led on to indicate something went wrong
void set_stat_led_state(bool ledState) {
    #ifdef STAT_LED_PIN
        if (ledState) {
            ESP_LOGE(TAG, "Panic LED triggered!");
            gpio_set_level(STAT_LED_PIN, 1);
        }
        else {
            ESP_LOGE(TAG, "Panic LED turned off!");
            gpio_set_level(STAT_LED_PIN, 0);
        }
    #endif
}

/**
 * For debugging: given a button id `button`, save 
 * the name of the button as a string to `button_name_str`
 * 
*/
void get_button_name_from_number(const uint8_t button, char *button_name_str) {
    switch(button) {
        case (WIZMOTE_BUTTON_ON):
            strcpy(button_name_str, "ON");
            break;
        case (WIZMOTE_BUTTON_OFF):
            strcpy(button_name_str, "OFF");
            break;
        case (WIZMOTE_BUTTON_NIGHT):
            strcpy(button_name_str, "NIGHT");
            break;
        case (WIZMOTE_BUTTON_ONE):
            strcpy(button_name_str, "ONE");
            break;
        case (WIZMOTE_BUTTON_TWO):
            strcpy(button_name_str, "TWO");
            break;
        case (WIZMOTE_BUTTON_THREE):
            strcpy(button_name_str, "THREE");
            break;
        case (WIZMOTE_BUTTON_FOUR):
            strcpy(button_name_str, "FOUR");
            break;
        case (WIZMOTE_BUTTON_BRIGHT_UP):
            strcpy(button_name_str, "BRIGHT_UP");
            break;
        case (WIZMOTE_BUTTON_BRIGHT_DOWN):
            strcpy(button_name_str, "BRIGHT_DOWN");
            break;
        default:
            ESP_LOGE(TAG, "invalid button=%d passed to get_button_name_from_number!", button);
            break;
    }
}


// FROM EXAMPLE
void example_espnow_recv_cb(const esp_now_recv_info_t *recv_info, const uint8_t *data, int len)
// static void example_espnow_recv_cb(const esp_now_recv_info_t *recv_info, const uint8_t *data, int len)
{

    // ESP_ERROR_CHECK( heap_trace_start(HEAP_TRACE_LEAKS) );

    // example_espnow_event_t evt;
    example_espnow_event_recv_cb_t recv_cb; 
    uint8_t *mac_addr = recv_info->src_addr;

    // check for invalid packet
    if (mac_addr == NULL || data == NULL || len <= 0) {
        ESP_LOGE(TAG, "Receive cb arg error");
        //  return;     // these functions arent't ever supposed to return
    }

    // copy MAC addr and copy data for recv_cb
    memcpy(recv_cb.mac_addr, mac_addr, ESP_NOW_ETH_ALEN);
    
    // this line causes fault????
    recv_cb.data = malloc(len);

    if (recv_cb.data == NULL) {
        ESP_LOGE(TAG, "Malloc receive data fail");
        return;      // these  functions aren't ever supposed to return
    }
    else {
        ESP_LOGD(TAG, "Malloc recv_cb.data len=%d SUCCESS: %p", len, recv_cb.data);
    }
    memcpy(recv_cb.data, data, len);

    // add packet to queue to be processed
    recv_cb.data_len = len;
    if (xQueueSend(s_example_espnow_queue, &recv_cb, ESPNOW_MAXDELAY) != pdTRUE) {
        ESP_LOGE(TAG, "Send receive queue fail");
        free(recv_cb.data);
    }


}

// FROM EXAMPLE
/* Parse received ESPNOW data. */
static uint8_t example_espnow_data_parse(uint8_t *data, uint16_t data_len, uint8_t *program, \
    uint32_t *seq, uint8_t *button)
{
    espnow_msg_structure *buf = (espnow_msg_structure *)data;
    // uint16_t crc, crc_cal = 0;

    if (data_len < sizeof(espnow_msg_structure)) {
        ESP_LOGE(TAG, "Receive ESPNOW data too short, len:%d ; expected=%d", data_len, sizeof(espnow_msg_structure));

        set_stat_led_state(1);

        return DATA_PARSE_ERR;
    }

    *program = buf->program;
    *seq = buf->seq;
    *button = buf->button;

    // if we've already seen this msg before
    if (*seq <= last_msg_seq) {
        return DATA_PARSE_STALE;
    }

    // save button to global variable
    button_info.button_val = *button;
    button_info.program_val = *program;

    last_msg_seq = *seq;

    ESP_LOGD(TAG, "completed espnow_data_parse()");

    return DATA_PARSE_OK;
}


/**
 * FreeRTOS task to handle data reception
*/
// static void espnow_recv_task(void *pvParameter)
void espnow_recv_task(void *pvParameter)
{


    example_espnow_event_recv_cb_t recv_cb;
    uint8_t program = 0;
    // uint8_t seq[4] = { 0 };
    uint32_t seq = 0;
    uint8_t button = 0;
    int ret;

    vTaskDelay(100 / portTICK_PERIOD_MS);

    while (xQueueReceive(s_example_espnow_queue, &recv_cb, portMAX_DELAY) == pdTRUE) {
        // example_espnow_event_recv_cb_t *recv_cb = &evt.info.recv_cb;


        // ret = example_espnow_data_parse(recv_cb.data, recv_cb.data_len, &recv_state, &recv_seq, &recv_magic);
        // SEQ MIGHT NOT NEED & 
        ret = example_espnow_data_parse(recv_cb.data, recv_cb.data_len, &program, &seq, &button);

        // original data not needed since we've parsed out the parts we want
        ESP_LOGD(TAG, "Freeing recv_cb.data %p", recv_cb.data);
        free(recv_cb.data);




        if (ret == DATA_PARSE_OK) {
            ESP_LOGD(TAG, "Receive seq=%ld data from: "MACSTR", len: %d\n", \
                seq, MAC2STR(recv_cb.mac_addr), recv_cb.data_len);

            /* If MAC address does not exist in peer list, add it to peer list. */
            // CRASH HAPPENED HERE WITH IF STATEMENT - NULL PTR DEREFERENCE
            //  crash also mentioned something about semaphore??
            assert(recv_cb.mac_addr != NULL);
            if (esp_now_is_peer_exist(recv_cb.mac_addr) == false) {
                ESP_LOGI(TAG, "peer not already present in peer list, adding now");

                // esp_now_peer_info_t *peer = malloc(sizeof(esp_now_peer_info_t));
                esp_now_peer_info_t peer = {
                    .peer_addr = {0},
                    .channel = CONFIG_ESPNOW_CHANNEL,
                    .lmk = CONFIG_ESPNOW_LMK,
                    .ifidx = ESPNOW_WIFI_IF,
                    .encrypt = false,
                };

                assert(peer.lmk != NULL);
                memcpy(peer.lmk, CONFIG_ESPNOW_LMK, ESP_NOW_KEY_LEN);
                assert(peer.peer_addr != NULL);
                memcpy(peer.peer_addr, recv_cb.mac_addr, ESP_NOW_ETH_ALEN);
                
                ESP_LOGI(TAG, "attempting to add peer recv.mac="MACSTR" peer.mac="MACSTR" lmk=%s on channel=%d", \
                    MAC2STR(recv_cb.mac_addr), MAC2STR(peer.peer_addr), peer.lmk, peer.channel);
                // PANIC was THROWN HERE
                ESP_ERROR_CHECK( esp_now_add_peer(&peer) );
                ESP_LOGI(TAG, "Peer added successfully");
                /*
                    // esp_now_peer_info_t peer_confirm;
                    // esp_now_get_peer(peer.peer_addr, &peer_confirm);
                    // ESP_LOGI("Get peer info: ADDR: "MACSTR", CHAN=%d, LMK=%s, IDX=%d", \
                    //     MAC2STR(&peer_confirm.peer_addr), peer_confirm.channel, peer_confirm.lmk, peer_confirm.ifidx );
                */
            }
            char button_name[SHORT_STR_LEN] = "";
            get_button_name_from_number(button, button_name);
            ESP_LOGI(TAG, "Incoming ESP-NOW Packet [SEQ=%ld, button=%d: %s] \n", seq, button, button_name);

            // ESP_LOGI(TAG, "Incoming ESP-NOW Packet [SEQ=%ld, button=%d] \n", seq, button);
            // ESP_ERROR_CHECK( heap_trace_stop() );
            // heap_trace_dump();
        }
        else if (ret == DATA_PARSE_STALE) {
            /*
            // ESP_LOGI(TAG, "Received stale packet from "MACSTR" [SEQ=%ld, button=%d], last_seq=%ld \n", \
            //     MAC2STR(recv_cb.mac_addr), seq, button, last_msg_seq);
            */
            ESP_LOGD(TAG, ".");

        }
        else {
            ESP_LOGI(TAG, "Receive error data from: "MACSTR"", MAC2STR(recv_cb.mac_addr));
        }

        // break;
    

    }
}

// a lot of this copied from espnow_example_main.c
// static esp_err_t espnow_remote_recv_init(void) {
esp_err_t espnow_remote_recv_init(void) {
 
    s_example_espnow_queue = xQueueCreate(ESPNOW_QUEUE_SIZE, sizeof(example_espnow_event_recv_cb_t));
    if (s_example_espnow_queue == NULL) {
        ESP_LOGE(TAG, "Create mutex fail");
        return ESP_FAIL;
    }    
    
    ESP_ERROR_CHECK(esp_now_init()); // must be called to set up ESP-NOW, but after wifi

    ESP_ERROR_CHECK( esp_now_register_recv_cb(example_espnow_recv_cb) );
    // ESP_ERROR_CHECK( esp_now_register_recv_cb(on_data_recv) );

    /* Set primary master key. (not used in my sketch )*/
    // ESP_ERROR_CHECK( esp_now_set_pmk((uint8_t *)CONFIG_ESPNOW_PMK) );


    esp_now_peer_info_t peer = {
        .peer_addr = {0},
        .channel = CONFIG_ESPNOW_CHANNEL,
        .lmk = CONFIG_ESPNOW_LMK,
        .ifidx = ESPNOW_WIFI_IF,
        .encrypt = false,
    };
    // idk trying something new
    // if (&peer == NULL) {
    //     ESP_LOGE(TAG, "peer init fail");
    //     panic_stat_led_on();
    //     espnow_remote_recv_deinit();
    //     vTaskDelete(NULL);
    // }

    memcpy(peer.lmk, CONFIG_ESPNOW_LMK, ESP_NOW_KEY_LEN);
    memcpy(peer.peer_addr, s_example_broadcast_mac, ESP_NOW_ETH_ALEN);
    
    ESP_LOGI(TAG, "attempting to add peer: "MACSTR" on channel=%d", MAC2STR(peer.peer_addr), peer.channel);
    ESP_ERROR_CHECK( esp_now_add_peer(&peer) );

    #ifdef STAT_LED_PIN
        gpio_reset_pin(STAT_LED_PIN);
        gpio_set_direction(STAT_LED_PIN, GPIO_MODE_OUTPUT);
    #endif

    // task params aren't used rn
    uint8_t *recv_task_params = 0;
    TaskHandle_t recv_task_handle = NULL;
    xTaskCreate(espnow_recv_task, "espnow_recv_task", TASK_STACK_DEPTH_BYTES, recv_task_params, 4, &recv_task_handle);
    // xTaskCreatePinnedToCore(espnow_recv_task, "espnow_recv_task", TASK_STACK_DEPTH_BYTES, recv_task_params, 4, &recv_task_handle, 0);
    ESP_LOGI(TAG, "Successfully finished remote_recv_init(), task created with handle %p", recv_task_handle);


    return ESP_OK;
}

void espnow_remote_recv_deinit(void) {
    vSemaphoreDelete(s_example_espnow_queue);
    esp_now_deinit();
}


remote_button_info get_buttons_state(void) {
    return button_info;
}

// reset values of button_info global variable back to zero
void reset_internal_buttons_state(void) {
    button_info.button_val = 0;
    button_info.program_val = 0;

}